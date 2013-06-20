//FILE: overlay/neighbortable.c
//
//Description: this file the API for the neighbor table
//
//Date: May 03, 2010

#include "neighbortable.h"

//This function first creates a neighbor table dynamically. It then parses the topology/topology.dat file and fill the nodeID and nodeIP fields in all the entries, initialize conn field as -1 .
//return the created neighbor table
nbr_entry_t* nt_create()
{
    nbr_entry_t *nbr_table;
    char hostName[100];
    char *nbrName[20];
    char *tempName;
    char lineBuf[200];
    int nbrNum = 0;
    struct hostent *hostInfo;
    int i = 0;
    FILE *fp;
    
    if ((fp = openDat()) == NULL) {
        puts("Fail to open topology.dat!");
        return NULL;
    }
    
    memset(nbrName, 0, sizeof(nbrName));
    if (gethostname(hostName, sizeof(hostName)) == 0) {
        while (fgets(lineBuf, 200, fp)) {
            tempName = strtok(lineBuf, " ");
            //            printf("%s\t%d\n", tempName, nbrNum);
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
    printf("the nbrNum is %d\n", nbrNum);
    
    nbr_table = (nbr_entry_t *)malloc(nbrNum * sizeof(nbr_entry_t));
    memset(nbr_table, 0, nbrNum * sizeof(nbr_entry_t));
    
    //initialize
    for (i = 0; i < nbrNum; i++) {
        nbr_table[i].nodeID = topology_getNodeIDfromname(nbrName[i]);
        hostInfo = gethostbyname(nbrName[i]);
        memcpy((char *) &nbr_table[i].nodeIP, hostInfo->h_addr_list[0], hostInfo->h_length);
        nbr_table[i].conn = -1;
        free(nbrName[i]);
    }
    
    return nbr_table;
}

//This function destroys a neighbortable. It closes all the connections and frees all the dynamically allocated memory.
void nt_destroy(nbr_entry_t* nt)
{
    int i;
    
    for (i = 0; i < topology_getNbrNum(); i++) {
        close(nt[i].conn);
    }
    free(nt);
    
    return;
}

//This function is used to assign a TCP connection to a neighbor table entry for a neighboring node. If the TCP connection is successfully assigned, return 1, otherwise return -1
int nt_addconn(nbr_entry_t* nt, int nodeID, int conn)
{
    int i;
    
    for (i = 0; i < topology_getNbrNum(); i++) {
        if (nt[i].nodeID == nodeID) {
            nt[i].conn = conn;
            return 1;
        }
    }
    
    return -1;
}
