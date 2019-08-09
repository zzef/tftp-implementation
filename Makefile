all: tftp-client tftp-server
tftp-client: tftpc.o sockets.o
	gcc -o tftp-client tftpc.o sockets.o tftp_utils.o
tftp-server: tftps.o sockets.o
	gcc -o tftp-server tftps.o sockets.o tftp_utils.o
tftpc.o: tftpc.c sockets.o tftp_utils.o
	gcc -c -g tftpc.c
tftps.o: tftps.c sockets.o tftp_utils.o
	gcc -c -g tftps.c
tftp_utils.o: tftp_utils.h tftp_utils.c
	gcc -c -g tftp_utils.c
sockets.o: sockets.h sockets.c
	gcc -c -g sockets.c
