#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "sockets.h"
#include <errno.h>
#include "time.h"
#include <unistd.h>

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

//doesn't actually connect, just establishes TID
int connect_host(u_int16_t* TID){

	int sockfd = socket(AF_INET, SOCK_DGRAM,0);
	int tries=0;
	do {
		*TID = random_16(10000,32000);
		if (sockfd<0){
			return sockfd;
		}
		if (bind_socket(sockfd,NULL,*TID)<0 && errno!=98) {
			return -1;
		}
		sleep(1);
		tries++;
		
	} while (errno==98 && tries<100);

	if(tries==100){
		return -1;
	}
	return sockfd;

}


