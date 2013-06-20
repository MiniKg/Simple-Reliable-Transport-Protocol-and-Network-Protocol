

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../common/constants.h"
#include "../topology/topology.h"
#include "routingtable.h"

//This is the hash function used the by the routing table
//It takes the hash key - destination node ID as input,
//and returns the hash value - slot number for this destination node ID.
//
//You can copy makehash() implementation below directly to routingtable.c:
//int makehash(int node) {
//	return node%MAX_ROUTINGTABLE_ENTRIES;
//}
//
int makehash(int node)
{
    return node % MAX_ROUTINGTABLE_SLOTS;
}

//This function creates a routing table dynamically.
//All the entries in the table are initialized to NULL pointers.
//Then for all the neighbors with a direct link, create a routing entry using the neighbor itself as the next hop node, and insert this routing entry into the routing table.
//The dynamically created routing table structure is returned.
routingtable_t* routingtable_create()
{
    routingtable_t *routingTable = (routingtable_t *)malloc(sizeof(routingtable_t));
    int nbrNum = topology_getNbrNum();
    int *nbrArray = topology_getNbrArray();
    int i;
    
    for (i = 0; i < MAX_ROUTINGTABLE_SLOTS; i++) {
        routingTable->hash[i] = NULL;
    }
    
    for (i = 0; i < nbrNum; i++) {
        routingtable_setnextnode(routingTable, nbrArray[i], nbrArray[i]);
    }
    
    free(nbrArray);
    return routingTable;
}

//This funtion destroys a routing table.
//All dynamically allocated data structures for this routing table are freed.
void routingtable_destroy(routingtable_t* routingtable)
{
    int i;
    routingtable_entry_t *temp;
    puts("in destroy!!!!!!");
    for (i = 0; i < MAX_ROUTINGTABLE_SLOTS; i++) {
        puts("before while");
        while (routingtable->hash[i] != NULL) {
            puts("in while");
            temp = routingtable->hash[i];
            routingtable->hash[i] = routingtable->hash[i]->next;
            printf("freed %d\n", i);
            free(temp);
        }
    }
    puts("about to free!!!!!!");
    free(routingtable);
    
    return;
}

//This function updates the routing table using the given destination node ID and next hop's node ID.
//If the routing entry for the given destination already exists, update the existing routing entry.
//If the routing entry of the given destination is not there, add one with the given next node ID.
//Each slot in routing table contains a linked list of routing entries due to conflicting hash keys (differnt hash keys (destination node ID here) may have same hash values (slot entry number here)).
//To add an routing entry to the hash table:
//First use the hash function makehash() to get the slot number in which this routing entry should be stored.
//Then append the routing entry to the linked list in that slot.
void routingtable_setnextnode(routingtable_t* routingtable, int destNodeID, int nextNodeID)
{
    int slotNum = makehash(destNodeID);
    routingtable_entry_t *slot = routingtable->hash[slotNum];
    
    if (slot != NULL) {
        routingtable_entry_t *before = slot;
        while (slot != NULL && slot->destNodeID != destNodeID) {
            before = slot;
            slot = slot->next;
        }
        
        //append
        if (slot == NULL) {
            slot = (routingtable_entry_t *)malloc(sizeof(routingtable_entry_t));
            slot->destNodeID = destNodeID;
            slot->nextNodeID = nextNodeID;
            slot->next = NULL;
            before->next = slot;
        }
        //update
        else if (slot != NULL && slot->destNodeID == destNodeID) {
            slot->nextNodeID = nextNodeID;
//            puts("changed!!");
        }
    }
    else {
        slot = (routingtable_entry_t *)malloc(sizeof(routingtable_entry_t));
        slot->destNodeID = destNodeID;
        slot->nextNodeID = nextNodeID;
        slot->next = NULL;
        routingtable->hash[slotNum] = slot;
//        puts("changed!!");
    }
    
    return;
}

//This function looks up the destNodeID in the routing table.
//Since routing table is a hash table, this opeartion has O(1) time complexity.
//To find a routing entry for a destination node, you should first use the hash function makehash() to get the slot number and then go through the linked list in that slot to search the routing entry.
//If the destNodeID is found, return the nextNodeID for this destination node.
//If the destNodeID is not found, return -1.
int routingtable_getnextnode(routingtable_t* routingtable, int destNodeID)
{
    int slotNum = makehash(destNodeID);
    routingtable_entry_t *slot;
    slot = routingtable->hash[slotNum];
//    if (destNodeID == 32) {
//        slot->destNodeID = 32;
//        slot->nextNodeID = 32;
//    }
//    
////    printf("slot->destNodeID is %d\n", slot->destNodeID);
    
    while (slot != NULL) {
//        printf("dest node ID is %d, slot->destNodeID is %d\n", destNodeID, slot->destNodeID);
        if (slot->destNodeID == destNodeID) {
            return slot->nextNodeID;
        }
        slot = slot->next;
    }
    
    return -1;
}

//This function prints out the contents of the routing table
void routingtable_print(routingtable_t* routingtable)
{
    int i;
    routingtable_entry_t *slot;
    
    printf("The routing table of %d is:\n", topology_getMyNodeID());
    for (i = 0; i < MAX_ROUTINGTABLE_SLOTS; i++) {
        slot = routingtable->hash[i];
        while (slot != NULL) {
            printf("Dest node is %d, next hop is %d\n", slot->destNodeID, slot->nextNodeID);
            slot = slot->next;
        }
    }
    
    return;
}
