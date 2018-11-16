/**
 * test file
 * @author zzp
 * @date 20180905
 */

#include<iostream>
#define BUFFER_BLOCK 1024
int main(int argc, char **argv)
{
	std::cout<<"hello"<<std::endl;
	int length = atoi(*arg[1]);
	int tmpCap = (0 + length) &  ~BUFFER_BLOCK * BUFFER_BLOCK;

	printf("cap:%d,~1024:%d,length:%d\n",tmpCap,~BUFFER_BLOCK,length);
	return 0;
}
