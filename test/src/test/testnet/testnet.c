/*
 * testnet.c
 *
 *  Created on: 2019年7月26日
 *      Author: zpzhao
 */


#include "inter_tcp.h"

int testnet_main(int argc, char *argv[])
{
	int ret = 0;

	printf("testnet start [%d] \n",argc);

	if((argc > 1) && (bcmp(argv[1],"client", sizeof("client")-1) == 0))
	{
		/* client */
		client = 1;
		ret = clientOpen("127.0.0.1","6000","8000");
		if(ret < 0)
		{
			LOG("create client err[%#x]\n", ret);
			return ret;
		}

		printf("client end \n");
		return ret;
	}

	ret = serverOpen("127.0.0.1","6000");
	if(ret < 0)
	{
		LOG("create server err[%#x]\n",ret);
		return ret;
	}

	printf("server end\n");
	return 0;
}
