#include "odbc_test_func.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sql.h>
#include <sqlext.h>

#include "error_func.h"

SQLCHAR *inString = (SQLCHAR *)("DRIVER={PSQL};servername=127.0.0.1;port=5432;database=installdb123;username=installdb;password=installdb");

SQLHENV env;
SQLHDBC dbc;

int connect_to_database()
{
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
		printf("Connected\n");
		printf("Returned connection string was:\n\t%s\n", outstr);
		
		if (ret == SQL_SUCCESS_WITH_INFO) {
			printf("Driver reported the following diagnostics\n");
			extract_error("SQLDriverConnect", dbc, SQL_HANDLE_DBC);
		}
		SQLDisconnect(dbc);		/* disconnect from driver */
	} else {
		fprintf(stderr, "Failed to connect\n");
		extract_error("SQLDriverConnect", dbc, SQL_HANDLE_DBC);
	}
	return 0;
}

int disconnect_from_database()
{
	/* free up allocated handles */
	SQLDisconnect(dbc);
	SQLFreeHandle(SQL_HANDLE_DBC, dbc);
	SQLFreeHandle(SQL_HANDLE_ENV, env);
	return 0;
}

int print_installed_drivers()
{
	SQLHENV env;
	SQLCHAR driver[256];
	SQLCHAR attr[256];
	
	SQLSMALLINT driver_ret;
	SQLSMALLINT attr_ret;
	SQLUSMALLINT direction;
	SQLRETURN ret;
	
	SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
	SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
	
	direction = SQL_FETCH_FIRST;
	
	printf("------------------\n");
	printf("Installed drivers\n");
	while(SQL_SUCCEEDED(ret = SQLDrivers(env, direction, driver, sizeof(driver), &driver_ret, attr, sizeof(attr), &attr_ret))) {
		direction = SQL_FETCH_NEXT;
		printf("%s - %s\n", driver, attr);
		if (ret == SQL_SUCCESS_WITH_INFO) printf("\tdata truncation\n");
	}
	return 0;
}

int print_installed_datasources()
{
	SQLHENV env;
	SQLCHAR dsn[256];
	SQLCHAR desc[256];
	SQLSMALLINT dsn_ret;
	SQLSMALLINT desc_ret;
	SQLUSMALLINT direction;
	SQLRETURN ret;
	
	SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
	SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
	
	direction = SQL_FETCH_FIRST;
	printf("------------------\n");
	printf("Installed datasources\n");
	while(SQL_SUCCEEDED(ret = SQLDataSources(env, direction, dsn, sizeof(dsn), &dsn_ret, desc, sizeof(desc), &desc_ret))) {
		direction = SQL_FETCH_NEXT;
		printf("%s - %s\n", dsn, desc);
		if (ret == SQL_SUCCESS_WITH_INFO) printf("\tdata truncation\n");
	}
	
	SQLFreeHandle(SQL_HANDLE_ENV, env);
	return 0;
}

int full_connection()
{	
	connect_to_database();
	
	disconnect_from_database();
	
	return 0;
}

void getCatalogs()
{
	printf("getCatalogs\n");
	SQLHSTMT stmt;
	SQLSMALLINT columns; /* number of columns in result-set */
	SQLCHAR buf[5][64];
	SQLINTEGER indicator[ 5 ];
	int i;
	
	connect_to_database();
	
	/* Allocate a statement handle */
	SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
	/* Retrieve a list of tables */
	
	SQLTables(stmt, (SQLCHAR*)SQL_ALL_CATALOGS, SQL_NTS, (SQLCHAR*)"", SQL_NTS, (SQLCHAR*)"", SQL_NTS, (SQLCHAR*)"", SQL_NTS );
	/* How many columns are there */
	
	SQLNumResultCols(stmt, &columns);
	
	/* Loop through the rows in the result-set binding to */
	/* local variables */
	for (i = 0; i < columns; i++) {
		SQLBindCol( stmt, i + 1, SQL_C_CHAR,
					buf[ i ], sizeof( buf[ i ] ), &indicator[ i ] );
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
	}
	
	SQLFreeHandle(SQL_HANDLE_STMT, stmt);
	disconnect_from_database();
}

