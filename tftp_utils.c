#include <string.h>
#include <stdlib.h>

char* concat(char* a, int size_a,
			 char* b, int size_b, 
		 	 int* len) {

	char* final = malloc(size_a+size_b);
	memcpy(final,a,size_a);
	memcpy(final+size_a,b,size_b); //+1 for terminating character '\0'
	*len=size_a+size_b;
	return final;
}

int bake_wrq_pkt(char* file_name, 
					char* mode, 
					char** wrq){
	
	int length=0;
	*wrq = "\x00\x02"; 
	*wrq = concat(*wrq,2,file_name,strlen(file_name)+1,&length);
	*wrq = concat(*wrq,length,mode,strlen(mode)+1,&length);
	printf("%i\n",length);

	return length;
}
