#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "common.h"
#include "holder.h"

struct SReportStatus* prepareStatusMessage()
{
    struct SReportStatus* report_status = malloc(sizeof(struct SReportStatus));

	sprintf(report_status->firm_version, "3.45.009");
	sprintf(report_status->firm_version_updated, "20.12.2013 12:26:11");

	sprintf( report_status->ip_address, "123.123.123.123");
	report_status->dhcp = 1;
	report_status->wps = 0;
	report_status->rssi = -67;
	report_status->snr = 123;
	sprintf(report_status->ssid, "dd-wrt");
	report_status->security_type = 2;

    return report_status;
}

struct SRegister* prepareRegisterMessage()
{
	int max_reg_size_bytes = MAX_REGISTERS * sizeof(struct SRegister);
	struct SRegister* registers = malloc(max_reg_size_bytes);
	
	memset(registers, 0, max_reg_size_bytes);
    int nr_registers = 0;
    while (nr_registers <= 10)
    {
        if (nr_registers >= MAX_REGISTERS-1)
            return registers;
        sprintf( registers[nr_registers].address, "007a" );
        sprintf( registers[nr_registers].value, "118e");
        nr_registers++;
    }
	
	return registers;
}

int main(int argc, char** argv)
{
	printf("Program started\n");
	
	struct SHandlers handlers;
	handlers.statusHandler = &prepareStatusMessage;
	handlers.registerHandler = &prepareRegisterMessage;
	
	holder_func(&handlers);
	
	printf("Program exited\n");
	return 0;
}
