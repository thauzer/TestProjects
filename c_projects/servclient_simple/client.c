#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

int check_websocket_response(char* response)
{
	char* pos = strstr(response, "Sec-WebSocket-Accept");
	if (pos) {
		return 0;
	}
	return 1;
}

void build_websocket_handshake(char* data)
{
	/* build websocket handshake */
	char* hs_get = "GET /chat HTTP/1.1\r\n";
	char* hs_host = "Host: www.example.com\r\n";
	char* hs_upgrade = "Upgrade: Websocket\r\n";
	char* hs_conn = "Connection: Upgrade\r\n";
	char* hs_key = "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n";
	char* hs_origin = "Origin: http://example.com\r\n";
	char* hs_protocol = "Sec-WebSocket-Protocol: chat, superchat\r\n";
	char* hs_version = "Sec-WebSocket-Version: 13\r\n";
	char* hs_device_id = "X-Nomnio-DeviceKey: asddfsdsfgsfghbvrttghhf\r\n";
	char* hs_sub_key = "X-Nomnio-SubscriptionKey: asdfdsdf34rg3dggv34dg\r\n";
	
	sprintf(data, "%s%s%s%s%s%s%s%s%s%s\r\n", hs_get, hs_host, hs_upgrade, hs_conn,
			hs_key, hs_protocol, hs_version, hs_origin, hs_device_id, hs_sub_key);
	printf("handshake_data build\n");
}

int main(int argc, char** argv)
{
	if (argc != 3) {
		printf("Number of arguments is not correct!!!!\n");
		printf("Usage: programName host portNr\n");
		exit(EXIT_FAILURE);
	}

	char *host = argv[1];
	int portnr = atoi(argv[2]);
	int sockfd;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		printf("socket failed\n");
		return -1;
	}

	server = gethostbyname(host);
	if (server == NULL) {
		printf("no such host\n");
		close(sockfd);
		return -1;
	}

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portnr);
	bcopy((char *)server->h_addr,
      		(char *)&serv_addr.sin_addr.s_addr,
		server->h_length);

	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) {
		printf("connect error: %s\n", strerror(errno));
		close(sockfd);
		return -1;
	}
	
	char* tx_data = malloc(512);
	char* rx_data = malloc(512);
	memset(rx_data, '\0', 512);
	
	build_websocket_handshake(tx_data);
	int length = strlen(tx_data);

	write(sockfd, tx_data, length);
	
	read(sockfd, rx_data, 512);
	printf("message: %s\n", rx_data);
		
	while(1) {
		printf("reading\n");
		memset(rx_data, '\0', 512);
		read(sockfd, rx_data, 512);
		printf("message: %s\n", rx_data);
		sleep(500);
	}
		
	free(tx_data);
	free(rx_data);

	close(sockfd);
	
	return 0;
}

#if 0
char buf[256];
memset(buf, '\0', 256);
fgets(buf, 256, stdin); 
#endif