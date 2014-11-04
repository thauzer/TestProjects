#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sql.h>
#include <sqlext.h>

#include "log.h"

//SQLCHAR *inString = (SQLCHAR *)("DRIVER={PSQL};servername=127.0.0.1;port=5432;database=temp_db;username=postgres;password=hautom1q");
SQLCHAR *inString = (SQLCHAR *)("DSN=testPostgres");

#define TABLE_NAME_COL_WIDTH		254
#define TAB_LEN SQL_MAX_TABLE_NAME_LEN + 1
#define COL_LEN SQL_MAX_COLUMN_NAME_LEN + 1

#define TYPE_NAME_COL_WIDTH		254
#define LITERAL_PREFIX_COL_WIDTH	254
#define LITERAL_SUFFIX_COL_WIDTH	254
#define CREATE_PARAMS_COL_WIDTH		254

#define LOG_MSG_WIDTH	1024
#define INI_LEN			1000
#define	SQL_STATE_LEN		50

#define NETEZZA_NAME "NetezzaSQL"
#define POSTGRESQL_NAME_LONG "PostgreSQL"
#define POSTGRESQL_NAME_SHORT "PSQL"

char *FILEPATH;
char *CFGPATH;
char *SCENARIO;
char *DBRELEASE;
char *DATARELEASE;
char *MNCOMMONDBRELEASE;
char *LOG_FILE_NAME;
char *InstallDB_NODEID_FLAG;
char *InstallDB_NODEID;
char *InstallDB_CONNECTION_STR_FLAG;
char *InstallDB_CONNECTION_STR;
char *InstallDB_USERNAME_FLAG;
char *InstallDB_USERNAME;
char *InstallDB_PASSWORD_FLAG;
char *InstallDB_PASSWORD;
char *InstallDB_VERS_FLAG;
char *InstallDB_VERS;
char SVNMAJORVER[3];
char SVNMINORVER[3];
char SVNMINOR1VER[3];
char SVNFULLVER[11];
char SVNDBMAJORVER[3];
char SVNDBMINORVER[3];
char SVNDBMINOR1VER[3];


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

void printUsage(void)
{
	printf( "odbcTest\n" );
	printf( "Usage: odbcTest 'DSN-LESS connection string'\n");
	printf( "eg. 'DRIVER={PostgreSQL Unicode};SERVER=localhost;Database=mn;UID=smaster;PWD=secret;'\n");
}

void print_string_with_error_checking(SQLRETURN ret, const char* name, SQLCHAR* string)
{
	if (SQL_SUCCEEDED(ret)) {
		if (ret == SQL_SUCCESS_WITH_INFO)
			printf("buffer too small, string truncated\n");
		printf("%s: %s\n", name, string);
	} else { 
		printf("SQLGetInfo error: %d\n", ret);
	}
}

#define MAX_DATA_WIDTH 300
void get_type_info()
{
	printf("\nget_type_info\n");
	SQLHENV env;
	SQLHDBC dbc;
	
	SQLRETURN ret; /* ODBC API return status */
	SQLCHAR outstr[1024];
	SQLSMALLINT outstrlen;
	
	/* Allocate an environment handle */
	SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
	/* We want ODBC 3 support */
	SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
	/* Allocate a connection handle */
	SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
	
	ret = SQLDriverConnect(dbc, NULL, inString, SQL_NTS, outstr, sizeof(outstr), &outstrlen, SQL_DRIVER_COMPLETE);
	if (SQL_SUCCEEDED(ret)) { printf("connected. Returned connection string was:\n\t%s\n", outstr);
		if (ret == SQL_SUCCESS_WITH_INFO) {
			printf("Driver reported the following diagnostics\n");
			extract_error("SQLDriverConnect", dbc, SQL_HANDLE_DBC);
		}
	} else {
		printf("Failed to connect\n");
		extract_error("SQLDriverConnect", dbc, SQL_HANDLE_DBC);
		SQLFreeHandle(SQL_HANDLE_DBC, dbc);
		SQLFreeHandle(SQL_HANDLE_ENV, env);
	}
	/*-------------------------------------------------*/
	SQLHSTMT hstmt;
	SQLINTEGER      nCol                            = 0;
	SQLSMALLINT     nColumns                        = 0;
	SQLLEN          nIndicator                      = 0;
	SQLCHAR         szColumn[MAX_DATA_WIDTH+20];
	SQLCHAR         szColumnValue[MAX_DATA_WIDTH+1];
	SQLCHAR         szColumnName[MAX_DATA_WIDTH+1];
	SQLRETURN       nReturn                         = 0;
	SQLLEN          nRows                           = 0;
	
	*szColumn = '\0';
	*szColumnValue = '\0';
	*szColumnName = '\0';
	
	nReturn = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &hstmt);
	nReturn = SQLGetTypeInfo(hstmt, SQL_ALL_TYPES);
	
	nReturn = SQLNumResultCols( hstmt, &nColumns );
	if ( nReturn != SQL_SUCCESS && nReturn != SQL_SUCCESS_WITH_INFO )
		nColumns = -1;
	
	/* ROWS */
	nReturn = SQLFetch( hstmt );
	while ( nReturn == SQL_SUCCESS || nReturn == SQL_SUCCESS_WITH_INFO )
	{
		/* COLS */
		for ( nCol = 1; nCol <= nColumns; nCol++ )
		{
			
			//			nOptimalDisplayWidth = OptimalDisplayWidth( hstmt, nCol, nUserWidth );
			SQLColAttribute( hstmt, nCol, SQL_DESC_LABEL, szColumnName, sizeof(szColumnName), NULL, NULL );
			nReturn = SQLGetData( hstmt, nCol, SQL_C_CHAR, (SQLPOINTER)szColumnValue, sizeof(szColumnValue), &nIndicator );
			szColumnValue[MAX_DATA_WIDTH] = '\0';
			
			if ( nReturn == SQL_SUCCESS && nIndicator != SQL_NULL_DATA ) {
				printf("%d. szColumnName: %s - szColumnValue: %s\n", nCol, szColumnName, szColumnValue);
			}
			else if ( nReturn == SQL_ERROR ) {
				break;
			} else {
				printf("szColumn: %s - aaaaa\n", szColumn);
			}
		} /* for columns */
		
		nRows++;
		printf( "|\n" );
		nReturn = SQLFetch( hstmt );
	} /* while rows */
	/*-------------------------------------------------*/
	/* free up allocated handles */
	SQLDisconnect(dbc);
	SQLFreeHandle(SQL_HANDLE_DBC, dbc);
	SQLFreeHandle(SQL_HANDLE_ENV, env);
}

