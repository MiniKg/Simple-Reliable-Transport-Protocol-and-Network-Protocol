
#include "seg.h"


//SRT process uses this function to send a segment and its destination node ID in a sendseg_arg_t structure to SNP process to send out.
//Parameter network_conn is the TCP descriptor of the connection between the SRT process and the SNP process.
//Return 1 if a sendseg_arg_t is succefully sent, otherwise return -1.
int snp_sendseg(int network_conn, int dest_nodeID, seg_t* segPtr)
{
    segPtr->header.checksum = checksum(segPtr);
    sendseg_arg_t *sendSegArg = (sendseg_arg_t *)malloc(sizeof(sendseg_arg_t));
    sendSegArg->nodeID = dest_nodeID;
    sendSegArg->seg = *segPtr;
    
    char bufstart[2];
	char bufend[2];
    int segSize;
    
	bufstart[0] = '!';
	bufstart[1] = '&';
	bufend[0] = '!';
	bufend[1] = '#';
    
	if (send(network_conn, bufstart, 2, 0) < 0) {
        free(sendSegArg);
		return -1;
	}
    
    segSize = sizeof(int) + sizeof(srt_hdr_t) + segPtr->header.length;     //??????
//    printf("^^^^^^^^^^in snp_sendseg, segSize is %d^^^^^^^^^^\n", segSize);
	if(send(network_conn, sendSegArg, segSize, 0) < 0) {
        free(sendSegArg);
		return -1;
	}
    
	if(send(network_conn, bufend, 2, 0) < 0) {
        free(sendSegArg);
		return -1;
	}
    
    free(sendSegArg);
    
	return 1;
}

//SRT process uses this function to receive a  sendseg_arg_t structure which contains a segment and its src node ID from the SNP process.
//Parameter network_conn is the TCP descriptor of the connection between the SRT process and the SNP process.
//When a segment is received, use seglost to determine if the segment should be discarded, also check the checksum.
//Return 1 if a sendseg_arg_t is succefully received, otherwise return -1.
int snp_recvseg(int network_conn, int* src_nodeID, seg_t* segPtr)
{
    char buf[sizeof(sendseg_arg_t)+2];
	char c;
	int idx = 0;
	// state can be 0,1,2,3;
	// 0 starting point
	// 1 '!' received
	// 2 '&' received, start receiving segment
	// 3 '!' received,
	// 4 '#' received, finish receiving segment
	int state = 0;
    sendseg_arg_t *sendSegArg = (sendseg_arg_t *)malloc(sizeof(sendseg_arg_t));
    //    puts("in getsegToSend");
    
	while(recv(network_conn,&c,1,0)>0) {
		if (state == 0) {
            if(c=='!')
				state = 1;
		}
		else if(state == 1) {
			if(c=='&')
				state = 2;
			else
				state = 0;
		}
		else if(state == 2) {
			if(c=='!') {
				buf[idx]=c;
				idx++;
				state = 3;
			}
			else {
				buf[idx]=c;
				idx++;
			}
		}
		else if(state == 3) {
			if(c=='#') {
				buf[idx]=c;
				idx++;
                
                memcpy(sendSegArg, buf, idx - 2);
                *src_nodeID = sendSegArg->nodeID;
                //                printf("in getpktToSend, nextNodeID is %d\n", *nextNode);
                *segPtr = sendSegArg->seg;
                
				state = 0;
				idx = 0;
                
                if(seglost(segPtr)>0) {
                    printf("seg lost!\n");
                    continue;
                }
                
                if(checkchecksum(segPtr)<0) {
                    printf("checksum error, drop!\n");
                    continue;
                }
                
                free(sendSegArg);
				return 1;
			}
			else if(c=='!') {
				buf[idx]=c;
				idx++;
			}
			else {
				buf[idx]=c;
				idx++;
				state = 2;
			}
		}
	}
    
    free(sendSegArg);
    return -1;
}

//SNP process uses this function to receive a sendseg_arg_t structure which contains a segment and its destination node ID from the SRT process.
//Parameter tran_conn is the TCP descriptor of the connection between the SRT process and the SNP process.
//Return 1 if a sendseg_arg_t is succefully received, otherwise return -1.
int getsegToSend(int tran_conn, int* dest_nodeID, seg_t* segPtr)
{
    char buf[sizeof(sendseg_arg_t)+2];
	char c;
	int idx = 0;
	int state = 0;
    
    sendseg_arg_t *sendSegArg = (sendseg_arg_t *)malloc(sizeof(sendseg_arg_t));
    
    //    puts("in getsegToSend");
    
	while(recv(tran_conn,&c,1,0)>0) {
		if (state == 0) {
            if(c=='!')
				state = 1;
		}
		else if(state == 1) {
			if(c=='&')
				state = 2;
			else
				state = 0;
		}
		else if(state == 2) {
			if(c=='!') {
				buf[idx]=c;
				idx++;
				state = 3;
			}
			else {
				buf[idx]=c;
				idx++;
			}
		}
		else if(state == 3) {
			if(c=='#') {
				buf[idx]=c;
				idx++;

                memcpy(sendSegArg, buf, idx - 2);
//                printf("^^^^^^^^in getsegToSend, idx-2 is %d^^^^^^^^^^^\n" , idx-2);
                *dest_nodeID = sendSegArg->nodeID;
                //                printf("in getpktToSend, nextNodeID is %d\n", *nextNode);
                *segPtr = sendSegArg->seg;
                
				state = 0;
				idx = 0;
                free(sendSegArg);
				return 1;
			}
			else if(c=='!') {
				buf[idx]=c;
				idx++;
			}
			else {
				buf[idx]=c;
				idx++;
				state = 2;
			}
		}
	}
    
    free(sendSegArg);
    return -1;
}

