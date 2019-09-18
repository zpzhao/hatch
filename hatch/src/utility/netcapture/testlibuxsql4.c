/*
 * src/test/examples/testlibuxsql4.c
 *
 *
 * testlibuxsql4.c
 *		this test program shows to use LIBUXSQL to make multiple backend
 * connections
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include "libuxsql-fe.h"

static void
exit_nicely(UXconn *conn1, UXconn *conn2)
{
	if (conn1)
		UXSQLfinish(conn1);
	if (conn2)
		UXSQLfinish(conn2);
	exit(1);
}

static void
check_conn(UXconn *conn, const char *dbName)
{
	/* check to see that the backend connection was successfully made */
	if (UXSQLstatus(conn) != CONNECTION_OK)
	{
		fprintf(stderr, "Connection to database \"%s\" failed: %s",
				dbName, UXSQLerrorMessage(conn));
		exit(1);
	}
}

int
main(int argc, char **argv)
{
	char	   *uxhost,
			   *uxport,
			   *uxoptions,
			   *uxtty;
	char	   *dbName1,
			   *dbName2;
	char	   *tblName;
	int			nFields;
	int			i,
				j;

	UXconn	   *conn1,
			   *conn2;

	/*
	 * UXresult   *res1, *res2;
	 */
	UXresult   *res1;

	if (argc != 4)
	{
		fprintf(stderr, "usage: %s tableName dbName1 dbName2\n", argv[0]);
		fprintf(stderr, "      compares two tables in two databases\n");
		exit(1);
	}
	tblName = argv[1];
	dbName1 = argv[2];
	dbName2 = argv[3];


	/*
	 * begin, by setting the parameters for a backend connection if the
	 * parameters are null, then the system will try to use reasonable
	 * defaults by looking up environment variables or, failing that, using
	 * hardwired constants
	 */
	uxhost = NULL;				/* host name of the backend */
	uxport = NULL;				/* port of the backend */
	uxoptions = NULL;			/* special options to start up the backend
								 * server */
	uxtty = NULL;				/* debugging tty for the backend */

	/* make a connection to the database */
	conn1 = UXSQLsetdb(uxhost, uxport, uxoptions, uxtty, dbName1);
	check_conn(conn1, dbName1);

	conn2 = UXSQLsetdb(uxhost, uxport, uxoptions, uxtty, dbName2);
	check_conn(conn2, dbName2);

	/* start a transaction block */
	res1 = UXSQLexec(conn1, "BEGIN");
	if (UXSQLresultStatus(res1) != UXRES_COMMAND_OK)
	{
		fprintf(stderr, "BEGIN command failed\n");
		UXSQLclear(res1);
		exit_nicely(conn1, conn2);
	}

	/*
	 * make sure to UXSQLclear() a UXresult whenever it is no longer needed to
	 * avoid memory leaks
	 */
	UXSQLclear(res1);

	/*
	 * fetch instances from the ux_database, the system catalog of databases
	 */
	res1 = UXSQLexec(conn1, "DECLARE myportal CURSOR FOR select * from ux_database");
	if (UXSQLresultStatus(res1) != UXRES_COMMAND_OK)
	{
		fprintf(stderr, "DECLARE CURSOR command failed\n");
		UXSQLclear(res1);
		exit_nicely(conn1, conn2);
	}
	UXSQLclear(res1);

	res1 = UXSQLexec(conn1, "FETCH ALL in myportal");
	if (UXSQLresultStatus(res1) != UXRES_TUPLES_OK)
	{
		fprintf(stderr, "FETCH ALL command didn't return tuples properly\n");
		UXSQLclear(res1);
		exit_nicely(conn1, conn2);
	}

	/* first, print out the attribute names */
	nFields = UXSQLnfields(res1);
	for (i = 0; i < nFields; i++)
		printf("%-15s", UXSQLfname(res1, i));
	printf("\n\n");

	/* next, print out the instances */
	for (i = 0; i < UXSQLntuples(res1); i++)
	{
		for (j = 0; j < nFields; j++)
			printf("%-15s", UXSQLgetvalue(res1, i, j));
		printf("\n");
	}

	UXSQLclear(res1);

	/* close the portal */
	res1 = UXSQLexec(conn1, "CLOSE myportal");
	UXSQLclear(res1);

	/* end the transaction */
	res1 = UXSQLexec(conn1, "END");
	UXSQLclear(res1);

	/* close the connections to the database and cleanup */
	UXSQLfinish(conn1);
	UXSQLfinish(conn2);

/*	 fclose(debug); */
	return 0;
}