void getSchemas()
{
	printf("getSchemas\n");
	SQLHSTMT stmt;
	SQLSMALLINT columns; /* number of columns in result-set */
	SQLCHAR buf[5][64];
	SQLINTEGER indicator[ 5 ];
	int i;
	
	connect_to_database();
	
	/* Allocate a statement handle */
	SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
	/* Retrieve a list of schemas */
	SQLTables(stmt, (SQLCHAR*)"", SQL_NTS, (SQLCHAR*)SQL_ALL_SCHEMAS, SQL_NTS, (SQLCHAR*)"", SQL_NTS, (SQLCHAR*)"", SQL_NTS );
	
	/* How many columns are there */
	SQLNumResultCols(stmt, &columns);
	
	/* Loop through the rows in the result-set binding to */
	/* local variables */
	for (i = 0; i < columns; i++) {
		SQLBindCol(stmt, i+1, SQL_C_CHAR, buf[i], sizeof(buf[i]), &indicator[i]);
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
	}
	SQLFreeHandle(SQL_HANDLE_STMT, stmt);
	disconnect_from_database();
}

void create_table()
{
	printf("create_table\n");
	SQLHSTMT stmt;
	RETCODE	r;
	char errorMsg[LOG_MSG_WIDTH];
	
	connect_to_database();
	
	/* Allocate a statement handle */
	SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
	
	r = SQLPrepare(stmt, (SQLCHAR *)"CREATE TABLE process_log (process_id bigint NOT NULL, order_id integer NOT NULL, finish_date timestamp, PRIMARY KEY (process_id));", SQL_NTS);
	if (r != SQL_SUCCESS) {
		printf("SQLPrepare: executeSqlStatement failed\n");
	}
	else
	{
		r = SQLExecute(stmt);
		if (!((r == SQL_SUCCESS) || (r == SQL_SUCCESS_WITH_INFO)))
		{
			printf("return value: %d\n", r);
			printf("Error:%s\n", dbErrorMsg(env, dbc, stmt, &errorMsg[0], LOG_MSG_WIDTH));
			printf("SQLExecute: executeSqlStatement failed\n");
		}
	}
	
	SQLFreeHandle(SQL_HANDLE_STMT, stmt);
	disconnect_from_database();
}

void get_tables_for_schema()
{
	printf("get_tables_for_schema\n");
	SQLHSTMT stmt;
	SQLSMALLINT columns; /* number of columns in result-set */
	SQLCHAR buf[5][64];
	SQLINTEGER indicator[ 5 ];
	int i;
	
	connect_to_database();
	
	/* Allocate a statement handle */
	SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
	/* Retrieve a list of tables */
	
	SQLTables(stmt, (SQLCHAR*)SQL_ALL_CATALOGS, SQL_NTS, (SQLCHAR*)"", SQL_NTS, (SQLCHAR*)"", SQL_NTS, (SQLCHAR*)"", SQL_NTS );
	/* How many columns are there */
	
	SQLNumResultCols(stmt, &columns);
	
	/* Loop through the rows in the result-set binding to */
	/* local variables */
	for (i = 0; i < columns; i++) {
		SQLBindCol( stmt, i + 1, SQL_C_CHAR, buf[i], sizeof( buf[i] ), &indicator[i] );
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
	}
	
	SQLFreeHandle(SQL_HANDLE_STMT, stmt);
	disconnect_from_database();
}

void get_tables_for_schema()
{
	printf("get_tables_for_schema\n");
	SQLHENV env;
	SQLHDBC dbc;
	SQLHSTMT stmt;
	SQLSMALLINT columns; /* number of columns in result-set */
	SQLCHAR buf[5][64];
	SQLINTEGER indicator[ 5 ];
	int i;
	
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
		printf("Connected\n");
		printf("Returned connection string was:\n\t%s\n", outstr);
		
		if (ret == SQL_SUCCESS_WITH_INFO) {
			printf("Driver reported the following diagnostics\n");
			extract_error("SQLDriverConnect", dbc, SQL_HANDLE_DBC);
		}
		//		SQLDisconnect(dbc);		/* disconnect from driver */
	} else {
		fprintf(stderr, "Failed to connect\n");
		extract_error("SQLDriverConnect", dbc, SQL_HANDLE_DBC);
	}
	
	/* Allocate a statement handle */
	SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
	/* Retrieve a list of tables */
	
	SQLTables(stmt, NULL, SQL_NTS, (SQLCHAR*)"installdb3", SQL_NTS, (SQLCHAR*)"", SQL_NTS, (SQLCHAR*)"TABLE", SQL_NTS );
	/* How many columns are there */
	
	SQLNumResultCols(stmt, &columns);
	
	/* Loop through the rows in the result-set binding to */
	/* local variables */
	for (i = 0; i < columns; i++) {
		SQLBindCol( stmt, i + 1, SQL_C_CHAR, buf[i], sizeof( buf[i] ), &indicator[i] );
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
	/* free up allocated handles */
	SQLDisconnect(dbc);
	SQLFreeHandle(SQL_HANDLE_DBC, dbc);
	SQLFreeHandle(SQL_HANDLE_ENV, env);
}

