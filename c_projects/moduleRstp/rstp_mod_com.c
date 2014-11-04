#include "mod_rstp.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

void change_port(const int fd)
{
	int ret;

	SW_PORT_CHANGE_T portChange;
	portChange.portNo = 234;
	portChange.portState = SW_PS_FORWARDING;

	ret = ioctl(fd, CMD_CHANGE_PORT, &portChange);

	if (ret < 0) {
		printf("ioctl - change_port failed:%d\n", ret);
		exit(-1);
	}
}

void flush_addresses(const int fd)
{
	int ret;

	int portNum = 987;

	ret = ioctl(fd, CMD_FLUSH_ADDRESSES, portNum);

	if (ret < 0) {
		printf("ioctl - flush_addresses failed:%d\n", ret);
		exit(-1);
	}
}

int main(int argc, char** argv)
{
	int fd;
	fd = open(DEVICE_FILE_NAME, 0);

	if (fd < 0) {
		printf("Can't open device file: %s\n", DEVICE_FILE_NAME);
		switch (errno) {
		case EACCES:  printf ("Permission denied. Errno: %d\n", errno); break;
 /*     	case EINVACC: printf ("Invalid access mode.\n"); break; */
		case EMFILE:  printf ("No file handle available. Errno: %d\n", errno); break;
		case ENOENT:  printf ("File or path not found. Errno: %d\n", errno); break;
		default:      printf ("Unknown error. Errno: %d\n", errno); break;
    		}

		exit(-1);
	}

	change_port(fd);
	flush_addresses(fd);

	close(fd);
}
