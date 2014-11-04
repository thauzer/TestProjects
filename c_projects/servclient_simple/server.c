#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>

#include "socketinit.h"
#include "evt_poll.h"

int main(int argc, char** argv)
{
	int serverfd;
	int ret;
	
	/* server socket init */
	ret = init_socket(&serverfd);
	if (ret < 0) {
		printf("Init server socket failed\n");
		return -1;
	}
	ret = make_socket_non_blocking(&serverfd);
	if (ret < 0) {
		printf("Make socket non blocking failed\n");
		goto closeServSock;
	}
	
	ret = listen(serverfd, SOMAXCONN);
	if (ret < 0) {
		printf("Listen socket failed\n");
		goto closeServSock;
	}

	/* init event poll */
	if (init_event_poll() < 0) {
		printf("Event poll init failed");
		goto closeServSock;
	}
	
	add_event_poll(serverfd);
	
	evt_loop(serverfd);

	close_event_poll();
closeServSock:
	close(serverfd);

	return 0;
}
