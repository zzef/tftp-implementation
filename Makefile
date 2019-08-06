tftp-server: tftps.o sockets.o
	gcc -o tftp-server tftps.o sockets.o
tftps.o: tftps.c sockets.h
	gcc -c -g tftps.c
sockets.o: sockets.h sockets.c
	gcc -c -g sockets.c