#define STR_LEN 128 + 1
#define REM_LEN 254 + 1

void get_columns()
{
	printf("\nget_columns\n");
	SQLHENV env;
	SQLHDBC dbc;
	
	SQLRETURN ret; /* ODBC API return status */
	SQLCHAR outstr[1024];
	SQLSMALLINT outstrlen;
	
	/* Allocate an environment handle */
	SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
	/* We want ODBC 3 support */
	SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
	/* Allocate a connection handle */
	SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
	
	ret = SQLDriverConnect(dbc, NULL, inString, SQL_NTS, outstr, sizeof(outstr), &outstrlen, SQL_DRIVER_COMPLETE);
	if (SQL_SUCCEEDED(ret)) { printf("connected. Returned connection string was:\n\t%s\n", outstr);
		if (ret == SQL_SUCCESS_WITH_INFO) {
			printf("Driver reported the following diagnostics\n");
			extract_error("SQLDriverConnect", dbc, SQL_HANDLE_DBC);
		}
	} else {
		printf("Failed to connect\n");
		extract_error("SQLDriverConnect", dbc, SQL_HANDLE_DBC);
		SQLFreeHandle(SQL_HANDLE_DBC, dbc);
		SQLFreeHandle(SQL_HANDLE_ENV, env);
	}
	/*-------------------------------------------------*/
	SQLHSTMT hstmt;
	SQLINTEGER      nCol                            = 0;
	SQLSMALLINT     nColumns                        = 0;
	SQLLEN          nIndicator                      = 0;
	SQLCHAR         szColumn[MAX_DATA_WIDTH+20];
	SQLCHAR         szColumnValue[MAX_DATA_WIDTH+1];
	SQLCHAR         szColumnName[MAX_DATA_WIDTH+1];
	SQLRETURN       nReturn                         = 0;
	SQLLEN          nRows                           = 0;
	
	*szColumn = '\0';
	*szColumnValue = '\0';
	*szColumnName = '\0';
	
	nReturn = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &hstmt);
	nReturn = SQLColumns(hstmt, NULL, 0, NULL, 0, (SQLCHAR*)"temp_all_types_table", SQL_NTS, NULL, 0);
	
	nReturn = SQLNumResultCols( hstmt, &nColumns );
	if ( nReturn != SQL_SUCCESS && nReturn != SQL_SUCCESS_WITH_INFO )
		nColumns = -1;
	
	/* ROWS */
	nReturn = SQLFetch( hstmt );
	while ( nReturn == SQL_SUCCESS || nReturn == SQL_SUCCESS_WITH_INFO )
	{
		/* COLS */
		for ( nCol = 1; nCol <= nColumns; nCol++ )
		{
			
//			nOptimalDisplayWidth = OptimalDisplayWidth( hstmt, nCol, nUserWidth );
			SQLColAttribute( hstmt, nCol, SQL_DESC_LABEL, szColumnName, sizeof(szColumnName), NULL, NULL );
			nReturn = SQLGetData( hstmt, nCol, SQL_C_CHAR, (SQLPOINTER)szColumnValue, sizeof(szColumnValue), &nIndicator );
			szColumnValue[MAX_DATA_WIDTH] = '\0';
			
			if ( nReturn == SQL_SUCCESS && nIndicator != SQL_NULL_DATA ) {
				printf("%d. szColumnName: %s - szColumnValue: %s\n", nCol, szColumnName, szColumnValue);
			}
			else if ( nReturn == SQL_ERROR ) {
				break;
			} else {
				printf("szColumn: %s - aaaaa\n", szColumn);
			}
		} /* for columns */
		
		nRows++;
		printf( "|\n" );
		nReturn = SQLFetch( hstmt );
	} /* while rows */
	
	/*-------------------------------------------------*/
	/* free up allocated handles */
	SQLDisconnect(dbc);
	SQLFreeHandle(SQL_HANDLE_DBC, dbc);
	SQLFreeHandle(SQL_HANDLE_ENV, env);
}


