/*
 * main.cpp
 *
 *  Created on: 2016-8-18
 *      Author: zhaozongpeng
 */

#include "CHatchTest.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
	CHatchTest obj;

	printf("hatch test \n");

	int ret = obj.hatRun();

	printf("hatch test end, ret [%d] \n", ret);
	return 0;
}
