#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "sockets.h"
#include <errno.h>
#include "time.h"
#include <unistd.h>
#include "tftp_utils.h"
#include "packet_debug.h"

/*
	Opcodes
	-------

	opcode   operation
	  1      Read Request (RRQ)
	  2      Write Request (WRQ)
	  3      Data (DATA)
      4      Acknowledgement (ACK)
      5  	 Error (ERROR)
*/

char* get_error_message(char code) {
	switch(code){
		case(0): return "Not defined. see error message (if any).";
		case(1): return "File not found.";
		case(2): return "Access violation.";
		case(3): return "Disk full or allocation exceeded.";
		case(4): return "Illegal TFTP operation.";
		case(5): return "Unknown transfer ID.";
		case(6): return "File already exists.";
		case(7): return "No such user/host";
	}
}


//This function concatenates characters
char* concat(const void* a, int size_a,
			 const void* b, int size_b, 
		 	 int* len) {

	char* final = malloc(size_a+size_b);
	memcpy(final,a,size_a);
	memcpy(final+size_a,b,size_b); //+1 for terminating character '\0'
	*len=size_a+size_b;
	return final;
}


//Encoding to netascii
int netascii(char* msg, int size, char** nascii) {
	
	int length = 0;
	char CR = 13;
	char LF = 10;
	char NUL = 0;
	for (int i = 0; i<size; i++) {
		if (msg[i]==CR && msg[i+1]!=LF){
			*nascii = concat(*nascii,length,&CR,1,&length);
			*nascii = concat(*nascii,length,&NUL,1,&length);
			continue;
		}
		if (msg[i]==LF && msg[i-1]!=CR){//maayyy be unsaffe not sure yet
			*nascii = concat(*nascii,length,&CR,1,&length);
			*nascii = concat(*nascii,length,&LF,1,&length);
			continue;
		}
		*nascii = concat(*nascii,length,msg+i,1,&length);
	}
	*nascii = concat(*nascii,length,"\0",1,&length);
	return length;
}



int bake_rq_pkt(const char rq, const char* file_name, 
					const char* mode, 
					char** wrq){
/*
	RRQ/WRQ Packet
	---------------
	   2 bytes     string     1 byte   string  1 byte
	 -------------------------------------------------
	|  opcode  |  file name  |   0   |  mode  |   0   |                   
	 -------------------------------------------------
*/

	int length = 0;
	switch(rq){
		case('r'): *wrq = "\0\1"; break;
		case('w'): *wrq = "\0\2"; break;
	}	
	*wrq = concat(*wrq,2,file_name,strlen(file_name)+1,&length);
	*wrq = concat(*wrq,length,mode,strlen(mode)+1,&length);

	return length;
}

int bake_data_pkt(char* data, int data_length, 
				u_int16_t block_no, char** data_pkt){
	
/*
	DATA Packet
	------------
	   2 bytes    2 bytes    n bytes
	 ---------------------------------
   	|  opcode  |  Block #  |   Data   |
	 ---------------------------------
*/
	u_int16_t n_block_no = htons(block_no);
	int length = 0;
	*data_pkt = "\0\3";
	*data_pkt = concat(*data_pkt,2,&n_block_no,2,&length);
	*data_pkt = concat(*data_pkt,length,data,data_length,&length);
	//check possible bug to do with endianness	
	return length;
}

int bake_ack_pkt(u_int16_t block_no, char** ack_pkt) {
/*
	ACK Packet
	----------
	   2 bytes    2 bytes
	 ----------------------
	|  opcode  |  Block #  |
	 ----------------------
*/	

	u_int16_t n_block_no = htons(block_no);
	int length = 0;
	*ack_pkt = "\0\4";
	*ack_pkt = concat(*ack_pkt,2,&n_block_no,2,&length);
	return length;	

}

