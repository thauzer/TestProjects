/**
 * @file log.h
 * @author Matej Zupanc
 * @date 23.3.2011
 * @brief Defines and declaration of functions for logging
 */

#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>
#include <stdarg.h>

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_DEBUG 3
#define LOG_LEVEL_RSTPLIB 4
#define LOG_LEVEL_MAX 100

#define LOG_LEVEL_DEFAULT LOG_LEVEL_INFO

extern void logDprint(int level, const char *fmt, ...);
extern void logDprintL(int level, const char *fmt, va_list ap);
extern int log_level;

#define PRINT(_level, _fmt, _args...) logDprint(_level, _fmt, ##_args)

#define CLPM(x,y, _fmt, _args...)	\
	do if (!(x)) {	\
			PRINT(LOG_LEVEL_ERROR,	\
				"Error in %s at %s:%d verifying %s. " _fmt,	\
				__PRETTY_FUNCTION__, __FILE__, __LINE__,	\
				#x, ##_args);	\
			return y;	\
       } while (0)

#define CLP(x,y) CLPM(x,y,"")

#define LOG(_fmt, _args...) \
	PRINT(LOG_LEVEL_DEBUG, "%s: " _fmt, __PRETTY_FUNCTION__, ##_args)

#define INFO(_fmt, _args...) \
	PRINT(LOG_LEVEL_INFO, "%s: " _fmt, __PRETTY_FUNCTION__, ##_args)

#define ERROR(_fmt, _args...) \
	PRINT(LOG_LEVEL_ERROR, "error, %s: " _fmt, __PRETTY_FUNCTION__, ##_args)

#endif /* LOG_H_ */
