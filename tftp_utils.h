#define TFTP_PORT 69
#define INPUT_SIZE 1024
#define BINARY "octet"
#define ASCII "ascii"
#define MAX_TRANSFER 512
#define DATA_HEADER_SIZE 4
#define OP_RRQ 1
#define OP_WRQ 2
#define OP_DATA 3
#define OP_ACK 4
#define OP_ERROR 5
#define MAX_TRIES 1000
#define SERVER_TIMEOUT 25


struct wrt_rq {
	char* opcode;
	char* file_name;
	char* mode;
};

int bake_rq_pkt(const char rq, 
						const char* file_name, 
						const char* mode, char** wrq
						);

int bake_data_pkt(char* data, int data_length, 
				u_int16_t block_no, char** data_pkt);
	
int bake_ack_pkt(u_int16_t block_no, 
				char** ack_pkt);

int bake_err_pkt(u_int16_t error_code, 
				char* err_msg,
				int err_msg_size, 
				char** err_pkt);

u_int16_t random_16(u_int16_t lower, u_int16_t upper);

int connect_host(int sockfd, u_int16_t* host_TID);

char* get_error_message(char code);

char* concat(const void* a, int size_a,
			 const void* b, int size_b, 
		 	 int* len); 


void report_error(unsigned char* res);

int bind_random(u_int16_t* TID);

int request_host(int sockfd, char* remote_host, 
			char* file_name, char type, char* mode);

FILE* prepare_file(char* dir, char* file_name, char* mode, char* method); 

int transfer(int sockfd, char* remote_host, int host_TID, FILE* file, 
	char* file_name, char* mode, int rexmt, int txmt);

char get_block(FILE* file, long file_size, int T_BLOCK, 
			char** data_section, int* len);

void send_client_error(int sockfd,char* ip_addr, int port, int error_no);

int receive_mode(int sockfd, int TID, FILE* file, int timeout, long start_block);

int send_file(FILE* file, char* file_name, char* remote_host, int host_TID, int txmt, int rexmt, char* mode);

int receive_file(FILE* file, char* file_name, char* client_ip, int client_tid, char* mode); 

int write_block(FILE* file, char* data, int length, u_int16_t block_n, long* expected_block);

void send_ack(int sockfd, char* ip_addr, int port, long block_n);





 
