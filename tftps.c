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
    int txmt = 30;
	int rexmt = 2;
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
			if (*res=='\0'&&*(res+1)=='\1') {
				char* remote_host = pckt->ip_addr;
				char* file_name = (pckt->data)+2;
				char* current_mode = (pckt->data)+2+strlen(file_name)+1;
				FILE* file = prepare_file(PATH,file_name,current_mode,"r");
				printf("  %s has requested %s mode %s\n",remote_host,file_name,current_mode);
				if (file==NULL) {
					printf("  File Error %s\n",strerror(errno));
					send_client_error(sockfd,pckt->ip_addr,pckt->port,get_error_code(errno));
					continue;
				}
				if (send_file(file,file_name,pckt->ip_addr,pckt->port,txmt,rexmt,current_mode)<0) {
					printf("  Error - %s\n",strerror(errno));
					send_client_error(sockfd,pckt->ip_addr,pckt->port,get_error_code(errno));
				}

			}
			else if (*res=='\0'&&*(res+1)=='\2') {
					
				file_name = (pckt->data)+2;
				mode = (pckt->data)+strlen(pckt->data+2)+3;
				FILE* file = prepare_file(PATH,file_name,mode,"wx");	
				
				if (file==NULL) {
					printf("  File Error - %s\n",strerror(errno));
					send_client_error(sockfd,pckt->ip_addr,pckt->port,get_error_code(errno));
					continue;
				}
				if (receive_file(file,file_name,pckt->ip_addr,pckt->port,mode)<0) {
					printf("  Error - %s\n",strerror(errno));
				}
			}
        }
    }
}
