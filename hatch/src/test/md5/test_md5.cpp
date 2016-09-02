
#include "stdio.h"
#include "string.h"
#include "md5.h"

/*
 * md5 Ëã·¨²âÊÔÑùÀý
 */
void test_md5(void)
{
	MD5_CTX md5;
	MD5Init(&md5);
	int i;
	unsigned char encrypt[] ="zzpwyp123";//21232f297a57a5a743894a0e4a801fc3
	unsigned char decrypt[16];
	MD5Update(&md5,encrypt,strlen((char *)encrypt));
	MD5Final(&md5,decrypt);
	printf("encrypt pre:%s\n encrypt after:",encrypt);
	for(i=0;i<16;i++)
	{
		printf("%02x",decrypt[i]);
	}
	printf("\n");
}
