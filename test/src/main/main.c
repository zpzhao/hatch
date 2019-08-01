/*
 * main.c
 *
 *  Created on: 2019年7月16日
 *      Author: zpzhao
 */

#include <stdio.h>
#define TEST_CASE

#ifdef TEST_CASE
#include "testnet.h"
#endif


int main(int argc, char *argv[])
{
	printf("test: hello world\n");

#ifdef TEST_CASE
	(void)testnet_main(argc, argv);
#endif

	return 0;
}


