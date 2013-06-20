//FILE: overlay/overlay.c
//
//Description: this file implements a ON process 
//A ON process first connects to all the neighbors and then starts listen_to_neighbor threads each of which keeps receiving the incoming packets from a neighbor and forwarding the received packets to the SNP process. Then ON process waits for the connection from SNP process. After a SNP process is connected, the ON process keeps receiving sendpkt_arg_t structures from the SNP process and sending the received packets out to the overlay network. 
//
//Date: April 28,2008

#include "overlay.h"

//you should start the ON processes on all the overlay hosts within this period of time
#define OVERLAY_START_DELAY 20

/**************************************************************/
//declare global variables
/**************************************************************/

//declare the neighbor table as global variable 
nbr_entry_t* nt; 
//declare the TCP connection to SNP process as global variable
int network_conn; 


/**************************************************************/
//implementation overlay functions
/**************************************************************/

// This thread opens a TCP port on CONNECTION_PORT and waits for the incoming connection from all the neighbors that have a larger node ID than my nodeID,
// After all the incoming connections are established, this thread terminates 
void* waitNbrs(void* arg) {
	//put your code here
    int listenfd, connfd;
    struct sockaddr_in cliaddr, servaddr;
    socklen_t clilen;
    int i;
    int myID;
    
    if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
        perror("\nProblem in creating the listening socket");
        return -1;
    }
    
    memset(&servaddr, 0, sizeof(servaddr));
    //preparation of the socket address
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(CONNECTION_PORT);
    //bind the socket
    if(bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("\nProblem in binding the socket");
        return -1;
    }
    
    //listen to the socket by creating a connection queue, then wait for connections
    if (listen(listenfd, MAX_NODE_NUM) < 0) {
        perror("\nProblem in listening the socket");
        return -1;
    }
    
    myID = topology_getMyNodeID();
    for (i = 0; i < topology_getNbrNum(); i++) {
        if (nt[i].nodeID > myID) {
            clilen = sizeof(cliaddr);
            //accept a connection
            printf("waiting for connection in wairNbr\n");
            connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
            
            nt_addconn(nt, topology_getNodeIDfromip(&cliaddr.sin_addr), connfd);
            printf("waitNbrs add %d on %d\n", nt[i].nodeID, connfd);
        }
    }
    
    return 0;
}

// This function connects to all the neighbors that have a smaller node ID than my nodeID
// After all the outgoing connections are established, return 1, otherwise return -1
int connectNbrs() {
	//put your code here
    struct sockaddr_in servaddr;
    int out_conn;
    int i;
    int myID;
    char *str;
    
    myID = topology_getMyNodeID();
    for (i = 0; i < topology_getNbrNum(); i++) {
        if (nt[i].nodeID < myID) {
            if ((out_conn = socket (AF_INET, SOCK_STREAM, 0)) <0) {
                perror("\nProblem in creating the socket in connectNbrs");
                return -1;
            }
            
            memset(&servaddr, 0, sizeof(servaddr));
            servaddr.sin_family = AF_INET;
            servaddr.sin_addr.s_addr= nt[i].nodeIP;
            servaddr.sin_port = htons(CONNECTION_PORT);
            
            str = inet_ntoa(*(struct in_addr*)&nt[i].nodeIP);
            puts(str);
            
            if (connect(out_conn, (struct sockaddr *) &servaddr, sizeof(servaddr))<0) {
                perror("\nProblem in connecting to the server in connectNbrs");
                return -1;
            }
            nt_addconn(nt, nt[i].nodeID, out_conn);
            printf("connectNbrs add %d on %d\n", nt[i].nodeID, out_conn);
        }
    }
    
    return 1;
}

//Each listen_to_neighbor thread keeps receiving packets from a neighbor. It handles the received packets by forwarding the packets to the SNP process.
//all listen_to_neighbor threads are started after all the TCP connections to the neighbors are established
void* listen_to_neighbor(void* arg) {
	//put your code here
    snp_pkt_t *pkt = (snp_pkt_t *)malloc(sizeof(snp_pkt_t));
    int *idx = (int *)arg;
    
    while (recvpkt(pkt, nt[*idx].conn) > 0) {
//        printf("overlay received pkt from %d, neighbor %d, neighbor idx %d, conn %d\n", pkt->header.src_nodeID, nt[*idx].nodeID, *idx, nt[*idx].conn);
        forwardpktToSNP(pkt, network_conn);
    }
    
    close(nt[*idx].conn);
    nt[*idx].conn = -1;
    printf("In listen_to_neighbor, one neighbor closed!");
    free(pkt);
    pthread_exit(NULL);
}

