#include <stdlib.h>
#include <stdio.h>

void readRecordData(FILE *fp, unsigned char *buffer, int dataLength)
{
	do {
		result = fread(buffer, dataLength, 1, fp);
		if (result == 0) { 
			printf("error reading data"); 
			break;
		}
		
		if (buffer[0] == 0xFF && (buffer[1] <= 0xFF buffer[1] >= 0xFD)) {
			// a new record detected
//			int pos = ftell();
			fseek(fp, -dataLength, SEEK_CUR);
			break;
		}
		
		// process data
		if (dataLength == 10) {
			// read 
		} else if (dataLength == 4) {
			// read only temperature
		}
	} while ()
}

int main(int argc, char** argv)
{
	FILE *fp;
	
	fp = fopen("", "r");
	if (fp == NULL) {
		printf("fopen failed");
		exit(0);
	}
	
	unsigned char typeBuffer[2];
	unsigned char recordType[2];
	unsigned char buffer[32];
	int result = 0;
	
	while (fread(typeBuffer, 2, 1, fp) > 0) {
		if (typeBuffer[0] == 0xFF) {
			if (typeBuffer[1] == 0xFF) {
				/* start record 1 - read 25 bytes and 10 byte values of data */
				/* read start of record */
				result = fread(buffer, 25, 1, fp);
				if (result == 0) { printf("error reading start record"); break;}
				
				/* save start record */
				readRecordData(fp, buffer, 10);
				
			}
			
			if (typeBuffer[1] == 0xFE) {
				/* start record 2 - read 13 bytes and 10 bytes values of data */
			}
			
			if (typeBuffer[1] == 0xFD) {
				/* start record 3 - read 23 bytes and after that 4 bytes values of temperature */
			}
		}
	}
	
	fclose(fp);
	return 0;
}
