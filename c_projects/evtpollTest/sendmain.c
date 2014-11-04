#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

int main(int argc, char** argv)
{
	int sockfd, portno;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	portno = 7463;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		printf("socket failed\n");
		return -1;
	}

	server = gethostbyname("localhost");
	if (server == NULL) {
		printf("no such host\n");
		close(sockfd);
		return -1;
	}

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	bcopy((char *)server->h_addr,
      		(char *)&serv_addr.sin_addr.s_addr,
		server->h_length);

	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) {
		printf("connect error");
		close(sockfd);
		return -1;
	}

	write(sockfd, "a", 1);

	close(sockfd);
	
	return 0;
}
