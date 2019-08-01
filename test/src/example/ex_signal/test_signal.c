/*
 * test_signal.c
 *
 *  Created on: 2019年7月19日
 *      Author: zpzhao
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>


/* SIGALRM signaled flag, default is 0 */
static int alarmFlag = 0;


void signal_proc(int sig);
void sigalarm_ex();
void sigzeromask_ex();

/**
 * signals between processes or threads to examples
 *
 */
int signal_main()
{

	// sigalarm_ex();
	// sigzeromask_ex();

	printf("signal test end.\n");

	return 0;
}


/**
 * signal procedure
 * input sig is signale id
 */
void signal_proc(int sig)
{
	switch(sig)
	{
	case SIGALRM:
		printf("recevied alarm[%d] and procedured\n");
		break;
	case SIGUSR2:
		break;
	case SIGUSR1:
		break;
	default:
		printf("recevied sig:%d, no set procedure.\n",sig);
		break;
	}
}

/**
 * SIGALARM signal example
 */
void sigalarm_ex()
{
	(void)signal(SIGALRM, signal_proc);

	alarm(10);

	pause();
}

/**
 * empty sigmask bits test
 * no response for all signals, except terminal or int etc , result process end.
 */
void sigzeromask_ex()
{
	sigset_t zerosg, igsg;

	sigemptyset(&zerosg);

	while(1)
	{
		printf("sigzero suspend start.\n");
		sigsuspend(&zerosg);
		printf("sigzero suspend end.\n");
	}
}
