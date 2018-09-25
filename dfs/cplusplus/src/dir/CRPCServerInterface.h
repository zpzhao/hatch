/*
 * CRPCServerInterface.h
 *
 *  Created on: 2018年9月25日
 *      Author: zpzhao
 */

#ifndef CRPCSERVERINTERFACE_H_
#define CRPCSERVERINTERFACE_H_

class CRPCServerResponse;
class CRPCServerRequest;

class CRPCServerInterface {
public:
	CRPCServerInterface();
	virtual ~CRPCServerInterface();

public:
	virtual void sendResponse(CRPCServerRequest request, CRPCServerResponse response) = 0;
};

#endif /* CRPCSERVERINTERFACE_H_ */
