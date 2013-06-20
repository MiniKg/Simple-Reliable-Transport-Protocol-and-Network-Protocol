
#include "nbrcosttable.h"

//This function creates a neighbor cost table dynamically
//and initialize the table with all its neighbors' node IDs and direct link costs.
//The neighbors' node IDs and direct link costs are retrieved from topology.dat file.
nbr_cost_entry_t* nbrcosttable_create()
{
    int nbrNum = topology_getNbrNum();
    int myNodeID = topology_getMyNodeID();
    int *myNbrArray = topology_getNbrArray();
    int i;
    nbr_cost_entry_t *nbrCostTable = (nbr_cost_entry_t *)malloc(nbrNum * sizeof(nbr_cost_entry_t));
    
    //initialize the neighbor cost table
    for (i = 0; i < nbrNum; i++) {
        nbrCostTable[i].nodeID = myNbrArray[i];
        nbrCostTable[i].cost = topology_getCost(myNodeID, myNbrArray[i]);
    }
    
    free(myNbrArray);
    return nbrCostTable;
}

//This function destroys a neighbor cost table.
//It frees all the dynamically allocated memory for the neighbor cost table.
void nbrcosttable_destroy(nbr_cost_entry_t* nct)
{
    free(nct);
    
    return;
}

//This function is used to get the direct link cost from neighbor.
//The direct link cost is returned if the neighbor is found in the table.
//INFINITE_COST is returned if the node is not found in the table.
unsigned int nbrcosttable_getcost(nbr_cost_entry_t* nct, int nodeID)
{
    int nbrNum = topology_getNbrNum();
    int i;
    
    for (i = 0; i < nbrNum; i++) {
        if (nct[i].nodeID == nodeID) {
            return nct[i].cost;
        }
    }
    
    return INFINITE_COST;
}

//This function prints out the contents of a neighbor cost table.
void nbrcosttable_print(nbr_cost_entry_t* nct)
{
    int nbrNum = topology_getNbrNum();
    int i;
    
    printf("The neighbor cost table of %d is:\n", topology_getMyNodeID());
    
    for (i = 0; i < nbrNum; i++) {
        printf("Neighbor nodeID: %d\tcost: %d\n", nct[i].nodeID, nct[i].cost);
    }
    
    return;
}
