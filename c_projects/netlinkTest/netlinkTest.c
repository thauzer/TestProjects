#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <asm/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#define MYPROTO NETLINK_ROUTE
#define MYRTMGROUP RTMGRP_IPV4_ROUTE

struct msgnames_t {
        int id;
        char *msg;
} typenames[] = {
#define MSG(x) { x, #x }
        MSG(RTM_NEWROUTE),
        MSG(RTM_DELROUTE),
        MSG(RTM_GETROUTE),
#undef MSG
        {0,0}
};

char *lookup_name(struct msgnames_t *db,int id)
{
        static char name[512];
        struct msgnames_t *msgnamesiter;
        for(msgnamesiter=db;msgnamesiter->msg;++msgnamesiter) {
                if (msgnamesiter->id == id)
                        break;
        }
        if (msgnamesiter->msg) {
                return msgnamesiter->msg;
        }
        snprintf(name,sizeof(name),"#%i",id);
        return name;
}

int open_netlink_socket()
{
	int s = socket(AF_NETLINK, SOCK_RAW, MYPROTO);
	if (s < 0) 
		return s;
		
	struct sockaddr_nl nladdr;
	memset((void *)&nladdr, 0, sizeof(nladdr));
	
	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid = getpid();
	nladdr.nl_groups = MYRTMGROUP;
	
	if (bind(s, (struct sockaddr *)&nladdr, sizeof(nladdr))) {
		return -1;
	}
	
	return s;
}

int read_event(int nl_socket)
{
	struct sockaddr_nl nladdr;
	struct msghdr msg;
	struct iovec iov[2];
	struct nlmsghdr nlh;
	char buffer[65536];
	int ret;
	iov[0].iov_base = (void*)&nlh;
	iov[0].iov_len = sizeof(nlh);
	iov[1].iov_base = (void*)&buffer;
	iov[1].iov_len = sizeof(buffer);
	msg.msg_name = (void*)&(nladdr);
	msg.msg_namelen = sizeof(nladdr);
	msg.msg_iov = iov;
	msg.msg_iovlen = sizeof(iov) / sizeof(iov[0]);
	
	ret = recvmsg(nl_socket, &msg, 0);
	if (ret < 0) {
		printf("recvmsg error: %d, %s\n", errno, strerror(errno));
		return ret;
	}
	
	printf("Type: %i (%s)\n",(nlh.nlmsg_type),lookup_name(typenames,nlh.nlmsg_type));
    printf("Flag:");
#define FLAG(x) if (nlh.nlmsg_flags & x) printf(" %s",#x)
    FLAG(NLM_F_REQUEST);
    FLAG(NLM_F_MULTI);
    FLAG(NLM_F_ACK);
    FLAG(NLM_F_ECHO);
    FLAG(NLM_F_REPLACE);
    FLAG(NLM_F_EXCL);
    FLAG(NLM_F_CREATE);
    FLAG(NLM_F_APPEND);
#undef FLAG
    printf("\n");
    printf("Seq : %i\n",nlh.nlmsg_seq);
    printf("Pid : %i\n",nlh.nlmsg_pid);
    printf("\n");
    return 0;
}

int main(int argc, char** argv)
{
	int netlink_socket;
	netlink_socket = open_netlink_socket();
	if (netlink_socket < 0) {
		printf("Opening netlink socket failed\n");
		return -1;
	}
	
	printf("Netlink socket opened\n");
	while(1) {
		read_event(netlink_socket);
	}
	return 0;
}
