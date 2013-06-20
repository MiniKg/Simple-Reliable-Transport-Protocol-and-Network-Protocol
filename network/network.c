//FILE: network/network.c
//
//Description: this file implements network layer process
//
//Date: April 29,2008

#include "network.h"

//network layer waits this time for establishing the routing paths
#define NETWORK_WAITTIME 20

/**************************************************************/
//delare global variables
/**************************************************************/
int overlay_conn; 			//connection to the overlay
int transport_conn;			//connection to the transport
nbr_cost_entry_t* nct;			//neighbor cost table
dv_t* dvt;				//distance vector table
pthread_mutex_t* dv_mutex;		//dvtable mutex
routingtable_t* routingtable;		//routing table
pthread_mutex_t* routingtable_mutex;	//routingtable mutex


/**************************************************************/
//implementation network layer functions
/**************************************************************/

//This function is used to for the SNP process to connect to the local ON process on port OVERLAY_PORT.
//TCP descriptor is returned if success, otherwise return -1.
int connectToOverlay() {
	//put your code here
    struct sockaddr_in servaddr;
    int conn;
    
    if ((conn = socket (AF_INET, SOCK_STREAM, 0)) <0) {
        perror("\nProblem in creating the socket in connectToOverlay");
        return -1;
    }
    
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr= inet_addr("127.0.0.1");
    servaddr.sin_port = htons(OVERLAY_PORT);
    
    if (connect(conn, (struct sockaddr *) &servaddr, sizeof(servaddr))<0) {
        perror("\nProblem in connecting to the server in connectToOverlay");
        return -1;
    }
    
    return conn;
}

//This thread sends out route update packets every ROUTEUPDATE_INTERVAL time
//The route update packet contains this node's distance vector.
//Broadcasting is done by set the dest_nodeID in packet header as BROADCAST_NODEID
//and use overlay_sendpkt() to send the packet out using BROADCAST_NODEID address.
void* routeupdate_daemon(void* arg) {
	//put your code here
    int nodeNum = topology_getNodeNum();
    int myNodeID = topology_getMyNodeID();
    int i;
    pkt_routeupdate_t *pktRouteUpdt = (pkt_routeupdate_t *)malloc(sizeof(pkt_routeupdate_t));
    snp_pkt_t *pkt = (snp_pkt_t *)malloc(sizeof(snp_pkt_t));
    
    while (1) {
        sleep(ROUTEUPDATE_INTERVAL);
        
        memset(pktRouteUpdt, 0, sizeof(pkt_routeupdate_t));
        pktRouteUpdt->entryNum = nodeNum;
        pthread_mutex_lock(dv_mutex);
        for (i = 0; i < nodeNum; i++) {     //this node's own dv
            pktRouteUpdt->entry[i].nodeID = dvt[0].dvEntry[i].desNodeID;
            pktRouteUpdt->entry[i].cost = dvt[0].dvEntry[i].cost;
        }
        pthread_mutex_unlock(dv_mutex);
        
        memset(pkt, 0, sizeof(snp_pkt_t));
        pkt->header.src_nodeID = myNodeID;
        pkt->header.dest_nodeID = BROADCAST_NODEID;
        pkt->header.length = sizeof(unsigned int) + nodeNum * sizeof(routeupdate_entry_t);
        pkt->header.type = ROUTE_UPDATE;
        memcpy(pkt->data, pktRouteUpdt, pkt->header.length);

        overlay_sendpkt(BROADCAST_NODEID, pkt, overlay_conn);
//        puts("sent a broadcast");
    }
    
    free(pktRouteUpdt);
    free(pkt);
    pthread_exit(NULL);
}

//This thread handles incoming packets from the ON process.
//It receives packets from the ON process by calling overlay_recvpkt().
//If the packet is a SNP packet and the destination node is this node, forward the packet to the SRT process.
//If the packet is a SNP packet and the destination node is not this node, forward the packet to the next hop according to the routing table.
//If this packet is an Route Update packet, update the distance vector table and the routing table.
void* pkthandler(void* arg) {
	//put your code here
    snp_pkt_t *pkt = (snp_pkt_t *)malloc(sizeof(snp_pkt_t));
    seg_t *seg = (seg_t *)malloc(sizeof(seg_t));
    pkt_routeupdate_t *pktRouteUpdt = (pkt_routeupdate_t *)malloc(sizeof(pkt_routeupdate_t));
    int myNodeID = topology_getMyNodeID();
    int i;
    
    while (overlay_recvpkt(pkt, overlay_conn) > 0) {
        if (pkt->header.type == SNP) {
            if (pkt->header.dest_nodeID == myNodeID) {
                memcpy(seg, pkt->data, pkt->header.length);
//                puts(seg->data);
                forwardsegToSRT(transport_conn, pkt->header.src_nodeID, seg);
            }
            else {
                pthread_mutex_lock(routingtable_mutex);
                overlay_sendpkt(routingtable_getnextnode(routingtable, pkt->header.dest_nodeID), pkt, overlay_conn);
                printf("redirect happened, srcNodeID is %d, destNodeID is %d!\n", pkt->header.src_nodeID, pkt->header.dest_nodeID);
//                memcpy(seg, pkt->data, pkt->header.length);
//                puts(seg->data);
                pthread_mutex_unlock(routingtable_mutex);
            }
        }
        else if (pkt->header.type == ROUTE_UPDATE) {
            memcpy(pktRouteUpdt, pkt->data, pkt->header.length);
            
            //update the neighbor's dv
            pthread_mutex_lock(dv_mutex);
            for (i = 0; i < pktRouteUpdt->entryNum; i++) {
                dvtable_setcost(dvt, pkt->header.src_nodeID, pktRouteUpdt->entry[i].nodeID, pktRouteUpdt->entry[i].cost);
            }
        
            pthread_mutex_lock(routingtable_mutex);
            //update the node's own dv and rt
            for (i = 0; i < pktRouteUpdt->entryNum; i++) {
                if (dvt[0].dvEntry[i].cost > (nbrcosttable_getcost(nct, pkt->header.src_nodeID) + pktRouteUpdt->entry[i].cost)) {
                    dvt[0].dvEntry[i].cost = nbrcosttable_getcost(nct, pkt->header.src_nodeID) + pktRouteUpdt->entry[i].cost;
//                    puts("changing routingtable");
                    routingtable_setnextnode(routingtable, dvt[0].dvEntry[i].desNodeID, pkt->header.src_nodeID);
                }
            }
            pthread_mutex_unlock(dv_mutex);
            pthread_mutex_unlock(routingtable_mutex);
        }
    }
    
    close(overlay_conn);
    overlay_conn = -1;
    free(pkt);
    free(seg);
    free(pktRouteUpdt);
    pthread_exit(NULL);
}

