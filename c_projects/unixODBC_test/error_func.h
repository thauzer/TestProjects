#ifndef _ERROR_FUNC_H_
#define _ERROR_FUNC_H_

#include <sql.h>
#include <sqlext.h>

#define LOG_MSG_WIDTH	1024
#define INI_LEN			1000
#define	SQL_STATE_LEN		50

void extract_error(char *fn, SQLHANDLE handle, SQLSMALLINT type);
char *dbErrorMsg(SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt, char *errorMsg, int errorLen);

#endif