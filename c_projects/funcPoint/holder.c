#include "holder.h" 
#include "common.h"
#include "stdio.h"

int holder_func(struct SHandlers* handlers)
{
	struct SReportStatus* status_report = handlers->statusHandler();
	struct SRegister* registers = handlers->registerHandler();
	
	printf("Firmware version: %s\n", status_report->firm_version);
	printf("Address: %s, value: %s\n", registers[0].address, registers[0].value);
	return 0;
}
