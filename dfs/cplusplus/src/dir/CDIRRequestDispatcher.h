/*
 * CDIRRequestDispatcher.h
 *
 *  Created on: 2018年9月10日
 *      Author: zpzhao
 */

#ifndef CDIRREQUESTDISPATCHER_H_
#define CDIRREQUESTDISPATCHER_H_

#include "CRPCServerRequestListener.h"
#include "CRPCNIOSocketServer.h"
#include "CBlockingQueue.h"
#include "CServerRequest.h"
#include "CThread.h"
#include <map>

class CDIROperation;
typedef std::map<int,CDIROperation&> OperationMap;

class CDIRRequestDispatcher: public CThread , public CRPCServerRequestListener {
public:
	CDIRRequestDispatcher();
	virtual ~CDIRRequestDispatcher();

	void ReceiveRecord(CServerRequest &rq);
	void ProcessRequest(CServerRequest &rq);
	/**
	 * thread interface implement
	 */
	void run();
	void startup();
	void shutdown();

	int InitDirDispatcher();
private:
	OperationMap registry;
	CBlockingQueue<CServerRequest*> *queue;
	CRPCNIOSocketServer *server;
	int numRequests;
	int quit;
};

#endif /* CDIRREQUESTDISPATCHER_H_ */
