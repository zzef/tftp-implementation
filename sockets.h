#define PORT 69
#define BUFF_SIZE 512

struct packet {
    int port;
    struct in_addr ip_addr;
    char* data;
    long data_len;
    
};


extern int send_data(int sockfd, char* ip_addr, int port, char* data, int size);
extern int bind_socket(int sockfd, char* ip_addr, int port);
extern int receive(int sockfd, struct packet* pckt);
