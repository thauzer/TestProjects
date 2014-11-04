#ifndef _ODBC_TEST_FUNC_H_
#define _ODBC_TEST_FUNC_H_

#include <sql.h>

#define TABLE_NAME_COL_WIDTH		254

extern SQLCHAR *inString;
extern SQLHENV env;
extern SQLHDBC dbc;

int connect_to_database();
int disconnect_from_database();

int print_installed_drivers();
int print_installed_datasources();
int full_connection();
void getCatalogs();
void getSchemas();
void create_table();
int get_tables_for_schema();
void get_tables_for_schema();
void get_foreign_keys();

#endif