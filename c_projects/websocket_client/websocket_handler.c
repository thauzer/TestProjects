#include "websocket_handler.h"
#include "wiced.h"
#include "debug.h"
#include "common.h"
#include "nomnio_wifimodul_dct.h"

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
    char hs_device_id[70];
    sprintf(hs_device_id, "X-Nomnio-DeviceKey:%s\r\n", ((nomnio_test_app_dct_t*)wiced_dct_get_app_section())->device_id);
    char hs_sub_key[70];
    sprintf(hs_sub_key, "X-Nomnio-SubscriptionKey:%s\r\n", ((nomnio_test_app_dct_t*)wiced_dct_get_app_section())->subscription_key);

    sprintf(data, "%s%s%s%s%s%s%s%s%s%s\r\n", hs_get, hs_host, hs_upgrade, hs_conn,
            hs_key, hs_protocol, hs_version, hs_origin, hs_device_id, hs_sub_key);
    DEBUG("handshake_data build\r\n");
}

wiced_result_t handle_websocket_handshake(wiced_tcp_socket_t* socket)
{
    wiced_result_t result;
    /* try connecting to the websocket server */
    wiced_packet_t* packet;
    char*           tx_data;
    uint16_t        available_data_length;

    wiced_packet_t* rx_packet;
    char*           rx_data;
    uint16_t        rx_data_length;

    if (wiced_packet_create_tcp(socket, MAX_PACKET_DATA_LENGTH, &packet, (uint8_t**)&tx_data,
            &available_data_length) != WICED_SUCCESS) {
        DEBUG("TCP packet creation failed\r\n");
        wiced_tcp_disconnect(socket); return WICED_ERROR;
    }
    DEBUG("packet created\r\n");

    build_websocket_handshake(tx_data);
    wiced_packet_set_data_end(packet, (uint8_t*)tx_data + strlen(tx_data));

    /* send websocket handshake */
    result = wiced_tcp_send_packet(socket, packet);
    if (result != WICED_SUCCESS) {
        WPRINT_APP_INFO(("TCP packet send failed\r\n")); DEBUG("Send failed - error:\r\n");
        wiced_packet_delete(packet); wiced_tcp_disconnect(socket); return WICED_ERROR;
    }

    /* receive websocket handshake response */
    result = wiced_tcp_receive(socket, &rx_packet, WICED_NEVER_TIMEOUT);
    if( result != WICED_SUCCESS ) {
        sprintf(debug_msg, "handshake reception failed - error: %d\r\n", result); DEBUG(debug_msg);
        wiced_packet_delete(rx_packet); wiced_tcp_disconnect(socket); return WICED_ERROR;
    }

    wiced_packet_get_data(rx_packet, 0, (uint8_t**)&rx_data, &rx_data_length, &available_data_length);
    rx_data[rx_data_length] = '\x0';
    DEBUG("something received\r\n");
    sprintf(debug_msg2, "length:%d\r\n%s", rx_data_length, rx_data);
    DEBUG(debug_msg2);

    if (check_websocket_response(rx_data)) {
        DEBUG("websocket response not valid\r\n");
        wiced_packet_delete(rx_packet); wiced_tcp_disconnect(socket); return WICED_ERROR;
    }
    wiced_packet_delete(rx_packet);
    DEBUG("Websocket response received. Starting reading of data")
    return WICED_SUCCESS;
}

wiced_result_t send_websocket_data_message(wiced_tcp_socket_t* socket, char* msg)
{
    DEBUG("send_websocket_data_message\r\n");
    wiced_result_t result;
    int msg_len = strlen(msg);
    char send_msg_len = 0;
    char mask[4];
    int new_random = rand();
    memcpy(mask, &new_random, 4);
    int  nr_packets = 1;
    if (msg_len > MAX_WS_FRAME_SIZE) {
        nr_packets = (msg_len/MAX_WS_FRAME_SIZE)+1;
    }

    wiced_packet_t* packet;
    char*           tx_data;
    uint16_t        available_data_length;

    int i=0;
    sprintf(debug_msg, "nr: %d\r\n", nr_packets);
    DEBUG(debug_msg);

    int mask_index = 0;
    for (i=0; i < msg_len; i++) {
        mask_index = i % 4;
        msg[i] = msg[i] ^ mask[mask_index];
    }

    for (i=1; i <= nr_packets; i++ ) {
        DEBUG("send packet\r\n");
        if (wiced_packet_create_tcp(socket, MAX_PACKET_DATA_LENGTH, &packet, (uint8_t**)&tx_data,
                &available_data_length) != WICED_SUCCESS) {
            DEBUG("TCP packet creation failed\r\n");
            wiced_tcp_disconnect(socket);
            return WICED_ERROR;
        }

        memset(tx_data, MAX_PACKET_DATA_LENGTH, 0x00);
        /* set opcode on first packet */
        if ( nr_packets == 1) { tx_data[0] |= 0x01; }
        /* set mask bit */
        tx_data[1] |= 0x80;

        DEBUG("send packet 1\r\n");
        /* set length to max if it is not the last packet */
        send_msg_len = 0;
        if (nr_packets != i) { send_msg_len |= MAX_WS_FRAME_SIZE; }

        if ( nr_packets == i ) {
            /* set FIN bit in last packet */
            tx_data[0] |= 0x80;
            /* set correct length on last packet */
            send_msg_len = msg_len % 0x7F;
        }
        sprintf(debug_msg2, "send_msg_len: %d\r\n", send_msg_len);
        DEBUG(debug_msg2);
        tx_data[1] |= send_msg_len;
        memcpy(tx_data+2, mask, 4);
        memcpy(tx_data+6, msg + ((i-1)*MAX_WS_FRAME_SIZE), send_msg_len);

        sprintf(debug_msg, "%d, %x, %u, %d, %x, %u", tx_data[0], tx_data[0], tx_data[0], tx_data[1], tx_data[1], tx_data[1]);
        DEBUG(debug_msg);
        wiced_packet_set_data_end(packet, (uint8_t*)tx_data + (send_msg_len + 6));

        result = wiced_tcp_send_packet(socket, packet);
        if (result != WICED_SUCCESS) {
            sprintf(debug_msg, "could not sent - error: %d\r\n", result); DEBUG(debug_msg);
            wiced_packet_delete(packet); wiced_tcp_disconnect(socket);
            return 1;
        }
    }

    return WICED_SUCCESS;
}