int bake_err_pkt(u_int16_t error_code, char* err_msg, int err_msg_size, char** err_pkt) {
/*
	ERROR Packet
	------------
	   2 bytes     2 bytes      string   1 byte
	 -------------------------------------------
	|  opcode  |  ErrorCode  |  ErrMsg  |   0   |
	 -------------------------------------------

	Error codes
	-----------
	0         Not defined, see error message (if any).
  	1         File not found.
  	2         Access violation.
  	3         Disk full or allocation exceeded.
  	4         Illegal TFTP operation.
  	5         Unknown transfer ID.
  	6         File already exists.
  	7         No such user.
*/

	u_int16_t n_error_code = htons(error_code);
	char* nascii_err_msg;
	int len_nascii = netascii(err_msg,err_msg_size,&nascii_err_msg);
	int length = 0;
	*err_pkt = "\0\5";
	*err_pkt = concat(*err_pkt,2,&n_error_code,2,&length);
	*err_pkt = concat(*err_pkt,length,nascii_err_msg,len_nascii,&length);
	return length;
}

u_int16_t random_16(u_int16_t lower, u_int16_t upper) {
	srand(time(0));
	return (rand() % (upper - lower + 1)) + lower;
}

int bind_random(u_int16_t* TID) {
	int sockfd = socket(AF_INET, SOCK_DGRAM,0);
	int tries=0;
	do {
	
		*TID = random_16(10000,32000);
		if (sockfd<0){
			return -1;
		}
		int res = bind_socket(sockfd,NULL,*TID);
		if (res<0 && errno==98) {
			tries++;
			sleep(1);
		}
		else if (res<0) {
			return -1;
		}
		else {
			return sockfd;
		}
		
	} while(errno=98 && tries<MAX_TRIES);
	return -1;
}
/*
int connect_host(int sockfd, u_int16_t* host_TID) {
	struct packet* pckt = malloc(sizeof(struct packet));
	if(receive(sockfd,pckt)<0) {
		free(pckt);
		pckt=NULL;
		return -1;
	}
	else {
		display_packet(pckt);
		unsigned char* res = (unsigned char*) pckt->data;		
		if (*res=='\0' && *(res+1)=='\5') {
			report_error(res);
			free(pckt);
			pckt=NULL;
			return -2;
		}
		else if (*res=='\0' && *(res+1)=='\4') {
			u_int16_t* block = (u_int16_t*) (pckt->data)+1;
			printf("  [ACK] -> block %i\n",ntohs(*block));
			if (ntohs(*block)==0) {
				*host_TID=pckt->port;
			}	
		}
	}
	free(pckt);
	pckt=NULL;
	return 0;
}
*/
int request_host(int sockfd, char* remote_host, 
			char* file_name, char type, char* mode){

	char* wrq;
	int size_wrq = bake_rq_pkt(type,file_name,mode,&wrq);
	if(send_data(sockfd,remote_host,TFTP_PORT,wrq,size_wrq)<0){
		return -1;
	}
	return 0;

}

void report_error(unsigned char* res) {
	u_int16_t* casted = (u_int16_t*) res;
	u_int16_t err_code = ntohs(*(casted+1));
	char* error_msg = get_error_message((char)err_code);
	printf("  Error[host] - %s\n",error_msg);
}

void send_client_error(int sockfd,char* ip_addr, int port, int error_no) {
	char *err_pkt;
	char err_code = error_no;
	char *err_msg = "There was a problem fam.";
	int size_err = bake_err_pkt(err_code,err_msg,strlen(err_msg),&err_pkt);
	send_data(sockfd,ip_addr,port,err_pkt,size_err);	
}

