#include <stdlib.h>
#include <stdio.h>

void endian_swap16bit(unsigned short* x)
{
    *x = (*x>>8) | 
        (*x<<8);
}


void endian_swap32bit(unsigned int* x)
{
    *x = (*x>>24) | 
        ((*x<<8) & 0x00FF0000) |
        ((*x>>8) & 0x0000FF00) |
        (*x<<24);
}

void endian_swap64bit(unsigned long long* x)
{
    *x = (*x>>56) | 
        ((*x<<40) & 0x00FF000000000000ULL) |
        ((*x<<24) & 0x0000FF0000000000ULL) |
        ((*x<<8)  & 0x000000FF00000000ULL) |
        ((*x>>8)  & 0x00000000FF000000) |
        ((*x>>24) & 0x0000000000FF0000) |
        ((*x>>40) & 0x000000000000FF00) |
        (*x<<56);
}

unsigned int endian_swap16(unsigned short x)
{
	return (x>>8) | 
			(x<<8);
}

unsigned int endian_swap32(unsigned int x)
{
	return (x>>24) | 
    	    ((x<<8) & 0x00FF0000) |
    	    ((x>>8) & 0x0000FF00) |
	        (x<<24);
}

unsigned long long endian_swap64(unsigned long long x)
{
	return (x>>56) | 
    	    ((x<<40) & 0x00FF000000000000ULL) |
    	    ((x<<24) & 0x0000FF0000000000ULL) |
    	    ((x<<8)  & 0x000000FF00000000ULL) |
    	    ((x>>8)  & 0x00000000FF000000) |
    	    ((x>>24) & 0x0000000000FF0000) |
    	    ((x>>40) & 0x000000000000FF00) |
	        (x<<56);
}

int main(int argc, char** argv)
{
	if (argc != 2) {
		printf("args error");
		return -1;
	}

	unsigned short num16 = atoi(argv[1]);
	unsigned int num32 = num16;
	unsigned long long num64 = num32;

	printf("16bit - Num: %u, Hex: %04X\n", 	num16, num16);
	printf("32bit - Num: %u, Hex: %08X\n", 	num32, num32);
	printf("64bit - Num: %llu , ", num64);
	printf("Hex: %016llX\n", num64);
	
	unsigned short tmpnum16 = 0;
	unsigned int tmpnum32 = 0;
	unsigned long long tmpnum64 = 0;
	tmpnum16 = endian_swap16(num16);
	tmpnum32 = endian_swap32(num32);
	tmpnum64 = endian_swap64(num64);

	endian_swap16bit(&num16);
	endian_swap32bit(&num32);
	endian_swap64bit(&num64);

	printf("16bit - Num: %u, Hex: %04X\n", 	num16, num16);
	printf("32bit - Num: %u, Hex: %08X\n", 	num32, num32);
	printf("64bit - Num: %llu, Hex: %016llX\n", num64, num64);
	
	printf("16bit - Num: %u, Hex: %04X\n", 	tmpnum16, tmpnum16);
	printf("32bit - Num: %u, Hex: %08X\n", 	tmpnum32, tmpnum32);
	printf("64bit - Num: %llu, Hex: %016llX\n", tmpnum64, tmpnum64);
	return 0;
}
