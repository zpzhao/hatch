/*
 * CDIRRequestDispatcher.cpp
 *
 *  Created on: 2018年9月10日
 *      Author: zpzhao
 */

#include "CDIRRequestDispatcher.h"
#include <iostream>
#define DEBUG(x,y,z)	std::cout<<x<<y<<z<<std::endl;

#define DEFAULT_PORT 8000
#define DEFAULT_IP	  "*"


CDIRRequestDispatcher::CDIRRequestDispatcher()
{
	// TODO Auto-generated constructor stub
	server = NULL;
	queue = NULL;
	numRequests = 0;
	quit = QUIT_NO;

	/* configure */

	/* */
}

CDIRRequestDispatcher::~CDIRRequestDispatcher()
{
	// TODO Auto-generated destructor stub
	if(server != NULL)
		delete server;
	if(queue != NULL)
		delete queue;
}

/**
 *
 */
int CDIRRequestDispatcher::InitDirDispatcher()
{
	int iRet = 0;

	queue = new CBlockingQueue<CServerRequest*>();
	if(NULL == queue)
		return -1;

	server = new CRPCNIOSocketServer(this);
	if(NULL == server)
	{
		return -1;
	}
	iRet = server->InitServer(DEFAULT_PORT, DEFAULT_IP);

	return iRet;
}


/**
 * 接收请求,加入消息队列
 */
void CDIRRequestDispatcher::ReceiveRecord(CServerRequest &rq)
{
	this->queue->put(&rq);
	return ;
}

/**
 * 主线程
 */
void CDIRRequestDispatcher::run()
{
	DEBUG("dispatcher run...","","");
	// TODO: process request
	while(!quit)
	{
		CServerRequest *rq = queue->take();
		ProcessRequest(*rq);
	}
	return ;
}

/**
 * 启动服务线程
 */
void CDIRRequestDispatcher::startup()
{
	DEBUG("startup run...","","");
	server->start();
	int iret = this->start();
	DEBUG("startup run ret = ",iret,"");
}

#define MB_MASK	(~(1024*1024))
/**
 * 处理请求
 */
void CDIRRequestDispatcher::ProcessRequest(CServerRequest &rq)
{
	static unsigned long len = 0;
	static unsigned long packnum = 0;
	long t = 0;
	if(len == 0)
	{
		t = this->getElapse(1);
	}
	packnum++;
	len += rq.BuffernLength/1024/8/1024;
	t = this->getElapse(0);
	if(t > 30)
	{
		printf("process packnum:%d, bytes(MB):%d\n", packnum,len);
		t = this->getElapse(1);
	}
	delete &rq;
}

/**
 * 停止服务线程
 */
void CDIRRequestDispatcher::shutdown()
{
	server->shutdown();
	quit = QUIT_YES;
	this->join();
}
