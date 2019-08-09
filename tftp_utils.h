#define TFTP_PORT 69

struct wrt_rq {
	char* opcode;
	char* file_name;
	char* mode;
};

extern int bake_wrq_pkt(char* file_name, char* mode, char** wrq);
