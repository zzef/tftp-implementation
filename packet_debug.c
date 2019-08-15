#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char to_hex(char h){
	
	switch(h){
		case(0):return '0';
		case(1):return '1';
		case(2):return '2';
		case(3):return '3';
		case(4):return '4';
		case(5):return '5';
		case(6):return '6';
		case(7):return '7';
		case(8):return '8';
		case(9):return '9';
		case(10):return 'a';
		case(11):return 'b';
		case(12):return 'c';
		case(13):return 'd';
		case(14):return 'e';
		case(15):return 'f';
	}
	
}

char* conv_to_hex(char chr) {
	char l4_bits = (chr >> 4) & ((1 << 4)-1);
	char r4_bits = chr & ((1 << 4)-1);
	char hex[] ={'\\','x',to_hex(l4_bits),to_hex(r4_bits),'\0'};
	char* hx = malloc(4);
	memcpy(hx,hex,4);
	return hx;
	//need to create string from this 
}

void print_pkt_data(char* pkt, int size) {
	for (int i=0; i<size; i++) {
		char chr = pkt[i];
		if (chr>=32 && chr<=126) {
			printf("%c",chr);
			continue;
		}
		printf("%s",conv_to_hex(chr));
	}
}
