/*
 * CThread.h
 *
 *  Created on: 2018年9月10日
 *      Author: zpzhao
 */

#ifndef CTHREAD_H_
#define CTHREAD_H_

#include <pthread.h>

#define THREAD_JOIN_CHECK_INTERVAL		100000		//nanoseconds
class CThread {
public:
	CThread();
	virtual ~CThread();

public:
	//线程的状态－新建
	static const int THREAD_STATUS_NEW = 0;
	//线程的状态－正在运行
	static const int THREAD_STATUS_RUNNING = 1;
	//线程的状态－运行结束
	static const int THREAD_STATUS_EXIT = -1;

	//线程的运行实体
	virtual void run()=0;
	//开始执行线程
	int start();
	//获取线程ID
	pthread_t getThreadID();
	//获取线程状态
	int getState();
	//等待线程直至退出
	void join();
	//等待线程退出或者超时
	void join(unsigned long millisTime);
	//计算时间差seconds
	long getElapse(int start=1);

private:
	//当前线程的线程ID
	pthread_t tid;
	//线程的状态
	int threadStatus;
	//获取执行方法的指针
	static void * thread_proxy_func(void * args);
	//内部执行方法
	void* run1();
};

#endif /* CTHREAD_H_ */
