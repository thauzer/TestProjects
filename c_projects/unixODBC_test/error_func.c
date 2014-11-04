#include "error_func.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void extract_error(char *fn, SQLHANDLE handle, SQLSMALLINT type)
{
	SQLINTEGER	 i = 0;
	SQLINTEGER	 native;
	SQLCHAR	 state[ 7 ];
	SQLCHAR	 text[256];
	SQLSMALLINT	 len;
	SQLRETURN	 ret;
	
	fprintf(stderr, "\nThe driver reported the following diagnostics whilst running %s\n\n", fn);
	
	do {
		ret = SQLGetDiagRec(type, handle, ++i, state, &native, text, sizeof(text), &len );
		if (SQL_SUCCEEDED(ret))
			printf("%s:%d:%d:%s\n", state, i, native, text);
	}
	while( ret == SQL_SUCCESS );
}

char *dbErrorMsg(SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt, char *errorMsg, int errorLen)
{
//	RETCODE r;
	char	msg[LOG_MSG_WIDTH];
	SQLCHAR	SQLstate[SQL_STATE_LEN];
	SQLCHAR	errormsg[INI_LEN];
	SQLINTEGER  errorcode;
	SQLSMALLINT msglen;
	
	if (errorMsg == NULL)
		return(errorMsg);
	SQLError(henv, hdbc, hstmt, SQLstate, &errorcode, errormsg, SQL_MAX_MESSAGE_LENGTH - 1, &msglen);
	sprintf(msg, "Errorcode: %d, Errormsg: %s\n", (int)errorcode, errormsg);
	strncpy(errorMsg, msg, errorLen);
	
	return errorMsg;
}