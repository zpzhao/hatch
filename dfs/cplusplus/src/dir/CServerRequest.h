/*
 * CServerRequest.h
 *
 *  Created on: 2018年9月18日
 *      Author: zpzhao
 */

#ifndef CSERVERREQUEST_H_
#define CSERVERREQUEST_H_

class CServerRequest {
public:
	CServerRequest();
	virtual ~CServerRequest();
	int InputBuffer(char *pValue, int length);

	long BuffernLength;
private:
	// temp buffer
	char *pRequestBuffer;

	long BufferCap;
};

#endif /* CSERVERREQUEST_H_ */
