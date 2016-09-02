/*
 * list.cpp
 *
 *  Created on: 2016-8-18
 *      Author: zhaozongpeng
 */


#include "hat_list.h"
#include "hat_mem.h"

CHatList::CHatList()
{

}

CHatList::~CHatList()
{

}

int CHatList::hatPut(void *pData, int iSize)
{
	if((NULL == pData) || (iSize <= 0))
	{
		return -1;
	}

	//ChatList *pNode = NULL;
	//hatMemAlloc((void**)&pNode, iSize);


	return 0;
}

int CHatList::hatGetFirst(void **ppData, int iSize)
{
	return 0;
}

int CHatList::hatGetNext(void **ppData, int iSize)
{
	return 0;
}
