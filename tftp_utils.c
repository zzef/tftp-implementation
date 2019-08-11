#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*

	Opcodes
	-------

	opcode   operation
	  1      Read Request (RRQ)
	  2      Write Request (WRQ)
	  3      Data (DATA)
      4      Acknowledgement (ACK)
      5  	 Error (ERROR)


*/


//This function concatenates characters
char* concat(const char* a, int size_a,
			 const char* b, int size_b, 
		 	 int* len) {

	char* final = malloc(size_a+size_b);
	memcpy(final,a,size_a);
	memcpy(final+size_a,b,size_b); //+1 for terminating character '\0'
	*len=size_a+size_b;
	return final;
}

int bake_rq_pkt(const char rq, const char* file_name, 
					const char* mode, 
					char** wrq){
	
/*


	RRQ/WRQ Packets
	--------------

	   2 bytes     string     1 byte   string  1 byte
	 -------------------------------------------------
	|  opcode  |  file name  |   0   |  mode  |   0   |                   
	 -------------------------------------------------
	

*/

	int length=0;
	switch(rq){
		case('r'): *wrq = "\0\1"; break;
		case('w'): *wrq = "\0\2"; break;
	}	
	*wrq = concat(*wrq,2,file_name,strlen(file_name)+1,&length);
	*wrq = concat(*wrq,length,mode,strlen(mode)+1,&length);

	return length;
}



