/*########################################################################*/
/*#                                                                      #*/
/*#                                                                      #*/
/*#               Copyright (c) 2005 Iskratel                            #*/
/*#                                                                      #*/
/*#                                                                      #*/
/*# Name:         log.h                                                  #*/
/*#                                                                      #*/
/*# Description:  Definition of functions for loging messages            #*/
/*#                                                                      #*/
/*# Code:         DJTF - XAC7124                                         #*/
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
/* static char unixid[] = "@(#)DJTF.z	1.5	12/01/24	log.h -O-"; */

#ifndef _LOG_H_INCLUDED 
#define _LOG_H_INCLUDED

#define LOG_TYPE_ERROR	0
#define LOG_TYPE_INFO	1
#define DEBUG_TYPE_INFO	2

#define DATE_TIME_WIDTH	20
#define LOG_MSG_WIDTH	1024

static const char *const logTypeStr[] = {
		"Error",
		"Info",
		"Debug"};

extern char *SCENARIO;
extern char *DBRELEASE;
extern char *DATARELEASE;
extern char *MNCOMMONDBRELEASE;
extern char *LOG_FILE_NAME;
extern char *FILEPATH;

/****************************************************************/
/* FUNCTION PROTOTYPES                                          */
/****************************************************************/

extern void log_message( char *message, int logType );
extern void write_logfile( char *logmessage );
extern char *strTime(void);

#endif
