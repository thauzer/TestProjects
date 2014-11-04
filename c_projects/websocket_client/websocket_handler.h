#ifndef WEBSOCKET_HANDLER_H_
#define WEBSOCKET_HANDLER_H_

#include "wiced.h"

int send_websocket_data_message(wiced_tcp_socket_t* socket, char* msg);

#endif
