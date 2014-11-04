#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <termios.h>

#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>

#include "log.h"

int main(int argc, char** argv)
{
    char *name = argv[0];
    simple_dbg.console_on = 1;
    simple_dbg.syslog_on = 1;
    simple_dbg.log_level = 4;
    
    int opt;   
    while(1) {
		opt = getopt(argc, argv, "v:");
		if (opt == -1) break;
		switch(opt) {
		case 'v': // set debug level
			simple_dbg.log_level = strtol(optarg, NULL, 10);
			break;
		case '?':
			printf("Unknown setting\n");
			break;
		default:
			printf("default response\n");
		}
	}
	
	printf("Program name: %s\nDebug options:\n\tConsole print: %s\n\tSyslog print: %s\n\tDebug level: %d\n",
	        name, simple_dbg.console_on != 0 ? "yes" : "no", simple_dbg.syslog_on != 0 ? "yes" : "no", simple_dbg.log_level);
	
	openlog(name, 0, LOG_USER);
    
    printf("Test CONSOLE | SYSLOG\n");
	LOG(LOG_LEVEL_ERROR, CONSOLE | SYSLOG, "log");
	ERROR(CONSOLE | SYSLOG, "error");
	INFO(CONSOLE | SYSLOG, "info");
	DEBUG(CONSOLE | SYSLOG, "debug");
	
	printf("Test CONSOLE\n");
	LOG(LOG_LEVEL_ERROR, CONSOLE, "log console");
	ERROR(CONSOLE, "error console");
	INFO(CONSOLE, "info console");
	DEBUG(CONSOLE, "debug console");
	
	printf("Test SYSLOG\n");
	LOG(LOG_LEVEL_ERROR, SYSLOG, "log syslog");
	ERROR(SYSLOG, "error syslog");
	INFO(SYSLOG, "info syslog");
	DEBUG(SYSLOG, "debug syslog");
	
	return 0;
}
