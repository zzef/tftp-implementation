#define BUFF_SIZE 1024

struct packet {
    int port;
    char* ip_addr;
    char* data;
    long data_len;
    
};


extern int send_data(int sockfd, char* ip_addr, int port, char* data, int size);
extern int bind_socket(int sockfd, char* ip_addr, int port);
extern int receive(int sockfd, struct packet* pckt);
