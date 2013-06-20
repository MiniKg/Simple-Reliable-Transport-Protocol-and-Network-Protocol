
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../common/constants.h"
#include "../topology/topology.h"
#include "dvtable.h"

//This function creates a dvtable(distance vector table) dynamically.
//A distance vector table contains the n+1 entries, where n is the number of the neighbors of this node, and the rest one is for this node itself.
//Each entry in distance vector table is a dv_t structure which contains a source node ID and an array of N dv_entry_t structures where N is the number of all the nodes in the overlay.
//Each dv_entry_t contains a destination node address the the cost from the source node to this destination node.
//The dvtable is initialized in this function.
//The link costs from this node to its neighbors are initialized using direct link cost retrived from topology.dat.
//Other link costs are initialized to INFINITE_COST.
//The dynamically created dvtable is returned.
dv_t* dvtable_create()
{
    int nbrNum = topology_getNbrNum();
    int nodeNum = topology_getNodeNum();
    int *nbrArray = topology_getNbrArray();
    int *nodeArray = topology_getNodeArray();
    int i, j;
    dv_t *dvt = (dv_t *)malloc((nbrNum+1) * sizeof(dv_t));
    
    for (i = 0; i < (nbrNum+1); i++) {
        dvt[i].dvEntry = (dv_entry_t *)malloc(nodeNum * sizeof(dv_entry_t));
        if (i == 0) {           //first entry is the node itself
            dvt[i].srcNodeID = topology_getMyNodeID();
            for (j = 0; j < nodeNum; j++) {
                dvt[i].dvEntry[j].desNodeID = nodeArray[j];
                dvt[i].dvEntry[j].cost = topology_getCost(dvt[i].srcNodeID, dvt[i].dvEntry[j].desNodeID);
            }
        }
        else {
            dvt[i].srcNodeID = nbrArray[i-1];
            for (j = 0; j < nodeNum; j++) {
                dvt[i].dvEntry[j].desNodeID = nodeArray[j];
                dvt[i].dvEntry[j].cost = INFINITE_COST;
            }
        }
    }
    
    free(nbrArray);
    free(nodeArray);
    return dvt;
}

//This function destroys a dvtable.
//It frees all the dynamically allocated memory for the dvtable.
void dvtable_destroy(dv_t* dvtable)
{
    int nbrNum = topology_getNbrNum();
    int i;
    
    for (i = 0; i < (nbrNum+1); i++) {
        free(dvtable[i].dvEntry);
    }
    free(dvtable);
    
    return;
}

//This function sets the link cost between two nodes in dvtable.
//If those two nodes are found in the table and the link cost is set, return 1.
//Otherwise, return -1.
int dvtable_setcost(dv_t* dvtable,int fromNodeID,int toNodeID, unsigned int cost)
{
    int nbrNum = topology_getNbrNum();
    int nodeNum = topology_getNodeNum();
    int i, j;
    
    for (i = 0; i < (nbrNum+1); i++) {
        if (dvtable[i].srcNodeID == fromNodeID) {
            for (j = 0; j < nodeNum; j++) {
                if (dvtable[i].dvEntry[j].desNodeID == toNodeID) {
                    dvtable[i].dvEntry[j].cost = cost;
                    return 1;
                }
            }
        }
    }
    
    return -1;
}

//This function returns the link cost between two nodes in dvtable
//If those two nodes are found in dvtable, return the link cost.
//otherwise, return INFINITE_COST.
unsigned int dvtable_getcost(dv_t* dvtable, int fromNodeID, int toNodeID)
{
    int nbrNum = topology_getNbrNum();
    int nodeNum = topology_getNodeNum();
    int i, j;
    
    for (i = 0; i < (nbrNum+1); i++) {
        if (dvtable[i].srcNodeID == fromNodeID) {
            for (j = 0; j < nodeNum; j++) {
                if (dvtable[i].dvEntry[j].desNodeID == toNodeID) {
                    return dvtable[i].dvEntry[j].cost;
                }
            }
        }
    }
    
    return INFINITE_COST;
}

//This function prints out the contents of a dvtable.
void dvtable_print(dv_t* dvtable)
{
    int nbrNum = topology_getNbrNum();
    int nodeNum = topology_getNodeNum();
    int i, j;
    
    printf("The distance vector table of %d is:\n", topology_getMyNodeID());
    
    for (i = 0; i < (nbrNum+1); i++) {
        for (j = 0; j < nodeNum; j++) {
            printf("From %d to %d: %d\n", dvtable[i].srcNodeID, dvtable[i].dvEntry[j].desNodeID, dvtable[i].dvEntry[j].cost);
        }
        printf("\n");
    }
    
    return;
}
