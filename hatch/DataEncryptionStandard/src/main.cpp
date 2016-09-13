/*
 * main.cpp
 *
 *  Created on: 2016-9-9
 *      Author: zhaozongpeng
 */


#include "hat_des.h"
#include <string.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	if(argc <= 1)
		return 0;

	char *pkey = "mykey";
	if(argc > 5)
		pkey = argv[4];

	if(strcmp(argv[1], "-c") == 0)
			DES_Encrypt(argv[2], pkey, argv[3]);


	if(strcmp(argv[1], "-x") == 0)
		DES_Decrypt(argv[2], pkey, argv[3]);


	printf("success end\n");
	return 0;
}
