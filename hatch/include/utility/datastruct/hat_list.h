/*
 * hat_list.h
 *
 *  Created on: 2016-8-18
 *      Author: zhaozongpeng
 */

#ifndef HAT_LIST_H_
#define HAT_LIST_H_


class CHatList
{
public:
	CHatList();
	virtual ~CHatList();

	int hatPut(void *pData, int iSize);
	int hatGetFirst(void **ppData, int iSize);
	int hatGetNext(void **ppData, int iSize);

private:
	void *m_pData;
	CHatList *m_pHeader;
	CHatList *m_pTail;
	CHatList *pNext;
};

#endif /* LIST_H_ */
