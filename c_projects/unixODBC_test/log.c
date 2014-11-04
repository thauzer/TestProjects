/*########################################################################*/
/*#                                                                      #*/
/*#                                                                      #*/
/*#               Copyright (c) 2005 Iskratel                            #*/
/*#                                                                      #*/
/*#                                                                      #*/
/*# Name:         log.c                                                  #*/
/*#                                                                      #*/
/*# Description:  Implementation of functions for loging messages        #*/
/*#                                                                      #*/
/*# Code:         DJTF - XAC7125                                         #*/
/*#                                                                      #*/
/*# Date:         August 2005                                            #*/
/*#                                                                      #*/
/*# Author:       Matej Zupanc, I-ROSE                                   #*/
/*#                                                                      #*/
/*# Remarks:                                                             #*/
/*#                                                                      #*/
/*# Revisions:                                                           #*/
/*#                                                                      #*/
/*########################################################################*/

/*************************************************************************/
/*                   UNIX - SCCS   VERSION DESCRIPTION                   */
/*************************************************************************/
/* static char unixid[] = "@(#)DJTG.z	1.4	12/01/24	log.c -O-"; */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <log.h>

/****************************************************************/
/* FUNCTION PROTOTYPES                                          */
/****************************************************************/
void log_message( char *message, int logType );
void write_logfile( char *logmessage );
char *strTime(void);

/****************************************************************/
/* FUNCTIONS                                                    */
/****************************************************************/

/*#######################################################################*/
/*#                                                                     #*/
/*# Author      : Matej Zupanc                                          #*/
/*#                                                                     #*/
/*# log_message - Log message from application to stdout and log file   #*/
/*#                                                                     #*/
/*# INPUT       : message - string value                                #*/
/*#                                                                     #*/
/*#               logType - numeric value of log type                   #*/
/*#                                                                     #*/
/*# OUTPUT      :                                                       #*/
/*#                                                                     #*/
/*# REMARK      :                                                       #*/
/*#                                                                     #*/
/*# SEE ALSO    :                                                       #*/
/*#                                                                     #*/
/*#######################################################################*/

void log_message( char *message, int logType )
{
	char logmsg[LOG_MSG_WIDTH];
	char tempMsg[LOG_MSG_WIDTH];
	char headMsg[LOG_MSG_WIDTH];
	int headLen;
	
	sprintf( headMsg, "%s [%s] [%s] [%s] [%s] [%s]", strTime(), SCENARIO, 
			DBRELEASE, DATARELEASE, MNCOMMONDBRELEASE, logTypeStr[logType] );
	headLen = strlen(headMsg);
	strncpy(tempMsg, message, LOG_MSG_WIDTH - headLen);
	if ((signed)strlen(message) > LOG_MSG_WIDTH - headLen)
		strcat(tempMsg, "...");
	printf ("[%s] %s\n", logTypeStr[logType], tempMsg);		/* write message into console */

	sprintf( logmsg, "%s %s\n", headMsg, tempMsg );		/* format message into log file format */
	write_logfile(logmsg);	
}

/*#######################################################################*/
/*#                                                                     #*/
/*# Author      : Matej Zupanc                                          #*/
/*#                                                                     #*/
/*# write_logfile - Write to log file                                   #*/
/*#                                                                     #*/
/*# INPUT       : logmessage - message to log into file                 #*/
/*#                                                                     #*/
/*# OUTPUT      :                                                       #*/
/*#                                                                     #*/
/*# REMARK      :                                                       #*/
/*#                                                                     #*/
/*# SEE ALSO    :                                                       #*/
/*#                                                                     #*/
/*#######################################################################*/

void write_logfile( char *logmessage )
{
	FILE	*logfile;
	FILE    *logfile_bckp;
	char    sec_log[LOG_MSG_WIDTH]="";

	if (LOG_FILE_NAME != NULL)
	{
		/* Open stream for init */
		if( (logfile = fopen( LOG_FILE_NAME, "a+" )) == NULL )
		{
			fprintf( stderr, "Can't open log file %s for write.\n", LOG_FILE_NAME );
		}
		else
		{
			fputs( logmessage, logfile );

			/* Close stream */
			if( fclose( logfile ) )
			{
				fprintf( stderr, "Can't close log file %s.\n", LOG_FILE_NAME );
			}
		}
        	sprintf(sec_log, "%s/installdb_sec.log", FILEPATH);
        	if( (logfile_bckp = fopen( sec_log, "a+" )) == NULL )
                {
                        fprintf( stderr, "Can't open log file %s for write.\n", sec_log );
                }
                else
                {
                        fputs( logmessage, logfile_bckp );

                        /* Close stream */
                        if( fclose( logfile_bckp ) )
                        {
                                fprintf( stderr, "Can't close log file %s. \n" ,sec_log );
                        }
                }
	}
}

/*#######################################################################*/
/*#                                                                     #*/
/*# Author      : Matej Zupanc                                          #*/
/*#                                                                     #*/
/*# log_message - format date time value into string                    #*/
/*#                                                                     #*/
/*# INPUT       :                                                       #*/
/*#                                                                     #*/
/*# OUTPUT      : string with formated date time value                  #*/
/*#                                                                     #*/
/*# REMARK      : String format YYYY-MM-DD hh:mm:ss                     #*/
/*#                                                                     #*/
/*# SEE ALSO    :                                                       #*/
/*#                                                                     #*/
/*#######################################################################*/

char *strTime()
{
	static char result[DATE_TIME_WIDTH];
	time_t aclock;
	struct tm *newtime;

	result[0] = 0;
	time( &aclock );			/* get time in seconds */
	newtime = localtime( &aclock );		/* convert time to struct tm form */

	sprintf(result, "%d-%02d-%02d %02d:%02d:%02d", newtime->tm_year+1900, newtime->tm_mon+1, newtime->tm_mday, newtime->tm_hour, newtime->tm_min, newtime->tm_sec);
	return result;
}
