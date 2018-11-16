/*
 * CRPCServerRequestListener.h
 *
 *  Created on: 2018年10月12日
 *      Author: zpzhao
 */

#ifndef CRPCSERVERREQUESTLISTENER_H_
#define CRPCSERVERREQUESTLISTENER_H_

class CServerRequest;

class CRPCServerRequestListener {
public:
	CRPCServerRequestListener();
	virtual ~CRPCServerRequestListener();
public:
	virtual void ReceiveRecord(CServerRequest &rq) = 0;
};

#endif /* CRPCSERVERREQUESTLISTENER_H_ */