//This function stops the SNP process.
//It closes all the connections and frees all the dynamically allocated memory.
//It is called when the SNP process receives a signal SIGINT.
void network_stop() {
	//put your code here
    close(overlay_conn);
    close(transport_conn);
    
    nbrcosttable_destroy(nct);
    dvtable_destroy(dvt);
    routingtable_destroy(routingtable);
    
    puts("In network_stop, about to exit!");
    exit(0);
}

//This function opens a port on NETWORK_PORT and waits for the TCP connection from local SRT process.
//After the local SRT process is connected, this function keeps receiving sendseg_arg_ts which contains the segments and their destination node addresses from the SRT process. The received segments are then encapsulated into packets (one segment in one packet), and sent to the next hop using overlay_sendpkt. The next hop is retrieved from routing table.
//When a local SRT process is disconnected, this function waits for the next SRT process to connect.
void waitTransport() {
	//put your code here
    int listenfd;
    struct sockaddr_in cliaddr, servaddr;
    socklen_t clilen;
    
    if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
        perror("\nProblem in creating the listening socket");
        return;
    }
    
    memset(&servaddr, 0, sizeof(servaddr));
    //preparation of the socket address
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(NETWORK_PORT);
    //bind the socket
    if(bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("\nProblem in binding the socket");
        return;
    }
    
    //listen to the socket by creating a connection queue, then wait for connections
    if (listen(listenfd, 1) < 0) {
        perror("\nProblem in listening the socket");
        return;
    }

    while (1) {
        clilen = sizeof(cliaddr);
        puts("Waiting for new local SRT connection!");
        transport_conn = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        puts("connected to local SRT!");
        
        seg_t *seg = (seg_t *)malloc(sizeof(seg_t));
//        seg_t seg;
        int *dest_nodeID = (int *)malloc(sizeof(int));
        
        snp_pkt_t *pkt = (snp_pkt_t *)malloc(sizeof(snp_pkt_t));
        memset(pkt, 0, sizeof(snp_pkt_t));
        pkt->header.src_nodeID = topology_getMyNodeID();
        
        puts("&&&&&&&");
        routingtable_print(routingtable);
        puts("&&&&&&&");
        
        int temp;
        while (getsegToSend(transport_conn, dest_nodeID, seg) > 0) {
        
            puts("*******");
            routingtable_print(routingtable);
            puts("*******");
            
            pkt->header.dest_nodeID = *dest_nodeID;
            pkt->header.length = sizeof(srt_hdr_t) + seg->header.length;
            pkt->header.type = SNP;
            memcpy(pkt->data, seg, pkt->header.length);
            
            pthread_mutex_lock(routingtable_mutex);
            temp = routingtable_getnextnode(routingtable, *dest_nodeID);
            printf("dest node ID is: %d, next hop is: %d\n", *dest_nodeID, temp);
            overlay_sendpkt(temp, pkt, overlay_conn);
            pthread_mutex_unlock(routingtable_mutex);
            puts("sent a seg in snp");
        }
        
        close(transport_conn);
        transport_conn = -1;
        free(seg);
        free(dest_nodeID);
    }
    
    return;
}

int main(int argc, char *argv[]) {
	printf("network layer is starting, pls wait...\n");
    
	//initialize global variables
	nct = nbrcosttable_create();
	dvt = dvtable_create();
	dv_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(dv_mutex,NULL);
	routingtable = routingtable_create();
	routingtable_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(routingtable_mutex,NULL);
	overlay_conn = -1;
	transport_conn = -1;
    
	nbrcosttable_print(nct);
	dvtable_print(dvt);
	routingtable_print(routingtable);
    
	//register a signal handler which is used to terminate the process
	signal(SIGINT, network_stop);
    
	//connect to local ON process
	overlay_conn = connectToOverlay();
	if(overlay_conn<0) {
		printf("can't connect to overlay process\n");
		exit(1);
	}
	
	//start a thread that handles incoming packets from ON process
	pthread_t pkt_handler_thread;
	pthread_create(&pkt_handler_thread,NULL,pkthandler,(void*)0);
    
	//start a route update thread
	pthread_t routeupdate_thread;
	pthread_create(&routeupdate_thread,NULL,routeupdate_daemon,(void*)0);
    
	printf("network layer is started...\n");
	printf("waiting for routes to be established\n");
	sleep(NETWORK_WAITTIME);
	routingtable_print(routingtable);
    
	//wait connection from SRT process
	printf("waiting for connection from SRT process\n");
	waitTransport(); 
    
}


