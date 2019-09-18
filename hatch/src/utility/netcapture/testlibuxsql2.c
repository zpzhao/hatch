/*
 * src/test/examples/testlibuxsql2.c
 *
 *
 * testlibuxsql2.c
 *		Test of the asynchronous notification interface
 *
 * Start this program, then from uxsql in another window do
 *	 NOTIFY TBL2;
 * Repeat four times to get this program to exit.
 *
 * Or, if you want to get fancy, try this:
 * populate a database with the following commands
 * (provided in src/test/examples/testlibuxsql2.sql):
 *
 *	 CREATE TABLE TBL1 (i int4);
 *
 *	 CREATE TABLE TBL2 (i int4);
 *
 *	 CREATE RULE r1 AS ON INSERT TO TBL1 DO
 *	   (INSERT INTO TBL2 VALUES (new.i); NOTIFY TBL2);
 *
 * and do this four times:
 *
 *	 INSERT INTO TBL1 VALUES (10);
 */

#ifdef WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

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
	UXnotify   *notify;
	int			nnotifies;

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
	 * Issue LISTEN command to enable notifications from the rule's NOTIFY.
	 */
	res = UXSQLexec(conn, "LISTEN TBL2");
	if (UXSQLresultStatus(res) != UXRES_COMMAND_OK)
	{
		fprintf(stderr, "LISTEN command failed: %s", UXSQLerrorMessage(conn));
		UXSQLclear(res);
		exit_nicely(conn);
	}

	/*
	 * should UXSQLclear UXresult whenever it is no longer needed to avoid memory
	 * leaks
	 */
	UXSQLclear(res);

	/* Quit after four notifies are received. */
	nnotifies = 0;
	while (nnotifies < 4)
	{
		/*
		 * Sleep until something happens on the connection.  We use select(2)
		 * to wait for input, but you could also use poll() or similar
		 * facilities.
		 */
		int			sock;
		fd_set		input_mask;

		sock = UXSQLsocket(conn);

		if (sock < 0)
			break;				/* shouldn't happen */

		FD_ZERO(&input_mask);
		FD_SET(sock, &input_mask);

		if (select(sock + 1, &input_mask, NULL, NULL, NULL) < 0)
		{
			fprintf(stderr, "select() failed: %s\n", strerror(errno));
			exit_nicely(conn);
		}

		/* Now check for input */
		UXSQLconsumeInput(conn);
		while ((notify = UXSQLnotifies(conn)) != NULL)
		{
			fprintf(stderr,
					"ASYNC NOTIFY of '%s' received from backend PID %d\n",
					notify->relname, notify->be_pid);
			UXSQLfreemem(notify);
			nnotifies++;
		}
	}

	fprintf(stderr, "Done.\n");

	/* close the connection to the database and cleanup */
	UXSQLfinish(conn);

	return 0;
}
