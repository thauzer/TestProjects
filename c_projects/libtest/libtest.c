#include <stdlib.h>
#include <stdio.h>

#include <sys/time.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	int nums[] = {5, 12, 456, 2};
	int size = sizeof(nums) / sizeof(int);
	
	struct timeval start, end;

    long mtime, seconds, useconds;    

    gettimeofday(&start, NULL);
    usleep(2000);
    gettimeofday(&end, NULL);

    seconds  = end.tv_sec  - start.tv_sec;
    useconds = end.tv_usec - start.tv_usec;

    mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;

    printf("Elapsed time: %ld milliseconds\n", mtime);
	
	int i;
	for (i=0; i < size; i++) {
		printf("Num %u: >%3u<\n", i, nums[i]);
	}
	if (!(1 || 1))
		printf("1 || 1\n");
		
	if (!(0 || 1))
		printf("0 || 1\n");
		
	if (!(1 || 0))
		printf("1 || 0\n");
		
	if (!(0 || 0))
		printf("0 || 0\n");
	return 0;
}
