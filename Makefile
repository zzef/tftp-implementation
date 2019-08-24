all: tftp-client tftp-server
tftp-client: tftpc.c sockets.o tftp_utils.o packet_debug.o
	gcc -g -o tftp-client tftpc.c sockets.o tftp_utils.o packet_debug.o
tftp-server: tftps.c sockets.o tftp_utils.o packet_debug.o
	gcc -g -o tftp-server tftps.c sockets.o tftp_utils.o packet_debug.o
packet_debug.o: packet_debug.h packet_debug.c
	gcc -c -g packet_debug.c
tftp_utils.o: tftp_utils.h tftp_utils.c sockets.o
	gcc -c -g tftp_utils.c
sockets.o: sockets.h sockets.c
	gcc -c -g sockets.c
