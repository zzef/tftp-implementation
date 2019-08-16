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
    
	FILE* file;
	long expected_block=1;
    printf("listening on port %i\n",TFTP_PORT);
    while(1) {
        
        struct packet* pckt = malloc(sizeof(struct packet));

        if(receive(sockfd,pckt)<0) {
            printf("Failed to receive packet - %s\n",strerror(errno));
        }
        else {
	
			printf("Received packet: \"");
			print_pkt_data(pckt->data,pckt->data_len);
			printf("\" from [%s,%i] (%li bytes)\n",
					pckt->ip_addr,
					pckt->port,
					pckt->data_len
					);

			printf("---------------------------\n");
			
			unsigned char* res = (unsigned char*) pckt->data;
			if (*res=='\0'&&*(res+1)=='\2') {
					
				file_name = (pckt->data)+2;
				int path_len = 0;
				char* path = concat(PATH,strlen(PATH),
						file_name,strlen(file_name),&path_len);

				mode = (pckt->data)+strlen(pckt->data+2)+3;

				if (strcmp(mode,BINARY)==0) {	
					file = fopen(path,"wbx");
					if (file==NULL) {
						printf("error with file\n");
						char *err_pkt;
						char err_code = get_error_code(errno);
						char *err_msg = "There was a problem fam.";
						int size_err = bake_err_pkt(err_code,err_msg,strlen(err_msg),&err_pkt);
						send_data(sockfd,pckt->ip_addr,pckt->port,err_pkt,size_err);	
						continue;
					}
					else {
						char *ack_pkt;	
						int size_ack = bake_ack_pkt(0,&ack_pkt);	
						send_data(sockfd,pckt->ip_addr,pckt->port,ack_pkt,size_ack);
						TID=pckt->port;
						printf("%i\n",strlen(pckt->data+2));
						printf("WRQ to %s (mode %s)\ntransmission identifier %i\n",path,mode,pckt->port);				
					}
				}
			}
			else if (*res=='\0'&&*(res+1)=='\3') {
				u_int16_t* blk = (u_int16_t*) (pckt->data)+1;	
				
				printf("TID: %i\nPORT %i\n",TID,pckt->port);

				if(pckt->port!=TID) {
					char *err_pkt;
					char err_code = 5;
					char *err_msg = "There was a problem fam.";
					int size_err = bake_err_pkt(err_code,err_msg,strlen(err_msg),&err_pkt);
					send_data(sockfd,pckt->ip_addr,pckt->port,err_pkt,size_err);	
					continue;
				}

				long block_n = ntohs(*blk);
				if (block_n==expected_block) {	
					fseek(file,MAX_TRANSFER*(block_n-1),SEEK_SET);
					fwrite(pckt->data,1,pckt->data_len,file);
					expected_block++;
					if (pckt->data_len<MAX_TRANSFER){
						expected_block=1;
						fclose(file);
						printf("Transfer complete!");
					}
				}
				char *ack_pkt;
				int size_ack = bake_ack_pkt(block_n,&ack_pkt);	
				send_data(sockfd,pckt->ip_addr,pckt->port,ack_pkt,size_ack);
			}	
        }
    }
}
