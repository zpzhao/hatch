/*
 * hat_mem.cpp
 *
 *  Created on: 2016-8-22
 *      Author: zhaozongpeng
 */

#include "hat_mem.h"
#include <new>

int hatMemAlloc(void **ppaddr, int iSize)
{
	*ppaddr = new(std::nothrow) char[iSize];
	if(NULL == *ppaddr)
		return -1;
	return 0;
}

int hatMemFree(void *addr)
{
	if(NULL != addr)
		delete[] (char*)addr;
}
