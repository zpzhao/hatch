/*
 * dir.cpp
 *
 *  Created on: 2018年9月4日
 *      Author: zpzhao
 */

#include "CDIRRequestDispatcher.h"
#include "CDir.h"
#include <iostream>
#include <string>
#include <signal.h>
#include <syslog.h>

void handle_signal(int signal);
void sigprocess();
static void exit_nicely();

int main(int argc, char *argv[])
{
	std::cout<<"dir start running ..."<<std::endl;

	sigprocess();

	/*
	 * configure file must be input.
	 */
	std::string configfile("test.configure");
	if(argc > 1)
	{
		configfile.assign(argv[1]);
	}

	std::cout<<"start configure "<<configfile<<std::endl;

	/*
	 * load configurefile ,parse configure parameters.babudb configure so do it.
	 */
	//TODO:

	/*
	 * start dispatch thread
	 */
	//TODO:
	CDIRRequestDispatcher *dirDisp = new CDIRRequestDispatcher();
	dirDisp->startup();
	dirDisp->join();

	delete dirDisp;

	return 0;
}

void sigprocess()
{
	struct sigaction sa;

	/* Catch, ignore and handle signals */
	signal(SIGCHLD, SIG_IGN);

	sa.sa_handler = &handle_signal;
	sa.sa_flags = SA_RESTART;
	sigfillset(&sa.sa_mask);

	if (sigaction(SIGHUP, &sa, NULL) == -1)
	{
		perror("Error: cannot handle SIGHUP"); // Should not happen
	}

	if (sigaction(SIGUSR1, &sa, NULL) == -1)
	{
		perror("Error: cannot handle SIGUSR1"); // Should not happen
	}

	if (sigaction(SIGUSR2, &sa, NULL) == -1)
	{
		perror("Error: cannot handle SIGUSR1"); // Should not happen
	}

	if (sigaction(SIGINT, &sa, NULL) == -1)
	{
		perror("Error: cannot handle SIGINT"); // Should not happen
	}

}

void handle_signal(int signal)
{
	const char *signal_name;
	sigset_t pending;

	switch (signal) {
	case SIGHUP:
		signal_name = "SIGHUP";
		break;
	case SIGUSR1:
		signal_name = "SIGUSR1";
		break;
	case SIGINT:
		syslog(LOG_NOTICE, "Caught SIGINT, exiting now\n");
		exit_nicely();
	default:
		syslog(LOG_NOTICE, "Caught wrong signal: %d\n", signal);
		return;
	}
	syslog(LOG_NOTICE, "Caught signal %s\n", signal_name);

	// So what did you send me while I was asleep?
	sigpending(&pending);
	if (sigismember(&pending, SIGHUP))
	{
		syslog(LOG_NOTICE, "A SIGHUP is waiting\n");
	}

	if (sigismember(&pending, SIGUSR1))
	{
		syslog(LOG_NOTICE, "A SIGUSR1 is waiting\n");
	}

	//It is time to change running...
	//running = 0;

	return;

}

static void exit_nicely()
{

	//closelog();
   exit(1);
}

