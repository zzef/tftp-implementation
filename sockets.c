#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include "sockets.h"
#include "sys/time.h"

struct sockaddr_in* prep_address(char* ip_addr, int port) {
 
	struct in_addr* addr = malloc(sizeof(struct in_addr));
    
    if(ip_addr==NULL){
        addr->s_addr=INADDR_ANY;// bind to all local addresses
    }
    else {
        inet_aton(ip_addr,addr);       
    }
    
    struct sockaddr_in* address = malloc(sizeof(struct sockaddr_in));
    address->sin_family=AF_INET;
    address->sin_port=htons(port);
    address->sin_addr=*addr;
    free(addr);
	addr=NULL;
	return address;

}

int destroy_address(struct sockaddr_in* address) {
	free(address);
	address=NULL;
}

int bind_socket(int sockfd, char* ip_addr, int port) {
    
   	struct sockaddr_in* address = prep_address(ip_addr, port);
    int bound = bind(sockfd, (const struct sockaddr*) address, sizeof(*address));
    destroy_address(address);
	return bound;
}

int send_data(int sockfd, char* ip_addr, int port, char* data, int size) {

	const struct sockaddr* address = (struct sockaddr*) prep_address(ip_addr,port); 
	int sent = sendto(sockfd,(const void*) data, size,0,
					address,sizeof(*address));
	free(data);
	data=NULL;
	destroy_address((struct sockaddr_in*)address);
	return sent;

}

int set_timeout(int sockfd, int timeout) {
	struct timeval tv;
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	return setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv));
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
			free(sender_address);
			free(packet_buffer);
			packet_buffer=NULL;
            return recv_size;
        }
        char* ip_address = inet_ntoa(sender_address->sin_addr);
        char* sender_ip_addr = calloc(strlen(ip_address)+1,sizeof(char));
		memcpy(sender_ip_addr,ip_address,strlen(ip_address));
        int port = sender_address->sin_port;
        
        pckt->port=ntohs(port);
        pckt->ip_addr=sender_ip_addr;
        pckt->data=packet_buffer;
        pckt->data_len=recv_size;
        
		free(sender_address);
		sender_address=NULL;
        return recv_size;
}

int destroy_packet(struct packet* pckt) {	

	if (pckt->ip_addr) {
		free(pckt->ip_addr);
		pckt->ip_addr=NULL;
	}
	if (pckt->data) {
		free(pckt->data);
		pckt->data=NULL;
	}
	free(pckt);
	pckt=NULL;
}

