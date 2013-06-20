//FILE: topology/topology.c
//
//Description: this file implements some helper functions used to parse
//the topology file
//
//Date: May 3,2010

#include "topology.h"

//this function returns node ID of the given hostname
//the node ID is an integer of the last 8 digit of the node's IP address
//for example, a node with IP address 202.120.92.3 will have node ID 3
//if the node ID can't be retrieved, return -1
int topology_getNodeIDfromname(char* hostname)
{
    struct hostent *hostInfo;
    char *ptr;
    char str[32];
    int nodeID;
    
	hostInfo = gethostbyname(hostname);
	if(!hostInfo) {
		printf("host name error!\n");
		return -1;
	}
    
    //move the pointer to the last byte of the address
    ptr = hostInfo->h_addr_list[0] + 3;
    inet_ntop(hostInfo->h_addrtype, ptr, str, sizeof(str));
    nodeID = atoi(str);
    
    return nodeID;
}

//this function returns node ID from the given IP address
//if the node ID can't be retrieved, return -1
int topology_getNodeIDfromip(struct in_addr* addr)
{
    int nodeID;
    char *str;
    int i = 0;
    
    if ((str = inet_ntoa(*addr)) == NULL) {
        return -1;
    }
    
    while (i < 3) {
        if (*str == '.') {
            i++;
        }
        str++;
    }
    nodeID = atoi(str);
    
    return nodeID;
}

//this function returns my node ID
//if my node ID can't be retrieved, return -1
int topology_getMyNodeID()
{
    char hostName[50];
    struct hostent *hostInfo;
    char *str;
    int nodeID;
    int i = 0;
    
    if (gethostname(hostName, sizeof(hostName)) == 0) {
        hostInfo = gethostbyname(hostName);
        str = inet_ntoa(*((struct in_addr *)hostInfo->h_addr_list[0]));
        while (i < 3) {
            if (*str == '.') {
                i++;
            }
            str++;
        }
        nodeID = atoi(str);
        
        return nodeID;
    }
    
    return -1;
}

//this functions parses the topology information stored in topology.dat
//returns the number of neighbors
int topology_getNbrNum()
{
    char hostName[50];
    char lineBuf[100];
    int nbrNum = 0;
    FILE *fp;
    
    if ((fp = openDat()) == NULL) {
        return -1;
    }
    
    if (gethostname(hostName, sizeof(hostName)) == 0) {
        while (fgets(lineBuf, 100, fp)) {
            if (strcmp(strtok(lineBuf, " "), hostName) == 0) {
                nbrNum++;
                continue;
            }
            else if (strcmp(strtok(NULL, " "), hostName) == 0) {
                nbrNum++;
            }
        }
    }
    
    return nbrNum;
}

//this functions parses the topology information stored in topology.dat
//returns the number of total nodes in the overlay
int topology_getNodeNum()
{
    char *hostNames[20];
    char *tempHost;
    char lineBuf[100];
    int nodeNum = 0;
    int i;
    int foundMatch = 0;
    FILE *fp;
    
    if ((fp = openDat()) == NULL) {
        return -1;
    }
    
    memset(hostNames, 0, sizeof(hostNames));
    while (fgets(lineBuf, 100, fp)) {
        //first host name
        tempHost = strtok(lineBuf, " ");
        foundMatch = 0;
        if (hostNames == NULL) {
            hostNames[0] = (char *)malloc(50);
            strcpy(hostNames[0], tempHost);
            nodeNum++;
        }
        else {
            for (i = 0; hostNames[i] != NULL && i < 20; i++) {
                if (strcmp(hostNames[i], tempHost) == 0) {
                    foundMatch = 1;
                    break;
                }
            }
            if (foundMatch == 0) {
                hostNames[i] = (char *)malloc(50);
                strcpy(hostNames[i], tempHost);
                nodeNum++;
            }
        }
        
        //second host name
        tempHost = strtok(NULL, " ");
        foundMatch = 0;
        for (i = 0; hostNames[i] != NULL && i < 20; i++) {
            if (strcmp(hostNames[i], tempHost) == 0) {
                foundMatch = 1;
                break;
            }
        }
        if (foundMatch == 0) {
            hostNames[i] = (char *)malloc(50);
            strcpy(hostNames[i], tempHost);
            nodeNum++;
        }
    }
    
    for (i = 0; i < nodeNum; i++) {
        free(hostNames[i]);
    }
    
    return nodeNum;
}

