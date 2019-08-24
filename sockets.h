#define BUFF_SIZE 1024

struct packet {
    int port;
    char* ip_addr;
    char* data;
    long data_len;
    
};


int send_data(int sockfd, char* ip_addr, int port, char* data, int size);
int bind_socket(int sockfd, char* ip_addr, int port);
int receive(int sockfd, struct packet* pckt);
int set_timeout(int sockfd, int timeout);
int destroy_packet(struct packet* pckt);
