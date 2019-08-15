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
	printf("  5. verbose [on|off]\n");
	printf("  6. help\n");
	printf("  7. quit\n\n");
	
}

void display_welcome() {

	//printf("\n ---------------------------\n");	
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

int transfer(char* file_name, char* remote_host, char* mode, int rexmt, int txmt){
	
	printf("  Transferring %s to %s (mode %s)\n  retransmission timeout %i\n  transmission timeout %i\n",
			file_name,remote_host,mode,rexmt,txmt);
	
	u_int16_t TID; 
	int sockfd = connect_host(&TID);
	if(sockfd<0){
		printf("  Failed to connect - %s\n\n",strerror(errno));
		return -1;
	}
	else {
		printf("  transmission identifier %i\n\n",TID);
	}
	
	int block_no=0;
		
	char* wrq;
	int size_wrq = bake_rq_pkt('w',file_name,mode,&wrq);
	//print_pkt_data(wrq,size_wrq);

	if(send_data(sockfd,remote_host,TFTP_PORT,wrq,size_wrq)<0){
		return -1;
	}

	struct packet* pckt = malloc(sizeof(struct packet));

    if(receive(sockfd,pckt)<0) {
         printf("  Failed to receive packet - %s\n",strerror(errno));
    }
    else {
		printf("  Recieved packet: \"");
		print_pkt_data(pckt->data,pckt->data_len);
		printf("\" from [%s,%i] (%li bytes)\n",
				pckt->ip_addr,
				pckt->port,
				pckt->data_len
				);
	
		unsigned char* res = (unsigned char*) pckt->data;
		if (*res=='\0' && *(res+1)=='\4') {
			printf("  ACK packet!\n");
			block_no++;				
		}	
    }

	printf("\n");	

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

		//for (int i=0; i<len; i++)i{
		//	printf("TOKEN: %s\n",*(token_list+i));
		//}

	
		if (strcmp(token_list[0],"put")==0) {
			if(len>=3) {
				for (int i = 1; i<len-1;i++){
					int trans = transfer(token_list[i],token_list[len-1],
									current_mode,rexmt,txmt);
					if (trans==-1) {
						printf("  Failed to connect to remote host - %s\n%s\n",
								token_list[len-1],strerror(errno));
					}

				}
			}
		}

		if (strcmp(token_list[0],"quit")==0) {
			break;
		}

		if (strcmp(token_list[0],"help")==0) {
			display_help();
		}	
	
	}
		
}






