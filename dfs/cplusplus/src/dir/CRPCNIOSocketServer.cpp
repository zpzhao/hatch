/*
 * CRPCNIOSocketServer.cpp
 *
 *  Created on: 2018年9月25日
 *      Author: zpzhao
 */

#include "CRPCNIOSocketServer.h"
#include "CServerRequest.h"
#include "CRPCServerResponse.h"
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <cstring>

#define DEFAULT_MAXCONN	100

CRPCNIOSocketServer::CRPCNIOSocketServer(CRPCServerRequestListener *rl)
{
	// TODO Auto-generated constructor stub
	serversocket = 0;
	listen_maxconn = DEFAULT_MAXCONN;
	this->server_ep = 0;
	this->p_ep_events = NULL;
	this->quit = QUIT_NO;
	receiver = rl;
}

CRPCNIOSocketServer::~CRPCNIOSocketServer()
{
	// TODO Auto-generated destructor stub
}

/**
 * 发送响应消息
 */
void CRPCNIOSocketServer::sendResponse(CServerRequest &request, CRPCServerResponse &response)
{
	return ;
}

/**
 * 线程处理
 */
void CRPCNIOSocketServer::run()
{
	int eventNum = 0;
	int iRet = 0;

	while(quit != QUIT_YES)
	{
		// no timeout limits
		eventNum = epoll_wait(this->server_ep, p_ep_events, listen_maxconn, -1);
		if(eventNum < 0)
		{
			//printf("wait err:%d- %d - %d \n",errno,server_ep, listen_maxconn);
			usleep(100);
			continue;
		}

		iRet = ProcessEvent(eventNum);
		if(iRet < 0)
		{
			printf("process Event err[%d]\n",errno); // TODO:
		}
	}

	// exit server
	close(this->server_ep);
	close(this->serversocket);
	delete[] this->p_ep_events;

	return;
}

char *pbuff = new char[8024];
/**
 * socket事件处理
 */
int CRPCNIOSocketServer::ProcessEvent(int eventNum)
{
	int iRet = 0;

	for(int i = 0; i < eventNum; i++)
	{
		int sockfd = this->p_ep_events[i].data.fd;
		if(sockfd == this->serversocket)
		{
			// accept new connect
			int connfd = 0;
			struct sockaddr_in connsockaddr;
			socklen_t sockaddrlen = sizeof(connsockaddr);

			if ((connfd = accept(this->serversocket,
					(struct sockaddr *) &connsockaddr, &sockaddrlen)) < 0) {
				return connfd;
			}
			printf("connect server[%s:%d]\n",inet_ntoa(connsockaddr.sin_addr),connsockaddr.sin_port);

			AddFd(connfd); // default edgetrrige and oneshot
		}
		else if(this->p_ep_events[i].events & EPOLLIN)
		{
			// readbuf, by multiple threads
			int len = ReadBuff(this->p_ep_events[i].data.fd, pbuff, 8024);

			CServerRequest *rq = new CServerRequest();
			rq->InputBuffer(pbuff, len);

			// receiver.ReceiveRecord
			this->receiver->ReceiveRecord(*rq);
		}
		else if(this->p_ep_events[i].events & EPOLLOUT)
		{
			// sendbuf, in single threads, block socket;
			;
		}
		else
		{
			//TODO:
			printf("other epoll event[%#x]-fd[%#x]\n", this->p_ep_events[i], sockfd);
		}
	}

	return 0;
}

/**
 * 读消息
 */
int CRPCNIOSocketServer::ReadBuff(int fd , char *buff, int size)
{
	int len = 0;

	len = recv(fd, buff, size, 0);

	return len;
}

/**
 * 添加socket fd及对应事件到epoll中
 */
int CRPCNIOSocketServer::AddFd(int fd, int enable_et, int oneshot)
{
	struct epoll_event ep_event;
	memset((void*)&ep_event, 0x00, sizeof(ep_event));

	//ep_event.events = EPOLLIN | EPOLLOUT;
	ep_event.events = EPOLLIN ;
	ep_event.data.fd = fd;

	if(enable_et)
		ep_event.events |= EPOLLET;
	if(oneshot)
		ep_event.events |= EPOLLONESHOT;

	int iRet = epoll_ctl(this->server_ep, EPOLL_CTL_ADD, fd, &ep_event);
	return iRet;
}

/**
 * 结束线程运行
 */
void CRPCNIOSocketServer::shutdown()
{
	quit = QUIT_YES;
	this->join();
}

/**
 * 创建socket,并设置参数
 */
int CRPCNIOSocketServer::CreateServer(unsigned int port, std::string address)
{
	if(serversocket > 0)
		return -1;

	serversocket = socket(AF_INET,SOCK_STREAM, 0);
	if(serversocket < 0)
	{
		return -1;
	}

	int oneValue = 1;
	struct linger li = {0,0};
	int ret = 0;

	ret = setsockopt(serversocket, SOL_SOCKET, SO_REUSEADDR, &oneValue, sizeof(oneValue));
	ret |= setsockopt(serversocket, SOL_SOCKET, SO_KEEPALIVE, &oneValue, sizeof(oneValue));
	ret |= setsockopt(serversocket, SOL_SOCKET, SO_LINGER, &li, sizeof(li));

	if(ret < 0)
	{
		printf("setsocketopt err[%d]\n",errno);
		close(serversocket);
		serversocket = -1;
		return -1;
	}

	struct sockaddr_in serveraddr;
	if(address == "*")
		serveraddr.sin_addr.s_addr = htons(INADDR_ANY);
	else
		serveraddr.sin_addr.s_addr = inet_addr(address.c_str());
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);

	if(bind(serversocket, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
	{
		printf("bind err[%d]\n",errno);
		close(serversocket);
		serversocket = -1;
		return -1;
	}

	if(listen(serversocket, listen_maxconn) < 0)
	{
		printf("listen err[%d]\n",errno);
		close(serversocket);
		serversocket = -1;
		return -1;
	}

	return 0;
}

/**
 *	建立事件监听
 */
int CRPCNIOSocketServer::CreateEventWait()
{
	if(p_ep_events)
		return -1;

	p_ep_events = new struct epoll_event[listen_maxconn];
	if(NULL == p_ep_events)
		return -1;

	int flag = EPOLL_CLOEXEC;
	server_ep = epoll_create1(flag);
	printf("ep:%d - %d\n",errno, server_ep);

	return server_ep;
}

int CRPCNIOSocketServer::InitServer(unsigned int port, std::string address)
{
	int iRet = 0;
	iRet = CreateServer(port, address);
	if(iRet < 0)
	{
		printf("createserver err[%d]\n",errno);
		return iRet;
	}
	iRet = CreateEventWait();
	if(iRet < 0)
	{
		printf("create event err[%d]\n",errno);
		return iRet;
	}

	iRet = AddFd(this->serversocket);

	return iRet;
}