void get_foreign_keys()
{
	printf("get_foreign_keys\n");
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
	SQLHSTMT stmt;
	SQLSMALLINT columns; /* number of columns in result-set */
	SQLCHAR bufChar[10][64];
	SQLSMALLINT bufSmallInt[4];
	SQLINTEGER indicator[5];
	
	ret = SQLAllocStmt(dbc, &stmt);
	if (ret != SQL_SUCCESS) {
		extract_error("SQLAllocStmt", dbc, SQL_HANDLE_DBC); return;
	}
	
	SQLTables(stmt, NULL, SQL_NTS, (SQLCHAR*)"installdb3", SQL_NTS, (SQLCHAR*)"", SQL_NTS, (SQLCHAR*)"TABLE", SQL_NTS );
	/* How many columns are there */
	
	SQLNumResultCols(stmt, &columns);
	
	/* Loop through the rows in the result-set binding to */
	/* local variables */
	int i;
	for (i = 0; i < columns; i++) {
		SQLBindCol( stmt, i + 1, SQL_C_CHAR, bufChar[i], sizeof( bufChar[i] ), &indicator[i] );
	}
	
	/* Fetch the data */
	while (SQL_SUCCEEDED(SQLFetch(stmt))) {
		/* display the results that will now be in the bound area's */
		for ( i = 0; i < columns; i ++ ) {
			if (indicator[ i ] == SQL_NULL_DATA) { 
				//				printf("  Column %u : NULL\n", i); 
			}
			else {
				if (strlen((char*)(bufChar[i])) > 0) { 
					printf("  Column %u : %s\n", i, bufChar[i]);
					
				} 
				else { 
					//					printf("  Column %u : Empty string\n", i); 
				}
			}
		}
		printf("\n");
	}
	
	SQLFreeHandle(SQL_HANDLE_STMT, stmt);
	
	/*----------------------------------*/
	SQLCHAR		errormsg[SQL_MAX_MESSAGE_LENGTH];
	ret = SQLAllocStmt(dbc, &stmt);
	if (ret != SQL_SUCCESS) {
		extract_error("SQLAllocStmt", dbc, SQL_HANDLE_DBC); return;
	}
	
	ret = SQLForeignKeys(stmt, 
						 NULL, 0, /* Primary catalog */ 
					  NULL, 0, /* Primary schema */
					  NULL, 0, /* Primary table */
					  (SQLCHAR*)"instaldb123", SQL_NTS, /* Foreign catalog */
						 (SQLCHAR*)"instaldb3", SQL_NTS, /* Foreign schema */
						  (SQLCHAR*)"eventtypes", SQL_NTS /* Foreign table */
						  );
						  if (ret != SQL_SUCCESS) {
							  
							  printf("error: %s\n", dbErrorMsg(env, dbc, stmt, &errormsg[0], SQL_MAX_MESSAGE_LENGTH));
						 printf("SQLForeignKeys - ret: %d\n", ret);
	extract_error("SQLForeignKeys", stmt, SQL_HANDLE_STMT); 
	return;
						  }
						  
						  SQLNumResultCols(stmt, &columns);
						  
						  SQLFreeHandle(SQL_HANDLE_STMT, stmt);
						  /*-------------------------------------------------*/
						  /* free up allocated handles */
						  SQLDisconnect(dbc);
						  SQLFreeHandle(SQL_HANDLE_DBC, dbc);
						  SQLFreeHandle(SQL_HANDLE_ENV, env);
}