char get_block(FILE* file, long file_size, int T_BLOCK, char** data_section, int* len) {

	long position = MAX_TRANSFER*(T_BLOCK-1);
	*data_section = malloc(MAX_TRANSFER);
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

void send_ack(int sockfd, char* ip_addr, int port, long block_n) {
	char *ack_pkt;
	int size_ack = bake_ack_pkt(block_n,&ack_pkt);		
	send_data(sockfd,ip_addr,port,ack_pkt,size_ack);
}

int receive_mode(int sockfd, int TID, FILE* file) {
	struct packet* pckt = malloc(sizeof(struct packet));
	int expected_block=1;
	while(1) {
		if(receive(sockfd,pckt)<0) {
			printf("Failed to receive packet - %s\n",strerror(errno));
		}
		else {
			display_packet(pckt);
			unsigned char* res = (unsigned char*) pckt->data;
			if (*res=='\0'&&*(res+1)=='\3') {
				u_int16_t* blk = (u_int16_t*) (pckt->data)+1;	
				printf("TID: %i\nPORT %i\n",TID,pckt->port);
				if(pckt->port!=TID) {
					send_client_error(sockfd,pckt->ip_addr,pckt->port,5);
					continue;
				}
				long block_n = ntohs(*blk);
				if (block_n==expected_block) {	
					fseek(file,MAX_TRANSFER*(block_n-1),SEEK_SET);
					fwrite(pckt->data+DATA_HEADER_SIZE,1,pckt->data_len-DATA_HEADER_SIZE,file);	
					expected_block++;
					if (pckt->data_len-DATA_HEADER_SIZE<MAX_TRANSFER){
						fclose(file);	
						printf("Transfer complete!");
						send_ack(sockfd,pckt->ip_addr,pckt->port,block_n);
						return 0;
					}	
				}
				send_ack(sockfd,pckt->ip_addr,pckt->port,block_n);
			}
		}
	}	
}
int transfer(int sockfd, char* remote_host, FILE* file, 
	char* file_name, char* mode, int rexmt, int txmt){
	
	char* data;
	fseek(file,0,SEEK_END);
	long f_size = ftell(file);
	int len = 0;
	rewind(file);
	int T_BLOCK = 0;
	char last=0;
	char* data_packet;
	struct packet* pckt = malloc(sizeof(struct packet));
	int host_TID;

	while(last==0) {
		
		if(receive(sockfd,pckt)>0) {
			display_packet(pckt);	
			unsigned char* res = (unsigned char*) pckt->data;
			if (*res=='\0' && *(res+1)=='\5') {
				report_error(res);
				return -2;
			}
			else if (*res=='\0' && *(res+1)=='\4') {
				u_int16_t* block = (u_int16_t*) (pckt->data)+1;
				printf("  [ACK] -> block %i\n",ntohs(*block));
				if (ntohs(*block)==T_BLOCK) {
					if (T_BLOCK==0) {
						host_TID=pckt->port;
					}
					if (host_TID==pckt->port) {
						T_BLOCK++;
						last = get_block(file,f_size,T_BLOCK,&data,&len);
						int size_data = bake_data_pkt(data,len,T_BLOCK,&data_packet);
						send_data(sockfd,remote_host,host_TID,data_packet,size_data);
						printf("\n\n  transferring block %i (%lu bytes)\n",T_BLOCK,size_data);
						printf("  -------------------------------------------------\n");
						print_pkt_data(data_packet,size_data);
						printf("\n  -------------------------------------------------\n");	
					}
				}
			}
		}
		else {
			return -1;
		}
	}

	free(data);
	data=NULL;
	printf("\n  Transfer Complete!\n\n");

}

FILE* prepare_file(char* dir, char* file_name, char* mode, char* method) {

	int path_len = 0;
	char* path = concat(dir,strlen(dir),
		file_name,strlen(file_name),&path_len);

	char *m;
	char* result_mode;
	if (strcmp(BINARY,mode)==0) {
		printf("  Opening file in binary mode\n");
		int len=0;
		result_mode = concat(method,strlen(method),"b",1,&len);	
	}
	else if (strcmp(ASCII,mode)==0) {
		printf("  Opening file in ascii mode\n");
		result_mode=method;
	}
	FILE* file = fopen(path,result_mode);
	return file;

}