void get_tables_for_schema()
{
	printf("get_tables_for_schema\n");
	SQLHENV env;
	SQLHDBC dbc;
	
	SQLRETURN ret; /* ODBC API return status */
	SQLCHAR outstr[1024];
	SQLSMALLINT outstrlen;
	
	/* Allocate an environment handle */
	SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
	/* We want ODBC 3 support */
	SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
	/* Allocate a connection handle */
	SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
	
	ret = SQLDriverConnect(dbc, NULL, inString, SQL_NTS, outstr, sizeof(outstr), &outstrlen, SQL_DRIVER_COMPLETE);
	if (SQL_SUCCEEDED(ret)) { 
		printf("connected. \n");
//		printf("Returned connection string was:\n\t%s\n", outstr);
		if (ret == SQL_SUCCESS_WITH_INFO) {
			printf("Driver reported the following diagnostics\n");
			extract_error("SQLDriverConnect", dbc, SQL_HANDLE_DBC);
		}
	} else {
		printf("Failed to connect\n");
		extract_error("SQLDriverConnect", dbc, SQL_HANDLE_DBC);
		SQLFreeHandle(SQL_HANDLE_DBC, dbc);
		SQLFreeHandle(SQL_HANDLE_ENV, env);
	}
	/*-------------------------------------------------*/
	SQLHSTMT stmt;
	SQLSMALLINT columns; /* number of columns in result-set */
	SQLCHAR buf[5][64];
	SQLINTEGER indicator[ 5 ];
	int i;
	
	/* Allocate a statement handle */
	SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
	/* Retrieve a list of tables */
	
	SQLTables(stmt, NULL, SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS, (SQLCHAR*)"TABLE", SQL_NTS );
	/* How many columns are there */
	
	SQLNumResultCols(stmt, &columns);
	
	/* Loop through the rows in the result-set binding to */
	/* local variables */
	for (i = 0; i < columns; i++) {
		SQLBindCol( stmt, i + 1, SQL_C_CHAR, buf[i], sizeof( buf[i] ), (SQLLEN*)&indicator[i] );
	}
	
	/* Fetch the data */
	while (SQL_SUCCEEDED(SQLFetch(stmt))) {
		/* display the results that will now be in the bound area's */
		for ( i = 0; i < columns; i ++ ) {
			if (indicator[ i ] == SQL_NULL_DATA) {
				printf("  Column %u : NULL\n", i);
			}
			else {
				if (strlen((char*)(buf[i])) > 0) {
					printf("  Column %u : %s\n", i, buf[i]);
				} else {
					printf("  Column %u : Empty string\n", i);
				}
			}
		}
		printf("\n");
	}
	
	SQLFreeHandle(SQL_HANDLE_STMT, stmt);
	
	/*-------------------------------------------------*/
	/* free up allocated handles */
	SQLDisconnect(dbc);
	SQLFreeHandle(SQL_HANDLE_DBC, dbc);
	SQLFreeHandle(SQL_HANDLE_ENV, env);
}

int main(int argc, char** argv)
{
	printf("Program started\n");
	
	if (argc == 2) {
		inString = (SQLCHAR*)argv[1];
		printf("Using this connection string: %s\n", inString);
	} 
	else {
		printUsage();
		printf("Using default connection string: %s\n", inString);
//		return (EXIT_FAILURE);
	}
	
//	print_installed_datasources();
	
//	print_installed_drivers();
	
//	getCatalogs();
	
//	getDatabases();
	
//	get_info_test();
	
//	get_tables_for_schema();
	
	get_type_info();
	
	get_columns();
	
//	get_foreign_keys();
	
	printf("Program exited\n");
	return 0;
}
