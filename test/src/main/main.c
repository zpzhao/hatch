/*
 * main.c
 *
 *  Created on: 2019年7月16日
 *      Author: zpzhao
 */

#include <stdio.h>
#include "test_signal.h"

int main(int argc, char *argv[])
{
	printf("test: hello world\n");

	(void)signal_main();

	return 0;
}
