#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "socketinit.h"
#include "common.h"

int init_socket(int* serverSock)
{
	struct sockaddr_in serv_addr;
	*serverSock = socket(AF_INET, SOCK_STREAM, 0);
	if (*serverSock < 0) {
		printf("socket failed\n");
		return -1;
	}
	
/*	bzero((char *)&serv_addr, sizeof(serv_addr)); */
	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORTNO);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(*serverSock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		close(*serverSock);
		return -1;
	}
	
	return 0;

}

int make_socket_non_blocking(int* sock)
{
	int ret;
	int flags;
	flags = fcntl(*sock, F_GETFL, 0);
	if (flags < 0) {
		printf("%s Error: %d - %s\n", __PRETTY_FUNCTION__, errno, strerror(errno));
		return -1;
	}
	
	flags |= O_NONBLOCK;
	ret = fcntl(*sock, F_SETFL, flags);
	if (ret < 0) {
		printf("%s Error: %d - %s\n", __PRETTY_FUNCTION__, errno, strerror(errno));
		return -1;
	}
	
	return 0;
}
