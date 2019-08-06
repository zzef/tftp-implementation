#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include "sockets.h"

int bind_socket(int sockfd, char* ip_addr, int port) {
    
    struct in_addr* addr = malloc(sizeof(struct in_addr));
    
    if(ip_addr==NULL){
        addr->s_addr=INADDR_ANY;// bind to all local addresses
    }
    else {
        inet_aton(ip_addr,addr);       
    }
    
    struct sockaddr_in* server_address = malloc(sizeof(struct sockaddr_in));
    server_address->sin_family=AF_INET;
    server_address->sin_port=htons(port);
    server_address->sin_addr=*addr;
    
    return bind(sockfd, (const struct sockaddr*) server_address, sizeof(*server_address));
    
}


int send_data(int sockfd, struct sockaddr *address, char* data) {
	
	return sendto(sockfd,(const void*) data, sizeof(*data),0,
					address,sizeof(*address)
					);

}


int receive(int sockfd, struct packet* pckt) {
    
        struct sockaddr_in* sender_address = malloc(sizeof(struct sockaddr_in));
        unsigned int len = sizeof(*sender_address);
        char* packet_buffer = malloc(BUFF_SIZE);
        
        int recv_size = recvfrom(
            sockfd,(void*)packet_buffer,
            BUFF_SIZE,0,
            (struct sockaddr*)sender_address,&len
        );
        
        if(recv_size<0) {
            return recv_size;
        }
        
        char* sender_ip_addr = inet_ntoa(sender_address->sin_addr);
        int port = sender_address->sin_port;
        
        pckt->port=port;
        pckt->ip_addr=sender_address->sin_addr;
        pckt->data=packet_buffer;
        pckt->data_len=recv_size;
        
        return recv_size;
}
