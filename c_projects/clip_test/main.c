#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clip_string.h"

int main(int argc, char** argv)
{
	printf("Program started\n");
	
	printf("\n----------test 1---------------\n");
	int str_size = 64;
//	char* string1 = (char *)malloc(str_size);
	char string1[str_size+1];
	char* string2;// = (char *)malloc(str_size);
	
	memset(string1, '\0', str_size);
	strcpy(string1, "abcde\0");
	string2 = NULL;
	
	printf("pointer &string1[0]: %p, sizeof: %d\n", &string1[0], sizeof(&string1[0]));
	printf("pointer string1: %p, sizeof: %d\n", string1, sizeof(string1));
	printf("pointer string2: %p, sizeof: %d\n", string2, sizeof(string2));
	
	string2 = clipString(string1);
	
	printf("pointer &string1[0]: %p, sizeof: %d\n", &string1[0], sizeof(&string1[0]));
	printf("pointer string1: %p, sizeof: %d\n", string1, sizeof(string1));
	printf("pointer string2: %p, sizeof: %d\n", string2, sizeof(string2));
	
	printf("\n----------test 2---------------\n");
	memset(string1, '\0', str_size);
	strcpy(string1, "  aaaaa\n\0");
	string2 = NULL;

	printf("pointer &string1[0]: %p, sizeof: %d\n", &string1[0], sizeof(&string1[0]));
	printf("pointer string1: %p, sizeof: %d\n", string1, sizeof(string1));
	printf("pointer string2: %p, sizeof: %d\n", string2, sizeof(string2));
	
	string2 = clipString(string1);
	
	printf("pointer &string1[0]: %p, sizeof: %d\n", &string1[0], sizeof(&string1[0]));
	printf("pointer string1: %p, sizeof: %d\n", string1, sizeof(string1));
	printf("pointer string2: %p, sizeof: %d\n", string2, sizeof(string2));
	
	printf("\n----------test 3---------------\n");
	memset(string1, '\0', str_size);
	strcpy(string1, "   bbbbbbbbb\n\0");
	string2 = NULL;
	
	printf("pointer &string1[0]: %p, sizeof: %d\n", &string1[0], sizeof(&string1[0]));
	printf("pointer string1: %p, sizeof: %d\n", string1, sizeof(string1));
	printf("pointer string2: %p, sizeof: %d\n", string2, sizeof(string2));
	
	string2 = clipString(string1);
	
	printf("pointer &string1[0]: %p, sizeof: %d\n", &string1[0], sizeof(&string1[0]));
	printf("pointer string1: %p, sizeof: %d\n", string1, sizeof(string1));
	printf("pointer string2: %p, sizeof: %d\n", string2, sizeof(string2));
	
	printf("Program exited\n");
	return 0;
}