struct SInfo {
	int code;
	char code_str[1024];
};

static const struct SInfo driver_info[] = {
	{SQL_DATA_SOURCE_NAME, 	"SQL_DATA_SOURCE_NAME"},
	{SQL_DRIVER_HDBC, 		"SQL_DRIVER_HDBC"},
	{SQL_DRIVER_HDESC, 		"SQL_DRIVER_HDESC"},
	{SQL_DRIVER_HENV, 		"SQL_DRIVER_HENV"},
	{SQL_DRIVER_HLIB, 		"SQL_DRIVER_HLIB"},
	{SQL_DRIVER_HSTMT, 		"SQL_DRIVER_HSTMT"},
	{SQL_DATABASE_NAME, 	"SQL_DATABASE_NAME"},
	{SQL_DBMS_NAME, 		"SQL_DBMS_NAME"},
	{SQL_DBMS_VER, 			"SQL_DBMS_VER"},
	{SQL_DRIVER_NAME, 		"SQL_DRIVER_NAME"},
	{SQL_DRIVER_ODBC_VER, 	"SQL_DRIVER_ODBC_VER"},
	{SQL_DRIVER_VER, 		"SQL_DRIVER_VER"},
	{SQL_ODBC_VER, 			"SQL_ODBC_VER"},
	{SQL_SERVER_NAME, 		"SQL_SERVER_NAME"}
};

void get_info_test()
{
	printf("get_info_test\n");
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
	
	SQLCHAR string_val[1024];
	SQLSMALLINT string_len;
	
	int i;
	for (i = 0; i < sizeof(driver_info) / sizeof(driver_info[0]); i++) {
		memset(string_val, '\0', 1024);
		ret = SQLGetInfo(dbc, driver_info[i].code, string_val, sizeof(string_val), &string_len);
		print_string_with_error_checking(ret, driver_info[i].code_str, string_val);
	}
	
	printf("\nSQL_DBMS_NAME test\n");
	memset(string_val, '\0', 1024);
	ret = SQLGetInfo(dbc, SQL_DBMS_NAME, string_val, sizeof(string_val), &string_len);
	print_string_with_error_checking(ret, "SQL_DBMS_NAME", string_val);
	
	char seqQueryStr[1024];
	memset(seqQueryStr, '\0', 1024);
	if (strcmp(POSTGRESQL_NAME_LONG, (const char*)string_val) == 0) {
		strcpy(seqQueryStr, "SELECT c.relname as sequence_name FROM pg_class c INNER JOIN pg_namespace n ON c.relnamespace = n.oid WHERE c.relkind = 'S'");
		//		seqQueryStr[0] = "SELECT c.relname as sequence_name FROM pg_class c INNER JOIN pg_namespace n ON c.relnamespace = n.oid WHERE c.relkind = 'S'";
	} else if (strcmp(POSTGRESQL_NAME_SHORT, (const char*)string_val) == 0) {
		strcpy(seqQueryStr, "SELECT c.relname as sequence_name FROM pg_class c INNER JOIN pg_namespace n ON c.relnamespace = n.oid WHERE c.relkind = 'S'");
		//		seqQueryStr[0] = "SELECT c.relname as sequence_name FROM pg_class c INNER JOIN pg_namespace n ON c.relnamespace = n.oid WHERE c.relkind = 'S'";
	} else if (strcmp(NETEZZA_NAME, (const char*)string_val) == 0) {
		strcpy(seqQueryStr, "SELECT SEQ_ID as sequence_name FROM _VT_SEQUENCE");
		//		seqQueryStr[0] = "SELECT SEQ_ID as sequence_name FROM _VT_SEQUENCE";
	}
	
	printf("query string: %s\n", &seqQueryStr[0]);
	
	if (strcmp(POSTGRESQL_NAME_LONG, (const char*)string_val) == 0) {
		printf("Long postgresql driver name: %s\n", string_val);
	} else if (strcmp(POSTGRESQL_NAME_SHORT, (const char*)string_val) == 0) {
		printf("Short postgresql driver name: %s\n", string_val);
	} else if (strcmp(NETEZZA_NAME, (const char*)string_val) == 0) {
		printf("Netezza driver name: %s\n", string_val);
	}
	
	/* for string values 
	 * memset(string_val, '\0', 1024);
	 * ret = SQLGetInfo(dbc, SQL_DATABASE_NAME, string_val, sizeof(string_val), &string_len);
	 * print_string_with_error_checking(ret, "SQL_DATABASE_NAME", string_val);
	 * 
	 * SQLUINTEGER u_val;
	 *	SQLUSMALLINT su_val;
	 * // for SQLUINTEGER values - mostly bitmasks , last two arguments ignored
	 * ret = SQLGetInfo(dbc, SQL_XXX, (SQLPOINTER)&u_val, 0, 0); 
	 * if (SQL_SUCCEEDED(ret)) {
	 *	printf("Returned value is %lx\n", u_val);
	 } else {
		 // error
	 }
	 
	 // for SQLUSMALLINT values - mostly counts/limits, last two arguments ignored
	 ret = SQLGetInfo(dbc, SQL_XXX, (SQLPOINTER)&su_val, 0, 0);
	 if (SQL_SUCCEEDED(ret)) {
		 printf("Returned value is %u\n", su_val);
	 } else { // error 
	 }
	 */
	/*-------------------------------------------------*/
	/* free up allocated handles */
	SQLDisconnect(dbc);
	SQLFreeHandle(SQL_HANDLE_DBC, dbc);
	SQLFreeHandle(SQL_HANDLE_ENV, env);
}

