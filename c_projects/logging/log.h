/**
 * @file log.h
 * @author Matej Zupanc
 * @date 23.3.2011
 * @brief Defines and declaration of functions for logging
 */

#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_DEBUG 3
#define LOG_LEVEL_MAX 100

#define LOG_LEVEL_DEFAULT LOG_LEVEL_ERROR

/* logging flags */
#define SYSLOG                 0x00000001	/* syslog */
#define CONSOLE                0x00000002	/* console */

typedef struct {
    int console_on;
    int syslog_on;
    int log_level;
} debug_t;

debug_t simple_dbg;

void log_print(int log_level, int flags, const char *fmt, ...)
{
    va_list arg_ptr;
	char output[250];

	if (fmt == NULL)
		return;
	
	if ((simple_dbg.log_level >= log_level) || (log_level <= 2))
	{
		va_start(arg_ptr, fmt);
		vsprintf(output, fmt, arg_ptr);
		va_end(arg_ptr);

		if (simple_dbg.log_level >= log_level) {
			if (((SYSLOG & flags) != 0 ) && (simple_dbg.syslog_on))
				syslog( LOG_ERR, "%s", output);
	
			if (((CONSOLE & flags) != 0 ) && (simple_dbg.console_on))
				printf(" %s\n", output);
		}
		
		fflush(stdout);
	}
}

#define INIT_LOG(_name) openlog(_name, 0, LOG_USER)

#define PRINT(_level, _flags, _fmt, _args...) log_print(_level, _flags, _fmt, ##_args)

#define CLPM(x,y, _fmt, _args...)	\
	do if (!(x)) {	\
			PRINT(LOG_LEVEL_ERROR,	\
				"Error in %s at %s:%d verifying %s. " _fmt,	\
				__PRETTY_FUNCTION__, __FILE__, __LINE__,	\
				#x, ##_args);	\
			return y;	\
       } while (0)

#define CLP(x,y) CLPM(x,y,"")

#define DEBUG(_flags, _fmt, _args...) \
	PRINT(LOG_LEVEL_DEBUG, _flags, "%s: " _fmt, __PRETTY_FUNCTION__, ##_args)

#define INFO(_flags, _fmt, _args...) \
	PRINT(LOG_LEVEL_INFO, _flags, "%s: " _fmt, __PRETTY_FUNCTION__, ##_args)

#define ERROR(_flags, _fmt, _args...) \
	PRINT(LOG_LEVEL_ERROR, _flags, "error, %s: " _fmt, __PRETTY_FUNCTION__, ##_args)
	
#define LOG(_log_level, _flags, _fmt, _args...) \
    PRINT(_log_level, _flags, "%s: " _fmt, __PRETTY_FUNCTION__, ##_args)

#endif /* __LOG_H__ */
