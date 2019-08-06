#define PORT 69
#define BUFF_SIZE 512

struct packet {
    int port;
    struct in_addr ip_addr;
    char* data;
    long data_len;
    
};

int bind_socket(int sockfd, char* ip_addr, int port);
int recieve(int sockfd, struct packet* pckt);
