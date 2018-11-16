/*
 * CRPCNIOSocketServer.h
 *
 *  Created on: 2018年9月25日
 *      Author: zpzhao
 */

#ifndef CRPCNIOSOCKETSERVER_H_
#define CRPCNIOSOCKETSERVER_H_

#include "CRPCServerRequestListener.h"
#include "CRPCServerInterface.h"
#include "CThread.h"
#include <string>
#include <sys/epoll.h>

typedef enum QUIT
{
	QUIT_NO    = 0x00,
	QUIT_YES
}QUIT_EN;

class CRPCNIOSocketServer: public CThread, public CRPCServerInterface {
public:
	CRPCNIOSocketServer(CRPCServerRequestListener *rl);
	virtual ~CRPCNIOSocketServer();

public:
	void sendResponse(CServerRequest &request, CRPCServerResponse &response);
	void run();

	void shutdown();
	int InitServer(unsigned int port, std::string address);
	int CreateServer(unsigned int port, std::string address);
	int CreateEventWait();
	int ProcessEvent(int eventNum);
	int AddFd(int fd, int enable_et=0, int oneshot=0);
	int ReadBuff(int fd, char *buff, int size);
private:
	int serversocket;
	int listen_maxconn;
	struct epoll_event *p_ep_events;
	int server_ep;
	int quit;
	CRPCServerRequestListener *receiver;
};

#endif /* CRPCNIOSOCKETSERVER_H_ */
