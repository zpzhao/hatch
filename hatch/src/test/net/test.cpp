//============================================================================
// Name        : test.cpp
// Author      : zpzhao
// Version     :
// Copyright   : hatch senllang studio
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>

#define BUFFER_BLOCK 1024

static time_t start_t = 0;
long getElapse(int start) {
	time_t cur_t = time(NULL);
	if (start)
		start_t = time(NULL);
	return cur_t - start_t;
}
/*
 void process(int BuffernLength)
 {
 static unsigned long len = 0;
 static unsigned long packnum = 0;
 long t = 0;
 if(packnum == 0)
 {
 t = getElapse(1);
 }
 packnum++;
 len += BuffernLength/1024/8/1024;
 t = getElapse(0);
 if(t > 10)
 {
 printf("process packnum:%d, bytes(MB):%d,time:%d\n", packnum,len,t);
 t = getElapse(1);
 }
 }

 // main(){
 std::cout<<"hello"<<std::endl;
 int length = 8024;
 int tmpCap = (0 + length) &  ~BUFFER_BLOCK ;

 printf("cap:%d,~1024:%d,length:%d\n",tmpCap,~BUFFER_BLOCK,length);
 int i = 0;
 while(1)
 {
 i++;
 process(i);
 }
 return ;
 }
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 5432
#define MAXDATASIZE 100
#define SERVER_IP "127.0.0.1"

int main()
{
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	struct sockaddr_in server_addr;

	printf("\n======================client initialization======================\n");
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	bzero(&(server_addr.sin_zero), sizeof(server_addr.sin_zero));

	if (connect(sockfd, (struct sockaddr *) &server_addr,
			sizeof(struct sockaddr_in)) == -1)
	{
		perror("connect error");
		exit(1);
	}

	while (1)
	{
		bzero(buf, MAXDATASIZE);
		printf("\nBegin receive...\n");
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) == -1)
		{
			perror("recv");
			exit(1);
		}
		else if (numbytes > 0)
		{
			int len, bytes_sent;
			buf[numbytes] = '\0';
			printf("Received: %s\n", buf);
			printf("Send:");
			char msg[100];
			scanf("%s", msg);
			len = strlen(msg);
			//sent to the server
			if (send(sockfd, msg, len, 0) == -1)
			{
				perror("send error");
			}
		}
		else
		{
			printf("soket end!\n");
			break;
		}
	}
	close(sockfd);
	return 0;
}

