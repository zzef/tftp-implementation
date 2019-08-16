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

void report_error(unsigned char* res) {
	u_int16_t* casted = (u_int16_t*) res;
	u_int16_t err_code = ntohs(*(casted+1));
	char* error_msg = get_error_message((char)err_code);
	printf("  Error[host] - %s\n",error_msg);
}

char get_block(FILE* file, long file_size, int T_BLOCK, char** data_section, int* len) {

	long position = MAX_TRANSFER*(T_BLOCK-1);
	*data_section = malloc(MAX_TRANSFER);
	memset(*data_section,0,MAX_TRANSFER);
	int to_read;
	fseek(file,position,SEEK_SET);
	char last=0;

	if (position+MAX_TRANSFER<=file_size) {
		to_read=MAX_TRANSFER;		
	}
	else {
		to_read=file_size-position;
		//printf("FILE SIZE: %lu\n file_size-position %lu\n",file_size, file_size-position);
		last=1;
	}
	fread(*data_section,1,to_read,file);
	*len=to_read;
	return last;
}

int transfer(FILE* file, char* file_name, char* remote_host, char* mode, int rexmt, int txmt){
	
	printf("  Transferring %s to %s (mode %s)\n  retransmission timeout %i\n  transmission timeout %i\n",
			file_name,remote_host,mode,rexmt,txmt);
	
	u_int16_t TID; 
	int sockfd = connect_host(&TID);
	if(sockfd<0){
		return -1;
	}
	else {
		printf("  transmission identifier %i\n\n",TID);
	}

	char* data;
	fseek(file,0,SEEK_END);
	long f_size = ftell(file);
	int len = 0;
	rewind(file);
	int T_BLOCK = 0;
	char last=0;
	char* data_packet;


	while(last==0) {
		if (T_BLOCK<1) {
			char* wrq;
			int size_wrq = bake_rq_pkt('w',file_name,mode,&wrq);
			if(send_data(sockfd,remote_host,TFTP_PORT,wrq,size_wrq)<0){
				return -1;
			}
		}
		else {
			last = get_block(file,f_size,T_BLOCK,&data,&len);
			printf("\n\n  transferring block %i (%lu bytes)\n",T_BLOCK,len);
			printf("  -------------------------------------------------\n");
			print_pkt_data(data,len);
			printf("\n  -------------------------------------------------\n");
			int size_data = bake_data_pkt(data,len,T_BLOCK,&data_packet);
			send_data(sockfd,remote_host,TFTP_PORT,data_packet,size_data);
		}
		
		struct packet* pckt = malloc(sizeof(struct packet));
		if(receive(sockfd,pckt)>0) {
			printf("  Recieved packet:");
			print_pkt_data(pckt->data,pckt->data_len);
			printf(" from [%s,%i] (%li bytes)\n",
				pckt->ip_addr,
				pckt->port,
				pckt->data_len
				);
	
			unsigned char* res = (unsigned char*) pckt->data;
		
			if (*res=='\0' && *(res+1)=='\5') {
				report_error(res);
				free(data);
				data=NULL;
				return -2;
			}
			else if (*res=='\0' && *(res+1)=='\4') {
				u_int16_t* block = (u_int16_t*) (pckt->data)+1;
				printf("  [ACK] -> block %i\n",ntohs(*block));
	
			}
		}
		else {
			return -1;
		}  

		T_BLOCK++;
	}

	free(data);
	data=NULL;
	printf("\n  Transfer Complete!\n\n");

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

		if (strcmp(token_list[0],"put")==0) {
			if(len>=3) {
				for (int i = 1; i<len-1;i++){
					FILE* file;
					if (strcmp(BINARY,current_mode)==0) {
						file = fopen(token_list[i],"rb");
					}
					else {
						file = fopen(token_list[i],"r");
					}
					if (file==NULL) {
						printf("  File Error - %s\n",strerror(errno));
						continue;
					}
					int trans = transfer(file,token_list[i],token_list[len-1],
									current_mode,rexmt,txmt);
					if (trans==-1) {
						printf("  Failed to connect to host - %s\n%s\n",
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






