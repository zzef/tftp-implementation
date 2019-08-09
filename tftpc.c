#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include "sockets.h"
#include "tftp_utils.h"

int main(int argc, char** argv) {
		
	char* IP_ADDRESS = argv[1];
	
	int sockfd = socket(AF_INET, SOCK_DGRAM,0);
	if (sockfd<0){
        printf("Error: Failed to create socket - %s\n",strerror(errno));
        exit(EXIT_FAILURE);
    } else {
        printf("Successfully created socket!\n");
    } 
	char* wrq;
	
	printf("yooo0\n");

	int size = bake_wrq_pkt("test_file.txt","octet",&wrq);
	
	printf("> %s\n",wrq);
	
	send_data(sockfd,IP_ADDRESS,69,wrq,size);
	
	printf("yooo\n");
}
