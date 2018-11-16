/*
 * CServerRequest.cpp
 *
 *  Created on: 2018年9月18日
 *      Author: zpzhao
 */

#include "CServerRequest.h"
#include <string.h>

#define BUFFER_BLOCK 1024

CServerRequest::CServerRequest() {
	// TODO Auto-generated constructor stub
	this->pRequestBuffer = NULL;
	this->BufferCap = 0;
	this->BuffernLength = 0;
}

CServerRequest::~CServerRequest() {
	// TODO Auto-generated destructor stub
	if(this->pRequestBuffer != NULL)
		delete[] this->pRequestBuffer;
	pRequestBuffer = NULL;
}

int CServerRequest::InputBuffer(char *pValue, int length)
{
	if(NULL == pValue)
		return 0;

	if(BufferCap == 0)
	{
		pRequestBuffer = new char[BUFFER_BLOCK];
		if(NULL == pRequestBuffer)
			return 0;
		memset(pRequestBuffer, 0x00, BUFFER_BLOCK);
		BufferCap = BUFFER_BLOCK;
	}

	if(BuffernLength + length >= BufferCap)
	{
		int tmpCap = (BuffernLength + length  + BUFFER_BLOCK) &  ~BUFFER_BLOCK * BUFFER_BLOCK;
		char *tmp = new char[tmpCap];
		if(NULL == tmp)
			return 0;
		memset(tmp, 0x00, tmpCap);
		memcpy(tmp,this->pRequestBuffer, BuffernLength);

		BufferCap = tmpCap;
		delete[] pRequestBuffer;
		pRequestBuffer = tmp;
	}

	memcpy(pRequestBuffer+BuffernLength, pValue, length);
	BuffernLength += length;
	return length;
}
