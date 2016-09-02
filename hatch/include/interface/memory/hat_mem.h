/*
 * hat_mem.h
 *
 *  Created on: 2016-8-22
 *      Author: zhaozongpeng
 */

#ifndef HAT_MEM_H_
#define HAT_MEM_H_

#ifndef NULL
#define NULL ((void*)0)
#endif

int hatMemAlloc(void **ppaddr, int iSize);
int hatMemFree(void *paddr);

#endif /* HAT_MEM_H_ */
