#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <strings.h>

int evt_fd = -1;
int listen_socket = -1;

struct poll_event_handler {
	int fd; /* File descriptor to be watched */
	void *arg; /* argument for the handler */
	void (*handler) (uint32_t events, struct poll_event_handler *p) /* function to be called on data receive */
	struct epoll_event *ref_ev;
}

int init_event_poll(void)
{
	int ret;
	ret = epoll_create(1);
	if (ret < 0) {
		printf("epoll_create failed: %d\n", errno);
		return -1;
	}
	evt_fd = ret;
	return 0;
}

int close_event_poll(void)
{
	int ret;
	if (evt_fd >= 0) {
		ret = close(evt_fd);
		if (ret < 0) {
			printf("close evt_fd failed: %d\n", errno);
			return -1;
		}
	}
	return 0;
}

int add_event_poll(int fd)
{
	struct epoll_event ev = {
		.events = EPOLLIN,
		.data.fd = fd,
	};
	int ret;
	ret = epoll_ctl(evt_fd, EPOLL_CTL_ADD, fd, &ev);
	if (ret < 0) {
		printf("epoll_ctl error");
		return -1;
	}
	return 0;
}

int evt_loop(int serversock)
{
#define EV_SIZE 8
	struct epoll_event events[EV_SIZE];

	printf("evt_fd: %d, serversock: %d\n", evt_fd, serversock);

	int ret, i;
	char c;
	while (1)
	{
		ret = epoll_wait(evt_fd, events, EV_SIZE, 1000);
		if (ret < 0) {
			printf(" epoll_wait error\n");
			return -1;
		}

		for (i = 0; i < ret; i++)
		{
			if (events[i].data.fd == serversock) {
				printf("event=%X on fd=%d  ", events[i].events, events[i].data.fd);
				if (read(events[i].data.fd, c, 1))
					printf("%c\n", c);
			}
		}	
		sleep(1);
	}
}

int newServerSocket = -1;

int main(int argc, char** argv)
{
	init_event_poll();

	int ret;
	struct sockaddr_in serv_addr, cli_addr;
	int serverfd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverfd < 0) {
		printf("socket failed\n");
		return -1;
	}
	
	bzero((char *)&serv_addr, sizeof(serv_addr));

	int portno = 7463;
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(serverfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		close(serverfd);
		return -1;
	}
	listen(serverfd, 5);

	socklen_t clilen;
	clilen = sizeof(cli_addr);
	newServerSocket = accept(serverfd, (struct sockaddr*) &cli_addr, &clilen);
	printf("newServerSocket: %d\n", newServerSocket);

	if (newServerSocket < 0) {
		close(serverfd);
		return -1;
	}

	if (add_event_poll(newServerSocket))
		return -1;

	evt_loop(newServerSocket);

	close(serverfd);

	return 0;
}
