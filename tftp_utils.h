#define TFTP_PORT 69
#define INPUT_SIZE 1024
#define BINARY "octet"
#define ASCII "asciii"

struct wrt_rq {
	char* opcode;
	char* file_name;
	char* mode;
};

extern int bake_rq_pkt(const char rq, 
						const char* file_name, 
						const char* mode, char** wrq
						);

extern int bake_data_pkt(char* data, int data_length, 
				u_int16_t block_no, char** data_pkt);
	
extern int bake_ack_pkt(u_int16_t block_no, 
				char** ack_pkt);

extern int bake_err_pkt(u_int16_t error_code, 
				char* err_msg,
				int err_msg_size, 
				char** err_pkt);

u_int16_t random_16(u_int16_t lower, u_int16_t upper);

int connect_host(u_int16_t* TID);


