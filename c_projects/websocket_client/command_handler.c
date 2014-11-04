
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "command_handler.h"

char* parse_json_command(char* in_msg)
{
	printf("parse_json_command\n");
	printf("message: \n %s\n", in_msg);
    char* response = NULL;
	printf("before json parse\n");
    cJSON* root = cJSON_Parse(in_msg);
	printf("after json parse\n");
	
    cJSON *header, *body;

    header = cJSON_GetObjectItem(root, "Header");
    body = cJSON_GetObjectItem(root, "Body");
	printf("header: %p, body: %p\n", header, body);

	printf("before header\n");
    if (header != NULL) {
		printf("header found\n");
        cJSON* msg_type = cJSON_GetObjectItem(header, "MessageType");
        cJSON* msg_action = cJSON_GetObjectItem(header, "Action");

        if (msg_type != NULL) {
			printf("message type found\n");
            if (strcmp(msg_type->valuestring, DATA_STR) == 0) { /* settings registers on WCD platform */
                printf("Data message\r\n");
                int i;
                for (i=0; i < cJSON_GetArraySize(body); i++) {
                    cJSON* reg = cJSON_GetArrayItem(body, i);
                    printf("reg: %s - value: %s\r\n", reg->string, reg->valuestring);
                }
            } else if (strcmp(msg_type->valuestring, FAULT_STR) == 0) { /* should NOT receive this, only sent to server */
                printf("Fault message\r\n");
            } else if (strcmp(msg_type->valuestring, STATUS_STR) == 0) { /* should NOT receive this, only sent to server */
                printf("Status message\r\n");
            } else if (strcmp(msg_type->valuestring, ADMIN_STR) == 0) { /*   */
                printf("Administration message\r\n");
                if (msg_action != NULL) {
                    if (strcmp(msg_action->valuestring, UPDATEFIRMWARE_STR) == 0) {
                        printf("Update firmware message\r\n"); /*  */
                    } else if (strcmp(msg_action->valuestring, REBOOT_STR) == 0) { /* set is configured flag */
                        printf("Reboot message - run configuration again\r\n");
                    } else if (strcmp(msg_action->valuestring, SYNC_STR) == 0) {
                        printf("Sync message - send all WDC register\r\n");
                    } else if (strcmp(msg_action->valuestring, REPORTSTATUS_STR) == 0) {
                        printf("Report status message\r\n");
                        response = buildStatusResponse(response);
                    }
                }
            }
        }
    }

    cJSON_Delete(root);

    return response;
}

char* buildStatusResponse(char* msg)
{
/*    cJSON *root;
	cJSON *header;
	cJSON *body;
    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "Header",   header=cJSON_CreateObject());
    cJSON_AddItemToObject(root, "Body",     body=cJSON_CreateObject());
    cJSON_AddStringToObject(header, "MessageType",      "Status");
	cJSON_AddStringToObject(body,   "FirmwareVersion",  "3.45.009");
	cJSON_AddStringToObject(body,   "FirmwareVersionUpdated",  "20-12-2013 12:26:11.201");
	cJSON_AddStringToObject(body,   "W",    "192");*/
/*	cJSON_AddStringToObject(body,   "WifiDhcp",         "true");
	cJSON_AddStringToObject(body,   "WifiWps",          "false");
	cJSON_AddStringToObject(body,   "WifiSignalStregth", "123");*/
/*	cJSON_AddStringToObject(body,   "WifiSecurityType", "WPA2_Mixed" );*/
/*
	msg=cJSON_PrintUnformatted(root);
    cJSON_Delete(root);*/

	msg = malloc(256);
	memset(msg, '\0', 256);
	int i;
	msg[0] = '{';
	msg[1] = '\"';
	for (i=2; i < 197; i++) {
		msg[i] = 'a';
	}
	msg[i+1] = '\"';
	msg[i+2] = '}';

    printf("status response build\r\n");
    return msg;
}

char* buildSyncResponse()
{
    char* msg;
    cJSON *root, *header, *body;
    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "Header",   header=cJSON_CreateObject());
    cJSON_AddItemToObject(root, "Body",     body=cJSON_CreateObject());

    msg=cJSON_Print(root);
    cJSON_Delete(root);

    printf("status response build\r\n");
    return msg;
}

void buildErrorMessage(char* msg, char* error_code, char* error_msg)
{
    cJSON *root, *header, *body;
    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "header",   header=cJSON_CreateObject());
    cJSON_AddItemToObject(root, "body",     body=cJSON_CreateObject());

    cJSON_AddStringToObject(header, "MessageType",      "Fault");

    cJSON_AddStringToObject(body,   "FaultCode",  error_code);
    cJSON_AddStringToObject(body,   "FaultMessage",  error_msg);
    msg=cJSON_Print(root);
    cJSON_Delete(root);
}
