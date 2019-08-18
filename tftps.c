#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include "sockets.h"
#include "packet_debug.h"
#include "tftp_utils.h" 

char* PATH = "/home/zef/test_folder/";
int server_timeout = 20;

char get_error_code(char error_no) {
	switch(error_no){
		case(ENOENT):return(1);
		case(EACCES):return(2);
		case(EFBIG):return(3);
		case(ENOSPC):return(3);
		case(EEXIST):return(6);
		default:return(0);
	}
}

int main(int argc, char** argv) {
   	 
	char* IP_ADDRESS = argv[1];
	u_int16_t TID;
	char* mode;
	char* file_name;

    int sockfd = socket(AF_INET,SOCK_DGRAM,0);  
    if (sockfd<0){
        printf("  Error: Failed to create socket - %s\n",strerror(errno));
        exit(EXIT_FAILURE);
    } else {
        printf("\n  Socket created\n");
    }
    
    if (bind_socket(sockfd,IP_ADDRESS,TFTP_PORT)<0) {
        printf("  Error: Failed to bind socket - %s\n",strerror(errno));
        exit(EXIT_FAILURE);
    } else {
        printf("  Socket bound!\n");
    }
    
	struct packet* pckt = malloc(sizeof(struct packet));
    while(1) {
	    printf("  listening on port %i\n",TFTP_PORT);    
        if(receive(sockfd,pckt)<0) {
            printf("  Failed to receive packet - %s\n",strerror(errno));
        }
        else {
			display_packet(pckt);
			printf("  ---------------------------\n");			
			unsigned char* res = (unsigned char*) pckt->data;
			if (*res=='\0'&&*(res+1)=='\2') {
					
				file_name = (pckt->data)+2;
				mode = (pckt->data)+strlen(pckt->data+2)+3;
				FILE* file = prepare_file(PATH,file_name,mode,"wx");
	
				if (file==NULL) {
					printf("  error with file\n");
					send_client_error(sockfd,pckt->ip_addr,pckt->port,get_error_code(errno));
					continue;
				}
				else {
					
					u_int16_t TID;
					int sockfd2 = bind_random(&TID);
					if (sockfd2<0) {
						printf("  Failed to bind to an address - %s\n",strerror(errno));
						continue;
					}

					char *ack_pkt;	
					int size_ack = bake_ack_pkt(0,&ack_pkt);	
					send_data(sockfd2,pckt->ip_addr,pckt->port,ack_pkt,size_ack);
					//printf("  %i\n",strlen(pckt->data+2));
					printf("  WRQ to %s  (mode %s)\n  transmission identifier %i\n",
						file_name,mode,pckt->port);
					if (receive_mode(sockfd2,pckt->port,file,server_timeout)<0) {
						printf("  Transfer failed - %s\n",strerror(errno));
					}
				}
			}
        }
    }
}