int print_installed_drivers()
{
	SQLHENV env;
	SQLCHAR driver[256];
	SQLCHAR attr[256];
	
	SQLSMALLINT driver_ret;
	SQLSMALLINT attr_ret;
	SQLUSMALLINT direction;
	SQLRETURN ret;
	
	SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
	SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
	
	direction = SQL_FETCH_FIRST;
	
	printf("------------------\n");
	printf("Installed drivers\n");
	while(SQL_SUCCEEDED(ret = SQLDrivers(env, direction, driver, sizeof(driver), &driver_ret, attr, sizeof(attr), &attr_ret))) {
		direction = SQL_FETCH_NEXT;
		printf("%s - %s\n", driver, attr);
		if (ret == SQL_SUCCESS_WITH_INFO) printf("\tdata truncation\n");
	}
	return 0;
}

int print_installed_datasources()
{
	SQLHENV env;
	SQLCHAR dsn[256];
	SQLCHAR desc[256];
	SQLSMALLINT dsn_ret;
	SQLSMALLINT desc_ret;
	SQLUSMALLINT direction;
	SQLRETURN ret;
	
	SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
	SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
	
	direction = SQL_FETCH_FIRST;
	printf("------------------\n");
	printf("Installed datasources\n");
	while(SQL_SUCCEEDED(ret = SQLDataSources(env, direction, dsn, sizeof(dsn), &dsn_ret, desc, sizeof(desc), &desc_ret))) {
		direction = SQL_FETCH_NEXT;
		printf("%s - %s\n", dsn, desc);
		if (ret == SQL_SUCCESS_WITH_INFO) printf("\tdata truncation\n");
	}
	
	SQLFreeHandle(SQL_HANDLE_ENV, env);
	return 0;
}

int isPostgreSQLDatabase(SQLHDBC hdbc)
{
	RETCODE		r;
	SQLCHAR string_val[1024];
	SQLSMALLINT string_len;
	
	memset(string_val, '\0', 1024);
	r = SQLGetInfo(hdbc, SQL_DBMS_NAME, string_val, sizeof(string_val), &string_len);
	
	if (r != SQL_SUCCESS) {
		printf("ni ratalo\n");
	}
	
	char seqQueryStr[1024];
	memset(seqQueryStr, '\0', 1024);
	int is_postgresql = 0;
	if (strcmp(POSTGRESQL_NAME_LONG, (const char*)string_val) == 0) {
		is_postgresql = 1;
	} else if (strcmp(POSTGRESQL_NAME_SHORT, (const char*)string_val) == 0) {
		is_postgresql = 1;
	} else if (strcmp(NETEZZA_NAME, (const char*)string_val) == 0) {
		is_postgresql = 0;
	}
	
	return is_postgresql;
}

