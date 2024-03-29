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
	printf("  Error[host] - %s\n\n",error_msg);
}

void send_client_error(int sockfd,char* ip_addr, int port, int error_no) {
	char *err_pkt;
	char err_code = error_no;
	char *err_msg = "There was a problem fam.";
	int size_err = bake_err_pkt(err_code,err_msg,strlen(err_msg),&err_pkt);
	send_data(sockfd,ip_addr,port,err_pkt,size_err);	
}

char get_block(FILE* file, long file_size, int T_BLOCK, char** data_section, int* len) {

	long position = MAX_TRANSFER*T_BLOCK;
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
int receive_file(FILE* file, char* file_name, char* client_ip, int client_tid, char* mode) {

	u_int16_t TID;
	int sockfd2 = bind_random(&TID);
	if (sockfd2<0) {
		printf("  Failed to bind to an address - %s\n",strerror(errno));
		return -1;
	}

	char *ack_pkt;	
	int size_ack = bake_ack_pkt(0,&ack_pkt);	
	send_data(sockfd2,client_ip,client_tid,ack_pkt,size_ack);
	//printf("  %i\n",strlen(pckt->data+2));
	printf("  WRQ to %s  (mode %s)\n  transmission identifier %i\n",
		file_name,mode,client_tid);
	if (receive_mode(sockfd2,client_tid,file,SERVER_TIMEOUT,1)<0) {
		return -1;
	}
	
}

int write_block(FILE* file, char* data, int length, u_int16_t block_n, long* expected_block) {

	printf("  block %i comp %i\n",block_n,*expected_block%65535);
	if (block_n==(*expected_block%65535)) {	
		fseek(file,MAX_TRANSFER*(*expected_block-1),SEEK_SET);
		fwrite(data+DATA_HEADER_SIZE,1,length-DATA_HEADER_SIZE,file);	
		if (length-DATA_HEADER_SIZE<MAX_TRANSFER){
			fclose(file);	
			printf("  Transfer complete!\n\n");
			return 1;
		} else {
			*expected_block=*expected_block+1;
		}
	}
	return 0;
}

int receive_mode(int sockfd, int TID, FILE* file, int timeout, long start_block) {
	long expected_block=start_block;
	int seek_block=0;
	set_timeout(sockfd,timeout);
	char hanging = 0;
	while(1) {
		struct packet* pckt = malloc(sizeof(struct packet));
		if(receive(sockfd,pckt)<0) {
			if (hanging) {
				destroy_packet(pckt);
				return 0;
			}
			destroy_packet(pckt);
			return -1;
		}
		else {
			printf("  ----------------------------------------------------------\n");
			display_packet(pckt);
			unsigned char* res = (unsigned char*) pckt->data;
			if (*res=='\0'&&*(res+1)=='\3') {
				u_int16_t* blk = (u_int16_t*) (pckt->data)+1;	
				u_int16_t block_n = ntohs(*blk);
				printf("  received block %i expected block %i\n", block_n,expected_block);
				if(pckt->port!=TID) {
					send_client_error(sockfd,pckt->ip_addr,pckt->port,5);
					destroy_packet(pckt);
					continue;
				}
				if(write_block(file,pckt->data,pckt->data_len,block_n,&expected_block)) {
					hanging=1;
				}
				send_ack(sockfd,pckt->ip_addr,pckt->port,block_n);
			}
		}
		destroy_packet(pckt);
	}	
}


int send_file(FILE* file,char* file_name, char* remote_host, int host_TID, int txmt, int rexmt, char* mode) {
	
	printf("\n\n  %s\n  ======================================================\n",file_name);	
	printf("  Transferring %s to %s (mode %s)\n  retransmission timeout %i\n  transmission timeout %i\n",
				file_name,remote_host,mode,rexmt,txmt);

	u_int16_t TID;
	int sockfd = bind_random(&TID);	
	if (sockfd<0) {				
		return -1;
	}						
	int trans = transfer(sockfd,remote_host,host_TID,file,file_name,
		mode,rexmt,txmt);

	if (trans==-1) {
		printf("  Could not send to host %s - %s\n",
		remote_host,strerror(errno));
		return -1;
	}
	return 0;
}


int transfer(int sockfd, char* remote_host, int host_TID, FILE* file, 
	char* file_name, char* mode, int rexmt, int txmt){
	

	fseek(file,0,SEEK_END);
	long f_size = ftell(file);
	int len = 0;
	rewind(file);
	u_int16_t T_BLOCK = 1;
	int seek_block=0;
	char last=0;
	int timeouts=0;
	set_timeout(sockfd,rexmt);

	while(last==0) {
		char* data;
		char* data_packet;
		struct packet* pckt = malloc(sizeof(struct packet));
		last = get_block(file,f_size,seek_block,&data,&len);
		int size_data = bake_data_pkt(data,len,T_BLOCK,&data_packet);
		send_data(sockfd,remote_host,host_TID,data_packet,size_data);
		printf("\n\n  transferring block %i (%lu bytes)\n",T_BLOCK,size_data);
		printf("  -------------------------------------------------\n");
		print_pkt_data(data_packet,size_data);
		printf("\n  -------------------------------------------------\n");	

		if(receive(sockfd,pckt)<0) {
			if (errno!=EWOULDBLOCK && errno!=EAGAIN) {
				free(data);
				data=NULL;
				destroy_packet(pckt);
				return -1;
			}
			else if	(txmt<=(timeouts*rexmt)) {
				free(data);
				data=NULL;
				destroy_packet(pckt);
				return -1;
			}
			else {
				printf("  retransmitting block %i try %i\n",T_BLOCK,timeouts);
				timeouts++;
			}
		}
		else {
			display_packet(pckt);	
			unsigned char* res = (unsigned char*) pckt->data;
			if (*res=='\0' && *(res+1)=='\5') {
				report_error(res);
				free(data);
				data=NULL;
				destroy_packet(pckt);
				return -2;
			}
			else if (*res=='\0' && *(res+1)=='\4') {
				u_int16_t* block = (u_int16_t*) (pckt->data)+1;
				printf("  [ACK] -> block %i\n",ntohs(*block));
				if (ntohs(*block)==T_BLOCK) {
					if (host_TID==pckt->port) {
						T_BLOCK++;
						T_BLOCK = T_BLOCK % 65535;	
						seek_block++;
					}
				}
			}
		};
		free(data);
		data=NULL;
		destroy_packet(pckt);
	}

	printf("  Transfer Complete!\n\n");
	return 0;

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
	free(path);
	path=NULL;
	free(result_mode);
	result_mode=NULL;
	return file;

}


