#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include "sockets.h"
#include "packet_debug.h"
#include "tftp_utils.h" 

int main(int argc, char** argv) {
   	 
	char* IP_ADDRESS = argv[1];

    int sockfd = socket(AF_INET,SOCK_DGRAM,0);  
    if (sockfd<0){
        printf("Error: Failed to create socket - %s\n",strerror(errno));
        exit(EXIT_FAILURE);
    } else {
        printf("Successfully created socket!\n");
    }
    
    if (bind_socket(sockfd,IP_ADDRESS,TFTP_PORT)<0) {
        printf("Error: Failed to bind socket - %s\n",strerror(errno));
        exit(EXIT_FAILURE);
    } else {
        printf("Successfully binded socket!\n");
    }
    
    printf("listening on port %i\n",TFTP_PORT);
    while(1) {
        
        struct packet* pckt = malloc(sizeof(struct packet));

        if(receive(sockfd,pckt)<0) {
            printf("Failed to receive packet - %s\n",strerror(errno));
        }
        else {		
			printf("Recieved packet: \"");
			print_pkt_data(pckt->data,pckt->data_len);
			printf("\" from [%s,%i] (%li bytes)\n",
					pckt->ip_addr,
					pckt->port,
					pckt->data_len
					);
	
			unsigned char* res = (unsigned char*) pckt->data;
			if (*res=='\0'&&*(res+1)=='\2') {
				char *ack_pkt;	
				int size_ack = bake_ack_pkt(0,&ack_pkt);	
				send_data(sockfd,pckt->ip_addr,pckt->port,ack_pkt,size_ack);
			}	
        }
    }
}
