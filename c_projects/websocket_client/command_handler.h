#ifndef COMMAND_HANDLER_H_
#define COMMAND_HANDLER_H_

#include "cJSON.h"

/* header message types */
#define DATA_STR "Data"
#define FAULT_STR "Fault"
#define ADMIN_STR "Administration"
#define STATUS_STR "Status"

/* header actions (Only for administration messages) */
#define UPDATEFIRMWARE_STR "UpdateFirmware"
#define REBOOT_STR "Reboot"
#define SYNC_STR "Sync"
#define REPORTSTATUS_STR "ReportStatus"

#define MAX_PACKET_DATA_LENGTH      256
#define WS_SERVER_PORT              1234
#define WS_CLIENT_CONNECT_TIMEOUT   5000
#define WS_CLIENT_RECEIVE_TIMEOUT   1000

char* parse_json_command(char* msg);

char* buildStatusResponse(char* msg);
void buildErrorMessage(char* msg, char* error_code, char* error_msg);

#endif
