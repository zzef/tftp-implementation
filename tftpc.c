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
	printf("  1. put [file1] [file2] [file3]... [remote-host]\n");
	printf("  2. get [remote-host] [file1] [file2] [file3]... \n");
	printf("  3. mode [binary|ascii]\n");
	printf("  4. rexmt [retransmission-timeout]\n");
	printf("  5. timeout [transmission-timeout]\n");
	printf("  6. verbose [on|off]\n");
	printf("  7. help\n");
	printf("  8. quit\n\n");
	
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
	int txmt = 10;
	int rexmt = 1;

	while(1) {
		printf("  tftp: ");
		fgets(comms,INPUT_SIZE,stdin);
		char ** token_list = malloc(100);
		int len = tokenize(comms,&token_list);
		
		if(empty(comms)){
			continue;
		}
		if (strcmp(token_list[0],"get")==0) {
			if(len>=3) {
				for (int i = 1; i<len-1;i++){
					
				}
			}
		}
		else if (strcmp(token_list[0],"put")==0) {
			if(len>=3) {
				for (int i = 1; i<len-1;i++){
				
					char* remote_host = token_list[len-1];
					char* file_name = token_list[i];
					
					printf("  Transferring %s to %s (mode %s)\n  retransmission timeout %i\n  transmission timeout %i\n",
							file_name,remote_host,current_mode,rexmt,txmt);

					u_int16_t TID;
					int sockfd = bind_random(&TID);
					if (sockfd<0) {
						printf("  Failed to bind to an address - %s\n",strerror(errno));
						continue;
					}
					
					if (request_host(sockfd,remote_host,file_name,'w',current_mode)<0) {
						printf("  Failed to send request to host - %s\n",strerror(errno));
						continue;
					}
						
					FILE* file = prepare_file("",file_name,current_mode,"r");
					if (file==NULL) {
						printf("  File Error - %s\n",strerror(errno));
						continue;
					}

					int trans = transfer(sockfd,remote_host,file,file_name,
									current_mode,rexmt,txmt);
					if (trans==-1) {
						printf("  Failed to get a response from host - %s\n%s\n",
								remote_host,strerror(errno));
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