void getCatalogs()
{
	printf("getCatalogs\n");
	
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
	//	ret = SQLConnect(dbc, "DSNtestPostgres", SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS);
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
	SQLHSTMT stmt;
	SQLSMALLINT columns; /* number of columns in result-set */
	SQLCHAR buf[5][64];
	SQLINTEGER indicator[ 5 ];
	int i;
	/* Allocate a statement handle */
	SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
	/* Retrieve a list of tables */
	
	SQLTables(stmt, (SQLCHAR*)SQL_ALL_CATALOGS, SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS );
	/* How many columns are there */
	
	SQLNumResultCols(stmt, &columns);
	
	/* Loop through the rows in the result-set binding to */
	/* local variables */
	for (i = 0; i < columns; i++) {
		SQLBindCol( stmt, i + 1, SQL_C_CHAR, buf[i], sizeof(buf[i]), (SQLLEN*)&indicator[i]);
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

void getDatabases()
{
	printf("getPostgreSQLDatabases\n");
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
	// check existing databases for Netezza or PostgreSQL
	SQLCHAR string_val[1024];
	SQLSMALLINT string_len;
	int isPostgreSQL;
	char seqQueryStr[1024];
	SQLHSTMT stmtQueryDbs;
	int i;
	SQLCHAR buf[1][64];
	SQLINTEGER indicator;
	SQLSMALLINT columns;
	
	printf("\nSQL_DBMS_NAME test\n");
	memset(string_val, '\0', 1024);
	ret = SQLGetInfo(dbc, SQL_DBMS_NAME, string_val, sizeof(string_val), &string_len);
	print_string_with_error_checking(ret, "SQL_DBMS_NAME", string_val);
	
	isPostgreSQL = isPostgreSQLDatabase(dbc);
	
	memset(seqQueryStr, '\0', 1024);
	if (isPostgreSQL > 0) {
		printf("PostgreSQL database\n");
		strcpy(seqQueryStr, "select datname from pg_database where datistemplate = false and datname='temp_db'");
	} else {
		printf("Netezza database\n");
		strcpy(seqQueryStr, "select database from _V_DATABASE where database='temp_db';");
	}
	printf("query string: %s\n", &seqQueryStr[0]);
	
	// query system table for database names
	SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmtQueryDbs);
	ret = SQLPrepare(stmtQueryDbs, (SQLCHAR*)seqQueryStr, SQL_NTS);
	if (ret != SQL_SUCCESS) {
		printf("SQLPrepare: executeSqlStatement failed\n");
	} else {
		ret = SQLExecute(stmtQueryDbs);
		if (!((ret == SQL_SUCCESS) || (ret == SQL_SUCCESS_WITH_INFO)))
		{
			printf("return value: %d\n", ret);
			//			printf("Error:%s\n", dbErrorMsg(env, dbc, stmtQueryDbs, &errorMsg[0], LOG_MSG_WIDTH));
			printf("SQLExecute: executeSqlStatement failed\n");
		}
	}
	
	SQLNumResultCols(stmtQueryDbs, &columns);
	printf("columns: %d\n", columns);
	
	for (i=0; i < columns; i++) {
		SQLBindCol(stmtQueryDbs, i+1, SQL_C_CHAR, buf[i], sizeof(buf[i]), (SQLLEN*)&indicator);
	}
	
	while (SQL_SUCCEEDED(SQLFetch(stmtQueryDbs))) {
		for (i=0; i < columns; i++) {
			if (indicator == SQL_NULL_DATA) {
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
	
	/*-------------------------------------------------*/
	/* free up allocated handles */
	SQLDisconnect(dbc);
	SQLFreeHandle(SQL_HANDLE_DBC, dbc);
	SQLFreeHandle(SQL_HANDLE_ENV, env);
}

struct SInfo {
	int code;
	char code_str[1024];
};

static const struct SInfo driver_info[] = {
	{SQL_DATA_SOURCE_NAME, 	"SQL_DATA_SOURCE_NAME"},
	{SQL_DRIVER_HDBC, 		"SQL_DRIVER_HDBC"},
	{SQL_DRIVER_HDESC, 		"SQL_DRIVER_HDESC"},
	{SQL_DRIVER_HENV, 		"SQL_DRIVER_HENV"},
	{SQL_DRIVER_HLIB, 		"SQL_DRIVER_HLIB"},
	{SQL_DRIVER_HSTMT, 		"SQL_DRIVER_HSTMT"},
	{SQL_DATABASE_NAME, 	"SQL_DATABASE_NAME"},
	{SQL_DBMS_NAME, 		"SQL_DBMS_NAME"},
	{SQL_DBMS_VER, 			"SQL_DBMS_VER"},
	{SQL_DRIVER_NAME, 		"SQL_DRIVER_NAME"},
	{SQL_DRIVER_ODBC_VER, 	"SQL_DRIVER_ODBC_VER"},
	{SQL_DRIVER_VER, 		"SQL_DRIVER_VER"},
	{SQL_ODBC_VER, 			"SQL_ODBC_VER"},
	{SQL_SERVER_NAME, 		"SQL_SERVER_NAME"}
};

void get_info_test()
{
	printf("get_info_test\n");
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
	
	SQLCHAR string_val[1024];
	SQLSMALLINT string_len;
	
	int i;
	for (i = 0; i < sizeof(driver_info) / sizeof(driver_info[0]); i++) {
		memset(string_val, '\0', 1024);
		ret = SQLGetInfo(dbc, driver_info[i].code, string_val, sizeof(string_val), &string_len);
		print_string_with_error_checking(ret, driver_info[i].code_str, string_val);
	}
	
	printf("\nSQL_DBMS_NAME test\n");
	memset(string_val, '\0', 1024);
	ret = SQLGetInfo(dbc, SQL_DBMS_NAME, string_val, sizeof(string_val), &string_len);
	print_string_with_error_checking(ret, "SQL_DBMS_NAME", string_val);
	
	char seqQueryStr[1024];
	memset(seqQueryStr, '\0', 1024);
	if (strcmp(POSTGRESQL_NAME_LONG, (const char*)string_val) == 0) {
		strcpy(seqQueryStr, "SELECT c.relname as sequence_name FROM pg_class c INNER JOIN pg_namespace n ON c.relnamespace = n.oid WHERE c.relkind = 'S'");
		//		seqQueryStr[0] = "SELECT c.relname as sequence_name FROM pg_class c INNER JOIN pg_namespace n ON c.relnamespace = n.oid WHERE c.relkind = 'S'";
	} else if (strcmp(POSTGRESQL_NAME_SHORT, (const char*)string_val) == 0) {
		strcpy(seqQueryStr, "SELECT c.relname as sequence_name FROM pg_class c INNER JOIN pg_namespace n ON c.relnamespace = n.oid WHERE c.relkind = 'S'");
		//		seqQueryStr[0] = "SELECT c.relname as sequence_name FROM pg_class c INNER JOIN pg_namespace n ON c.relnamespace = n.oid WHERE c.relkind = 'S'";
	} else if (strcmp(NETEZZA_NAME, (const char*)string_val) == 0) {
		strcpy(seqQueryStr, "SELECT SEQ_ID as sequence_name FROM _VT_SEQUENCE");
		//		seqQueryStr[0] = "SELECT SEQ_ID as sequence_name FROM _VT_SEQUENCE";
	}
	
	printf("query string: %s\n", &seqQueryStr[0]);
	
	if (strcmp(POSTGRESQL_NAME_LONG, (const char*)string_val) == 0) {
		printf("Long postgresql driver name: %s\n", string_val);
	} else if (strcmp(POSTGRESQL_NAME_SHORT, (const char*)string_val) == 0) {
		printf("Short postgresql driver name: %s\n", string_val);
	} else if (strcmp(NETEZZA_NAME, (const char*)string_val) == 0) {
		printf("Netezza driver name: %s\n", string_val);
	}
	
	/* for string values 
	 * memset(string_val, '\0', 1024);
	 * ret = SQLGetInfo(dbc, SQL_DATABASE_NAME, string_val, sizeof(string_val), &string_len);
	 * print_string_with_error_checking(ret, "SQL_DATABASE_NAME", string_val);
	 * 
	 * SQLUINTEGER u_val;
	 *	SQLUSMALLINT su_val;
	 * // for SQLUINTEGER values - mostly bitmasks , last two arguments ignored
	 * ret = SQLGetInfo(dbc, SQL_XXX, (SQLPOINTER)&u_val, 0, 0); 
	 * if (SQL_SUCCEEDED(ret)) {
	 *	printf("Returned value is %lx\n", u_val);
} else {
	// error
}

// for SQLUSMALLINT values - mostly counts/limits, last two arguments ignored
ret = SQLGetInfo(dbc, SQL_XXX, (SQLPOINTER)&su_val, 0, 0);
if (SQL_SUCCEEDED(ret)) {
	printf("Returned value is %u\n", su_val);
} else { // error 
}
*/
	/*-------------------------------------------------*/
	/* free up allocated handles */
	SQLDisconnect(dbc);
	SQLFreeHandle(SQL_HANDLE_DBC, dbc);
	SQLFreeHandle(SQL_HANDLE_ENV, env);
}

#define TYPE_NAME_COL_WIDTH		254

void test_log_message()
{
	printf("test_log_message\n");
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
	
	SQLHSTMT stmt;
	RETCODE	r;
	char errorMsg[LOG_MSG_WIDTH];
	
	/* Allocate a statement handle */
	SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
	
	r = SQLPrepare(stmt, (SQLCHAR *)"CREATE TABLE process_log (process_id bigint NOT NULL, order_id integer NOT NULL, finish_date timestamp, PRIMARY KEY (process_id));", SQL_NTS);
	if (r != SQL_SUCCESS) {
		printf("SQLPrepare: executeSqlStatement failed\n");
	}
	else
	{
		r = SQLExecute(stmt);
		if (!((r == SQL_SUCCESS) || (r == SQL_SUCCESS_WITH_INFO)))
		{
			printf("return value: %d\n", r);
			log_message( dbErrorMsg(env, dbc, stmt, &errorMsg[0], SQL_MAX_MESSAGE_LENGTH), LOG_TYPE_ERROR );
			printf("Error:%s\n", dbErrorMsg(env, dbc, stmt, &errorMsg[0], LOG_MSG_WIDTH));
			printf("SQLExecute: executeSqlStatement failed\n");
		}
	}
	
	SQLFreeHandle(SQL_HANDLE_STMT, stmt);
	
	
	/*-------------------------------------------------*/
	/* free up allocated handles */
	SQLDisconnect(dbc);
	SQLFreeHandle(SQL_HANDLE_DBC, dbc);
	SQLFreeHandle(SQL_HANDLE_ENV, env);
}

#if 0
void get_foreign_keys()
{
	printf("get_foreign_keys\n");
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
			SQLHSTMT stmt;
			SQLSMALLINT columns; /* number of columns in result-set */
			SQLCHAR bufChar[10][64];
			SQLSMALLINT bufSmallInt[4];
			SQLINTEGER indicator[5];
			
			/*----------------------------------*/
			SQLCHAR		errormsg[SQL_MAX_MESSAGE_LENGTH];
			ret = SQLAllocStmt(dbc, &stmt);
			if (ret != SQL_SUCCESS) {
				extract_error("SQLAllocStmt", dbc, SQL_HANDLE_DBC); return;
			}
			
			ret = SQLForeignKeys(stmt, 
			NULL, 0, /* Primary catalog */ 
			NULL, 0, /* Primary schema */
			NULL, 0, /* Primary table */
			NULL, 0, /* Foreign catalog */
			NULL, 0, /* Foreign schema */
			(SQLCHAR*)"ne_node_version", SQL_NTS /* Foreign table */
			);
			printf("SQLForeignKeys - ret: %d\n", ret);
			if (ret != SQL_SUCCESS) {
				
				printf("error: %s\n", dbErrorMsg(env, dbc, stmt, &errormsg[0], SQL_MAX_MESSAGE_LENGTH));
				extract_error("SQLForeignKeys", stmt, SQL_HANDLE_STMT); 
				return;
			}
			
			SQLNumResultCols(stmt, &columns);
			
			SQLFreeHandle(SQL_HANDLE_STMT, stmt);
			/*-------------------------------------------------*/
			/* free up allocated handles */
			SQLDisconnect(dbc);
			SQLFreeHandle(SQL_HANDLE_DBC, dbc);
			SQLFreeHandle(SQL_HANDLE_ENV, env);
			}
			#endif