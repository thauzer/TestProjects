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
#include <time.h>

#include "command_handler.h"

#define MAX_WS_FRAME_SIZE 126
#define BUFFER_SIZE 512

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

int send_websocket_data_message(int socket, char* msg)
{
    printf("send_websocket_data_message\r\n");
    int result = 0;;
    int msg_len = strlen(msg);
    unsigned char send_msg_len = 0;
    char mask[4];
    int new_random = rand();
    memcpy(mask, &new_random, 4);
	
	printf("message length: %d\n", msg_len);

    char* tx_data = malloc(BUFFER_SIZE);
	memset(tx_data, BUFFER_SIZE, 0);
	
	tx_data[0] |= 0x80; /* set fin bit */
	tx_data[0] |= 0x01; /* set text frame */
	tx_data[1] |= 0x80; /* set mask bit */
	int start_offset = 0;
	
	if (msg_len <= 125) {
		send_msg_len = (unsigned char)msg_len;
		printf("send_msg_len: %d, %x, msg_len: %d, %x\n", send_msg_len, send_msg_len, msg_len, msg_len);
		tx_data[1] |= send_msg_len;
		start_offset = 2;
	} else if (msg_len > 125) {
		send_msg_len = 126;
		printf("send_msg_len: %d, %x, msg_len: %d, %x\n", send_msg_len, send_msg_len, msg_len, msg_len);
		tx_data[1] |= send_msg_len;
		tx_data[2] |= (unsigned char)(msg_len >> 8);
		tx_data[3] |= (unsigned char)msg_len;
		start_offset = 4;
	}
	memcpy(tx_data+start_offset, mask, 4);
	start_offset += 4;

	int i;
    int mask_index = 0;
    for (i=0; i < msg_len; i++) {
        mask_index = i % 4;
        msg[i] = msg[i] ^ mask[mask_index];
	}
	
	memcpy(tx_data+start_offset, msg, send_msg_len);

	printf("%d, %x, %u, %d, %x, %u\n", (unsigned char)tx_data[0], (unsigned char)tx_data[0], (unsigned char)tx_data[0], (unsigned char)tx_data[1], (unsigned char)tx_data[1], (unsigned char)tx_data[1]);

	result = write(socket, tx_data, msg_len + start_offset);

    return result;
}

int main(int argc, char** argv)
{
	if (argc != 3) {
		printf("Number of arguments is not correct!!!!\n");
		printf("Usage: programName host portNr\n");
		exit(EXIT_FAILURE);
	}
	
	srand(time(NULL));

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
	
	char* tx_data = malloc(BUFFER_SIZE);
	char* rx_data = malloc(BUFFER_SIZE);
	memset(rx_data, '\0', BUFFER_SIZE);
	
	build_websocket_handshake(tx_data);
	int length = strlen(tx_data);

	write(sockfd, tx_data, length);
	
	read(sockfd, rx_data, BUFFER_SIZE);
	printf("message:\n %s\n", rx_data);
	char* response_json = NULL;
/*	char* tmp_buffer = malloc(BUFFER_SIZE);*/
		
	while(1) {
		printf("reading\n");
		memset(rx_data, '\0', BUFFER_SIZE);
		read(sockfd, rx_data, BUFFER_SIZE);
		printf("message, rx_data[0]: %x, rx_data[1]: hex:%x, dec: %u\n%s\n", (unsigned char)rx_data[0], (unsigned char)rx_data[1], (unsigned char)rx_data[1], rx_data);
		
		unsigned char b1 = rx_data[0];
		unsigned int fin = (b1 & 0x80) ? 1 : 0;
		unsigned int opcode = (b1 & 0x0f);
		
		printf("b1: %02x, fin bit: %d, opcode: 0x%02x\n", b1, fin, opcode);
			
		switch (opcode) {
			case 0x01: /* text frame */
			{
				printf("text frame\n");
				
				response_json = parse_json_command(rx_data+2);
				char* start_json = strchr(rx_data, '{');
				char* end_json = strrchr(rx_data, '}');
				printf("length: %d, response:\n%s\n", (int)(end_json-start_json), response_json);
		
				send_websocket_data_message(sockfd, response_json);
				break;
			}
			case 0x02: /* binary frame */
			{
				printf("binary frame\n");
				break;
			}
			case 0x08: /* connection close */
			{
				printf("connection close frame\n");
				return 1;
			}
		}
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