//This function opens a TCP port on OVERLAY_PORT, and waits for the incoming connection from local SNP process. After the local SNP process is connected, this function keeps getting sendpkt_arg_ts from SNP process, and sends the packets to the next hop in the overlay network. If the next hop's nodeID is BROADCAST_NODEID, the packet should be sent to all the neighboring nodes.
void waitNetwork() {
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
    servaddr.sin_port = htons(OVERLAY_PORT);
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
    
    clilen = sizeof(cliaddr);
    network_conn = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
    puts("connected to local SNP!");
    
    snp_pkt_t *pkt = (snp_pkt_t *)malloc(sizeof(snp_pkt_t));
    int *nextNode = (int *)malloc(sizeof(int));
    int i;
    int nbrNum = topology_getNbrNum();
    
    while (getpktToSend(pkt, nextNode, network_conn) > 0) {
//        printf("got a pkt from above, nextNode is %d\n", *nextNode);
        if (*nextNode == BROADCAST_NODEID) {
            //            puts("received a broadcast, about to send out");
            for (i = 0; i < nbrNum; i++) {
//                printf("broadcast to %d\n", nt[i].nodeID);
                sendpkt(pkt, nt[i].conn);
            }
        }
        else {
//            for (i = 0; i < nbrNum; i++) {
//                printf("%d, %d\t", i, nt[i].conn);
//            }
            printf("\n");
            for (i = 0; i < nbrNum; i++) {
                if (nt[i].nodeID == *nextNode) {
//                    printf("send to %d, conn %d", nt[i].nodeID, nt[i].conn);
                    sendpkt(pkt, nt[i].conn);
                    break;
                }
            }
        }
    }
    
    printf("In waitNetwork, about to exit!");
    close(network_conn);
    free(pkt);
    free(nextNode);
}

//this function stops the overlay
//it closes all the connections and frees all the dynamically allocated memory
//it is called when receiving a signal SIGINT
void overlay_stop() {
	//put your code here
    close(network_conn);
    nt_destroy(nt);
    printf("In overlay_stop, about to exit!");
    exit(0);
}

int main() {
	//start overlay initialization
	printf("Overlay: Node %d initializing...\n",topology_getMyNodeID());	

	//create a neighbor table
	nt = nt_create();
	//initialize network_conn to -1, means no SNP process is connected yet
	network_conn = -1;
	
	//register a signal handler which is sued to terminate the process
	signal(SIGINT, overlay_stop);

	//print out all the neighbors
	int nbrNum = topology_getNbrNum();
	int i;
	for(i=0;i<nbrNum;i++) {
		printf("Overlay: neighbor %d:%d\n",i+1,nt[i].nodeID);
	}

	//start the waitNbrs thread to wait for incoming connections from neighbors with larger node IDs
	pthread_t waitNbrs_thread;
	pthread_create(&waitNbrs_thread,NULL,waitNbrs,(void*)0);

	//wait for other nodes to start
	sleep(OVERLAY_START_DELAY);
	
	//connect to neighbors with smaller node IDs
	connectNbrs();

	//wait for waitNbrs thread to return
	pthread_join(waitNbrs_thread,NULL);	

	//at this point, all connections to the neighbors are created
	
	//create threads listening to all the neighbors
	for(i=0;i<nbrNum;i++) {
		int* idx = (int*)malloc(sizeof(int));
		*idx = i;
		pthread_t nbr_listen_thread;
		pthread_create(&nbr_listen_thread,NULL,listen_to_neighbor,(void*)idx);
	}
	printf("Overlay: node initialized...\n");
	printf("Overlay: waiting for connection from SNP process...\n");

	//waiting for connection from  SNP process
	waitNetwork();
}
