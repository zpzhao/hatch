/*
 * src/test/examples/testlibuxsql.c
 *
 *
 * testlibuxsql.c
 *
 *		Test the C version of libuxsql, the UXsinoDB frontend library.
 */
#include <stdio.h>
#include <stdlib.h>
#include "libuxsql-fe.h"

static void
exit_nicely(UXconn *conn)
{
	UXSQLfinish(conn);
	exit(1);
}

int
main(int argc, char **argv)
{
	const char *conninfo;
	UXconn	   *conn;
	UXresult   *res;
	int			nFields;
	int			i,
				j;

	/*
	 * If the user supplies a parameter on the command line, use it as the
	 * conninfo string; otherwise default to setting dbname=uxdb and using
	 * environment variables or defaults for all other connection parameters.
	 */
	if (argc > 1)
		conninfo = argv[1];
	else
		conninfo = "dbname = uxdb";

	/* Make a connection to the database */
	conn = UXSQLconnectdb(conninfo);

	/* Check to see that the backend connection was successfully made */
	if (UXSQLstatus(conn) != CONNECTION_OK)
	{
		fprintf(stderr, "Connection to database failed: %s",
				UXSQLerrorMessage(conn));
		exit_nicely(conn);
	}

	/*
	 * Our test case here involves using a cursor, for which we must be inside
	 * a transaction block.  We could do the whole thing with a single
	 * UXSQLexec() of "select * from ux_database", but that's too trivial to make
	 * a good example.
	 */

	/* Start a transaction block */
	res = UXSQLexec(conn, "BEGIN");
	if (UXSQLresultStatus(res) != UXRES_COMMAND_OK)
	{
		fprintf(stderr, "BEGIN command failed: %s", UXSQLerrorMessage(conn));
		UXSQLclear(res);
		exit_nicely(conn);
	}

	/*
	 * Should UXSQLclear UXresult whenever it is no longer needed to avoid memory
	 * leaks
	 */
	UXSQLclear(res);

	/*
	 * Fetch rows from ux_database, the system catalog of databases
	 */
	res = UXSQLexec(conn, "DECLARE myportal CURSOR FOR select * from ux_database");
	if (UXSQLresultStatus(res) != UXRES_COMMAND_OK)
	{
		fprintf(stderr, "DECLARE CURSOR failed: %s", UXSQLerrorMessage(conn));
		UXSQLclear(res);
		exit_nicely(conn);
	}
	UXSQLclear(res);

	res = UXSQLexec(conn, "FETCH ALL in myportal");
	if (UXSQLresultStatus(res) != UXRES_TUPLES_OK)
	{
		fprintf(stderr, "FETCH ALL failed: %s", UXSQLerrorMessage(conn));
		UXSQLclear(res);
		exit_nicely(conn);
	}

	/* first, print out the attribute names */
	nFields = UXSQLnfields(res);
	for (i = 0; i < nFields; i++)
		printf("%-15s", UXSQLfname(res, i));
	printf("\n\n");

	/* next, print out the rows */
	for (i = 0; i < UXSQLntuples(res); i++)
	{
		for (j = 0; j < nFields; j++)
			printf("%-15s", UXSQLgetvalue(res, i, j));
		printf("\n");
	}

	UXSQLclear(res);

	/* close the portal ... we don't bother to check for errors ... */
	res = UXSQLexec(conn, "CLOSE myportal");
	UXSQLclear(res);

	/* end the transaction */
	res = UXSQLexec(conn, "END");
	UXSQLclear(res);

	/* close the connection to the database and cleanup */
	UXSQLfinish(conn);

	return 0;
}
