#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include "sockets.h"
#include "tftp_utils.h"
#include "packet_debug.h"
#include "time.h"

/*
	1. Make sure to fix buffer overflow error when using scanf()
	2. Make sure to free all memory
*/

void display_help() {
	printf("\n  Commands\n");
	printf("  -----------------------------------------------\n");
	printf("  put [file1] [file2] [file3]... [remote-host]\n");
	printf("  get [remote-host] [file1] [file2] [file3]... \n");
	printf("  mode [binary|ascii]\n");
	printf("  rexmt [retransmission-timeout]\n");
	printf("  timeout [transmission-timeout]\n");
	printf("  verbose [on|off]\n");
	printf("  help\n");
	printf("  quit\n\n");
	
}

void display_welcome() {

	//printf("\n ----------------------------\n");	
	printf("\n  TFTP Client\n");
	printf("  -----------------------------------------------\n");
	printf("  help for commands list\n\n");
}

int tokenize(char* str, char*** token_list) {
	
	char * pch;
  	pch = strtok (str," \n");
	int len = 0;
  	while (pch != NULL) {
		*(*(token_list)+len)=malloc(strlen(pch));
		memcpy(*(*(token_list)+len),pch,strlen(pch));
   		len++;		
		pch = strtok (NULL, " \n");
 	}
  	return len;	

}

char empty(char* str) {
	for (int i=0; i<strlen(str); i++) {
		if (str[i]>32&&str[i]<127)
			return 0;
	}
	return 1;
}

int main(int argc, char** argv) {
		
	display_welcome();
	char comms[INPUT_SIZE];
	char* current_mode = BINARY;
	int txmt = 30;
	int rexmt = 2;

	while(1) {
		printf("  tftp: \n\033[A");
		printf("\033[8C");
		fgets(comms,INPUT_SIZE,stdin);
		char ** token_list = malloc(100);
		int len = tokenize(comms,&token_list);
		
		if(empty(comms)){
			continue;
		}
		if (strcmp(token_list[0],"get")==0) {
			if(len>=3) {
				for (int i = 1; i<len-1;i++){
					char* remote_host = token_list[len-1];
					char* file_name = token_list[i];
					u_int16_t TID;
					int sockfd2 = bind_random(&TID);
					if (sockfd2<0) {
						printf("  Failed to bind to an address - %s\n",strerror(errno));
						continue;
					}
					set_timeout(sockfd2,rexmt);
					int timeouts=0;
					struct packet* pckt = malloc(sizeof(struct packet));
					while (1) {	
						if (request_host(sockfd2,remote_host,file_name,'r',current_mode)<0) {
							printf("  Error - %s\n",strerror(errno));
							break;
						}				
						if(receive(sockfd2,pckt)<0) {
							if (errno!=EWOULDBLOCK && errno!=EAGAIN) {
								printf("  Error - %s\n",strerror(errno));
								break;
							}
							else if	(txmt<=(timeouts*rexmt)) {
								printf("  Error - %s\n",strerror(errno));
								break;
							}
							else {
								printf("  retransmitting rrq try %i\n",timeouts);
								timeouts++;
							}
						}
						else {
							display_packet(pckt);
							long expected_block=1;
							unsigned char* res = (unsigned char*) pckt->data;
							if (*res=='\0' && *(res+1)=='\5') {
								report_error(res);
								break;
							}
							if (*res=='\0'&&*(res+1)=='\3') {
								u_int16_t* blk = (u_int16_t*) (pckt->data)+1;	
								u_int16_t block_n = ntohs(*blk);
								FILE* file = prepare_file("",file_name,current_mode,"wx");
								if (file==NULL) {
									printf("  File Error %s\n",strerror(errno));
									break;
								}
								printf("  received block %i expected block %i\n", block_n,expected_block);
								int done = write_block(file,pckt->data,pckt->data_len,block_n,&expected_block);
								send_ack(sockfd2,pckt->ip_addr,pckt->port,block_n);
								if (done) {
									break;
								}
								int res = receive_mode(sockfd2,pckt->port,file,SERVER_TIMEOUT,2);
								if (res==-1) {
									printf("  Error - %s\n",strerror(errno));
									break;
								}
							}
						}
					}
				}
			}
		}
		else if (strcmp(token_list[0],"put")==0) {
			if(len>=3) {
				for (int i = 1; i<len-1;i++){
					char* remote_host = token_list[len-1];
					char* file_name = token_list[i];
					FILE* file = prepare_file("",file_name,current_mode,"r");
					if (file==NULL) {
						printf("  File Error %s\n",strerror(errno));
						continue;
					}
					int timeouts=0;
					u_int16_t TID;
					int sockfd = bind_random(&TID);	
					if (sockfd<0) {				
						return -1;
					}
					struct packet* pckt = malloc(sizeof(struct packet));	
					set_timeout(sockfd,rexmt);
					while(1) {
		
						if (request_host(sockfd,remote_host,file_name,'w',current_mode)<0) {
							break;
						}				
						if(receive(sockfd,pckt)<0) {
							if (errno!=EWOULDBLOCK && errno!=EAGAIN) {
								printf("  Error - %s\n",strerror(errno));
								break;
							}
							else if	(txmt<=(timeouts*rexmt)) {
								printf("  Error - %s\n",strerror(errno));
								break;
							}
							else {
								printf("  retransmitting wrq try %i\n",timeouts);
								timeouts++;
							}
						}
						else {
							unsigned char* res = (unsigned char*) pckt->data;
							display_packet(pckt);	
							if (*res=='\0' && *(res+1)=='\5') {
								report_error(res);
								break;
							}
							else if (*res=='\0'&&*(res+1)=='\4') {
								u_int16_t* blk = (u_int16_t*) (pckt->data)+1;	
								u_int16_t block_n = ntohs(*blk);
								printf("  received block %i expected block %i\n", block_n,0);
								if (block_n==0) {
									printf("\n\n  %s\n  ======================================================\n",file_name);	
									printf("  Transferring %s to %s (mode %s)\n  retransmission timeout %i\n  transmission timeout %i\n",
											file_name,remote_host,current_mode,rexmt,txmt);
						
									if(transfer(sockfd,remote_host,pckt->port,file,file_name,
										current_mode,rexmt,txmt)<0){
										printf("  Could not send to host %s - %s\n",
										remote_host,strerror(errno));
										break;
									}
									break;
								}
							}
							else {
								timeouts++;
							}
						}
					}					
				}
			}
		}
		else if (strcmp(token_list[0],"quit")==0) {
			break;
		}
		else if (strcmp(token_list[0],"help")==0) {
			display_help();
		}	
	
	}
		
}






