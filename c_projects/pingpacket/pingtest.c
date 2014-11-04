#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/time.h>
#include <arpa/inet.h>

u_short in_cksum(u_short *addr, int len);

extern int errno;

int net_socket = -1;
int packet_socket = -1;

int main(int argc, char** argv)
{	
	// socke tfor getting the hw address
	net_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (net_socket < 0) {
		printf("Error opening: %d, %s\n", errno, strerror(errno));
		return -1;
	}
	
	unsigned char MAC[ETH_ALEN];
	
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);
	
	if (ioctl(net_socket, SIOCGIFHWADDR, &ifr)) {
		printf("Error getting hw address: %d, %s\n", errno, strerror(errno));
		close(net_socket);
		return -1;
	}
	memcpy(MAC, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
	
	printf("MAC: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);

// icmp packet sending
	packet_socket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (packet_socket < 0) {
		printf("Packet socket open error: %d, %s\n", errno, strerror(errno));
		close(net_socket);
		return -1;
	}

	unsigned char buf[1500];
	memset(buf, 0, 1500);
	struct ip* ip_header = (struct ip*) buf;
	struct icmp* icmp_packet = (struct icmp*) (ip_header + 1);
	int icmp_len;
	struct sockaddr_in destination;
	
	printf("ip: %p, icmp: %p\n", ip_header, icmp_packet);
	printf("sizeof ip_header: %d, sizeof *ip_header >> 2: %d, sizeof icmp_packet: %d\n", sizeof *ip_header, sizeof *ip_header >> 2, sizeof *icmp_packet);
	
	ip_header->ip_hl 	= 5;
	ip_header->ip_v 	= 4;
	ip_header->ip_tos 	= 0x00;
	ip_header->ip_len 	= sizeof *ip_header >> 2;
	ip_header->ip_id 	= htons(3432);
	ip_header->ip_off 	= 0;
	ip_header->ip_ttl 	= 255;
	ip_header->ip_p 	= 1;
	ip_header->ip_sum 	= 0;
/*	ip_header->ip_src 	= NULL;
	
	*/
	icmp_packet->icmp_seq	= 1;
	icmp_packet->icmp_type	= ICMP_TSTAMP;
	icmp_packet->icmp_code	= 0;
	icmp_packet->icmp_cksum	= 0;
	
	gettimeofday((struct timeval *) (icmp_packet+8), NULL);
	memset(icmp_packet+12, 0, 8);
	icmp_len = 20;
	
	ip_header->ip_len = sizeof(struct ip) + icmp_len; 
	
	struct in_addr tmpaddr;
	tmpaddr.s_addr = inet_addr("192.168.0.126");
	destination.sin_family = AF_INET;
	ip_header->ip_dst.s_addr = tmpaddr.s_addr;
	destination.sin_addr = tmpaddr;
	icmp_packet->icmp_cksum = in_cksum((u_short *) icmp_packet, icmp_len);
	
	if (sendto(packet_socket, buf, ip_header->ip_len, 0, (struct sockaddr*) &destination, sizeof(destination)) < 0) {
		printf("Packet send error: %d, %s\n", errno, strerror(errno));
	}
	
// closing of sockets
	if (close(net_socket) < 0) {
		printf("Socket close error: %d, %s\n", errno, strerror(errno));
	}
	if (close(packet_socket) < 0) {
		printf("Socket close error: %d, %s\n", errno, strerror(errno));
	}
	return 0;
}

/*
 * in_cksum --
 *	Checksum routine for Internet Protocol family headers (C Version)
 *      From FreeBSD's ping.c
 */

u_short
in_cksum(addr, len)
	u_short *addr;
	int len;
{
	register int nleft = len;
	register u_short *w = addr;
	register int sum = 0;
	u_short answer = 0;

	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1) {
		*(u_char *)(&answer) = *(u_char *)w ;
		sum += answer;
	}

	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return(answer);
}
