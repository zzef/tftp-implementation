#define TFTP_PORT 69

struct wrt_rq {
	char* opcode;
	char* file_name;
	char* mode;
};

extern int bake_rq_pkt(const char rq, 
						const char* file_name, 
						const char* mode, char** wrq
						);