//this functions parses the topology information stored in topology.dat
//returns a dynamically allocated array which contains all the nodes' IDs in the overlay network
int* topology_getNodeArray()
{
    char *hostNames[20];
    char *tempHost;
    char lineBuf[100];
    int nodeNum = 0;
    int i;
    int foundMatch = 0;
    FILE *fp;
    
    if ((fp = openDat()) == NULL) {
        return NULL;
    }
    
    memset(hostNames, 0, sizeof(hostNames));
    while (fgets(lineBuf, 100, fp)) {
        //first host name
        tempHost = strtok(lineBuf, " ");
        foundMatch = 0;
        if (hostNames == NULL) {
            hostNames[0] = (char *)malloc(50);
            strcpy(hostNames[0], tempHost);
            nodeNum++;
        }
        else {
            for (i = 0; hostNames[i] != NULL && i < 20; i++) {
                if (strcmp(hostNames[i], tempHost) == 0) {
                    foundMatch = 1;
                    break;
                }
            }
            if (foundMatch == 0) {
                hostNames[i] = (char *)malloc(50);
                strcpy(hostNames[i], tempHost);
                nodeNum++;
            }
        }
        
        //second host name
        tempHost = strtok(NULL, " ");
        foundMatch = 0;
        for (i = 0; hostNames[i] != NULL && i < 20; i++) {
            if (strcmp(hostNames[i], tempHost) == 0) {
                foundMatch = 1;
                break;
            }
        }
        if (foundMatch == 0) {
            hostNames[i] = (char *)malloc(50);
            strcpy(hostNames[i], tempHost);
            nodeNum++;
        }
    }
    
    int *nodeIdArray = (int *)malloc(nodeNum * sizeof(int));
    
    for (i = 0; i < nodeNum; i++) {
        nodeIdArray[i] = topology_getNodeIDfromname(hostNames[i]);
        free(hostNames[i]);
    }
    
    return nodeIdArray;
}

//this functions parses the topology information stored in topology.dat
//returns a dynamically allocated array which contains all the neighbors'IDs
int* topology_getNbrArray()
{
    char hostName[50];
    char *nbrName[20];
    char *tempName;
    char lineBuf[100];
    int nbrNum = 0;
    int i = 0;
    FILE *fp;
    
    if ((fp = openDat()) == NULL) {
        return NULL;
    }
    
    memset(nbrName, 0, sizeof(nbrName));
    if (gethostname(hostName, sizeof(hostName)) == 0) {
        while (fgets(lineBuf, 100, fp)) {
            tempName = strtok(lineBuf, " ");
            if (strcmp(tempName, hostName) == 0) {
                nbrName[i] = (char *)malloc(50);
                strcpy(nbrName[i++], strtok(NULL, " "));
                //                i++;
                nbrNum++;
                continue;
            }
            else if (strcmp(strtok(NULL, " "), hostName) == 0) {
                nbrName[i] = (char *)malloc(50);
                strcpy(nbrName[i++], tempName);
                //                i++;
                nbrNum++;
            }
        }
    }
    
    int *nbrIdArray = (int *)malloc(nbrNum * sizeof(int));
    for (i = 0; i < nbrNum; i++) {
        nbrIdArray[i] = topology_getNodeIDfromname(nbrName[i]);
        free(nbrName[i]);
    }
    
    return nbrIdArray;
}

//this functions parses the topology information stored in topology.dat
//returns the cost of the direct link between the two given nodes
//if no direct link between the two given nodes, INFINITE_COST is returned
unsigned int topology_getCost(int fromNodeID, int toNodeID)
{
    char lineBuf[100];
    char *host1, *host2, *charCost;
    int nodeID1, nodeID2, cost;
    FILE *fp;
    
    if ((fp = openDat()) == NULL) {
        return NULL;
    }
    
    if (fromNodeID == toNodeID) {
        return 0;
    }
    
    while (fgets(lineBuf, 100, fp)) {
        host1 = strtok(lineBuf, " ");
        host2 = strtok(NULL, " ");
        nodeID1 = topology_getNodeIDfromname(host1);
        nodeID2 = topology_getNodeIDfromname(host2);
        
        if ((nodeID1 == fromNodeID && nodeID2 == toNodeID) || (nodeID1 == toNodeID && nodeID2 == fromNodeID)) {
            charCost = strtok(NULL, " ");
            cost = atoi(charCost);
            return cost;
        }
    }
    
    return INFINITE_COST;
}

FILE *openDat() {
    FILE *fp;
    if ((fp = fopen("./topology.dat", "r")) != NULL) {
        return fp;
    }
    puts("Fail to open topology.dat!!!");
    return NULL;
}
