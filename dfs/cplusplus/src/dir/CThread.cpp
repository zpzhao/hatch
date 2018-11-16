/*
 * CThread.cpp
 *
 *  Created on: 2018年9月10日
 *      Author: zpzhao
 */

#include "CThread.h"
#include <iostream>
#include <time.h>

CThread::CThread() {
	// TODO Auto-generated constructor stub
	tid = 0;
	threadStatus = THREAD_STATUS_NEW;
}

CThread::~CThread() {
	// TODO Auto-generated destructor stub
}

void* CThread::run1() {
	threadStatus = THREAD_STATUS_RUNNING;
	tid = pthread_self();
	run();
	threadStatus = THREAD_STATUS_EXIT;
	tid = 0;
	pthread_exit(NULL);
}

int CThread::start() {
	int iRet = 0;
	iRet = pthread_create(&tid, NULL, thread_proxy_func, this);
	return iRet;
}

pthread_t CThread::getThreadID() {
	return tid;
}

int CThread::getState() {
	return threadStatus;
}

void CThread::join() {
	if (tid > 0) {
		pthread_join(tid, NULL);
	}
}

void * CThread::thread_proxy_func(void * args) {
	CThread * pThread = static_cast<CThread *>(args);

	pThread->run1();

	return NULL;
}

void CThread::join(unsigned long millisTime) {
	if (tid == 0) {
		return;
	}
	if (millisTime == 0) {
		join();
	} else {
		unsigned long k = 0;
		timespec req={0,THREAD_JOIN_CHECK_INTERVAL},rem={0,0};

		while (threadStatus != THREAD_STATUS_EXIT && k <= millisTime) {
			(void) nanosleep(&req, &rem);
			k++;
		}
	}
}

/**
 * get timie elapse ; from start= 1
 */
static time_t start_t = 0;
long CThread::getElapse(int start)
{
	time_t cur_t = time(NULL);
	if(start)
		start_t = time(NULL);
	return cur_t - start_t;
}
