/*
 * CDIRRequestDispatcher.cpp
 *
 *  Created on: 2018年9月10日
 *      Author: zpzhao
 */

#include "CDIRRequestDispatcher.h"
#include <iostream>
#define DEBUG(x,y,z)	std::cout<<x<<y<<z<<std::endl;

CDIRRequestDispatcher::CDIRRequestDispatcher() {
	// TODO Auto-generated constructor stub
	/* configure */

	/* */
}

CDIRRequestDispatcher::~CDIRRequestDispatcher() {
	// TODO Auto-generated destructor stub
}

/**
 * 接收请求,加入消息队列
 */
void CDIRRequestDispatcher::ReceiveRecord(CServerRequest rq)
{
	this->queue.put(rq);
	return ;
}

/**
 * 主线程
 */
void CDIRRequestDispatcher::run()
{
	DEBUG("dispatcher run...","","");
	// TODO: process request

	return ;
}

/**
 * 启动服务线程
 */
void CDIRRequestDispatcher::startup()
{
	DEBUG("startup run...","","");
	int iret = this->start();
	DEBUG("startup run ret = ",iret,"");
}