//SNP process uses this function to send a sendseg_arg_t structure which contains a segment and its src node ID to the SRT process.
//Parameter tran_conn is the TCP descriptor of the connection between the SRT process and the SNP process.
//Return 1 if a sendseg_arg_t is succefully sent, otherwise return -1.
int forwardsegToSRT(int tran_conn, int src_nodeID, seg_t* segPtr)
{
    sendseg_arg_t *sendSegArg = (sendseg_arg_t *)malloc(sizeof(sendseg_arg_t));
    sendSegArg->nodeID = src_nodeID;
    sendSegArg->seg = *segPtr;
    
    char bufstart[2];
	char bufend[2];
    int segSize;
    
	bufstart[0] = '!';
	bufstart[1] = '&';
	bufend[0] = '!';
	bufend[1] = '#';
    
	if (send(tran_conn, bufstart, 2, 0) < 0) {
        free(sendSegArg);
		return -1;
	}
    
    segSize = sizeof(int) + sizeof(srt_hdr_t) + segPtr->header.length;     //??????
	if(send(tran_conn, sendSegArg, segSize, 0) < 0) {
        free(sendSegArg);
		return -1;
	}
    
	if(send(tran_conn, bufend, 2, 0) < 0) {
        free(sendSegArg);
		return -1;
	}
    
    free(sendSegArg);
    
	return 1;
}

// for seglost(seg_t* segment):
// a segment has PKT_LOST_RATE probability to be lost or invalid checksum
// with PKT_LOST_RATE/2 probability, the segment is lost, this function returns 1
// If the segment is not lost, return 0.
// Even the segment is not lost, the packet has PKT_LOST_RATE/2 probability to have invalid checksum
// We flip  a random bit in the segment to create invalid checksum
int seglost(seg_t* segPtr)
{
    int random = rand()%100;
	if(random<PKT_LOSS_RATE*100) {
		//50% probability of losing a segment
		if(rand()%2==0) {
			printf("seg lost!!!\n");
            return 1;
		}
		//50% chance of invalid checksum
		else {
			//get data length
			int len = sizeof(srt_hdr_t)+segPtr->header.length;
			//get a random bit that will be flipped
			int errorbit = rand()%(len*8);
			//flip the bit
			char* temp = (char*)segPtr;
			temp = temp + errorbit/8;
			*temp = *temp^(1<<(errorbit%8));
			return 0;
		}
	}
	return 0;
}
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//This function calculates checksum over the given segment.
//The checksum is calculated over the segment header and segment data.
//You should first clear the checksum field in segment header to be 0.
//If the data has odd number of octets, add an 0 octets to calculate checksum.
//Use 1s complement for checksum calculation.
unsigned short checksum(seg_t* segment)
{
    register long sum = 0;
    unsigned int count = 0;
    
    //to make the increment by 16 bits, do a type cast
    unsigned short* addr = (unsigned short*)segment;
    
    //clear the checksum field to 0
    segment->header.checksum = 0;
    
    //add a byte of 0 if odd number
	if(segment->header.length % 2 == 1) {
		segment->data[segment->header.length] = 0;
    }
	
	count = sizeof(srt_hdr_t) + segment->header.length;
	if(count % 2 == 1) {
        count++;
    }
    
	count = count / 2;
	
    while(count > 0){
        sum += *addr++;
        if(sum & 0x10000) {
            sum = (sum & 0xFFFF) + 1;
        }
        count--;
    }
    
	return ~sum;
}

//Check the checksum in the segment,
//return 1 if the checksum is valid,
//return -1 if the checksum is invalid
int checkchecksum(seg_t* segment)
{
    register long sum = 0;
    unsigned short int checksum;
    unsigned int count = 0;
    
    //to make the increment by 16 bits, do a type cast
    unsigned short* addr = (unsigned short*)segment;
    
    //add a byte of 0 if odd number
	if(segment->header.length % 2 == 1) {
		segment->data[segment->header.length] = 0;
    }
    
	count = sizeof(srt_hdr_t) + segment->header.length;
	if(count % 2 == 1) {
        count++;
    }
    
	count = count / 2;
	
    while(count > 0){
        sum += *addr++;
        if(sum & 0x10000) {
            sum = (sum & 0xFFFF) + 1;
        }
        count--;
    }
    checksum = ~sum;
    if (checksum == 0) {
        return 1;
    }
    else {
        return -1;
    }
}
