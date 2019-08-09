#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include "sockets.h"

int main(int argc, char** argv) {
   	 
    int sockfd = socket(AF_INET,SOCK_DGRAM,0);  
    if (sockfd<0){
        printf("Error: Failed to create socket - %s\n",strerror(errno));
        exit(EXIT_FAILURE);
    } else {
        printf("Successfully created socket!\n");
    }
    
    if (bind_socket(sockfd,NULL,PORT)<0) {
        printf("Error: Failed to bind socket - %s\n",strerror(errno));
        exit(EXIT_FAILURE);
    } else {
        printf("Successfully binded socket!\n");
    }
    
    printf("listening on port %i\n",PORT);
    while(1) {
        
        struct packet* pckt = malloc(sizeof(struct packet));

        if(receive(sockfd,pckt)<0) {
            printf("Failed to receive packet - %s\n",strerror(errno));
        }
        else {
            printf("Received: \"%s\" from [%s,%i] (%li bytes)\n",
							pckt->data,
							inet_ntoa(pckt->ip_addr),
							pckt->port,
							pckt->data_len
							);
        }
        
        
    }
    
}
