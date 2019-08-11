#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include "sockets.h"
#include "tftp_utils.h"
#include "packet_debug.h"

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
	char* rrq;

	int size_wrq = bake_rq_pkt('r',"test_file.txt","octet",&wrq);
	int size_rrq = bake_rq_pkt('w',"test_file.txt","octet",&rrq);

	printf("Sending packet: \"");
	print_pkt_data(wrq,size_wrq);
	printf("\" to [%s,%i] (%i bytes)\n",IP_ADDRESS,TFTP_PORT,size_wrq);
	send_data(sockfd,IP_ADDRESS,TFTP_PORT,wrq,size_wrq);
	
	printf("Sending packet: \"");
	print_pkt_data(rrq,size_rrq);
	printf("\" to [%s,%i] (%i bytes)\n",IP_ADDRESS,TFTP_PORT,size_rrq);
	send_data(sockfd,IP_ADDRESS,TFTP_PORT,rrq,size_rrq);
}
