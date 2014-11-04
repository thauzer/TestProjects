#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "evt_poll.h"
#include "socketinit.h"

int evt_fd = -1;

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
	printf("%s - fd: %d\n", __PRETTY_FUNCTION__, fd);
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

int evt_loop(int serverfd)
{
#define EV_SIZE 8
	struct epoll_event events[EV_SIZE];

	int numEvt, i;
	int ret;
	int done = 0;
	while (1)
	{
		printf("Start event loop\n");
		numEvt = epoll_wait(evt_fd, events, EV_SIZE, -1);
		
		for (i=0; i < numEvt; i++)
		{
			if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP || !(events[i].events & EPOLLIN))
			{
				printf("epoll error\n");
				close(events[i].data.fd);
				continue;
			}
			else if (serverfd == events[i].data.fd) /* event on listening file descriptor */
			{
				while (1)
				{
					struct sockaddr in_addr;
					socklen_t in_len;
					in_len = sizeof(in_addr);
					
					int accept_fd = accept(events[i].data.fd, &in_addr, &in_len);
					if (accept_fd < 0) {
						if (errno == EAGAIN || errno == EWOULDBLOCK) {
							break;
						}
						else {
							printf("accept error\n");
							break;
						}
					}
					
					char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
					ret = getnameinfo(&in_addr, in_len, 
									hbuf, sizeof hbuf, 
									sbuf, sizeof sbuf, 
									NI_NUMERICHOST | NI_NUMERICSERV);
					
					if (ret == 0) {
						printf("Accepted connection fd: %d, host: %s, port: %s\n", accept_fd, hbuf, sbuf);
					}
					
					make_socket_non_blocking(&accept_fd);
					add_event_poll(accept_fd);
				}
				continue;
			}
			else /* data on one of the others file desccriptors */
			{
				done = 0;
				while (1)
				{
					ssize_t numRead;
					char buffer[256];
				
					numRead = read(events[i].data.fd, buffer, 256);
					if (numRead < 0) {
						if (errno != EAGAIN) {
							printf("read error");
							done = 1;
						}
						break;
					}
					else if (numRead == 0) {
						done = 1;
						break;
					}
					
					// do something with the data, call function that processes data
					printf("%s\n", buffer);
				}
				
				if (done) {
					printf("Closed connection: %d\n", events[i].data.fd);
					close(events[i].data.fd);
				}
			}
		}
	}
}
