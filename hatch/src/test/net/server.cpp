/*
 * server.cpp
 *
 *  Created on: 2019年1月19日
 *      Author: zpzhao
 */


#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>

#define PORT 5433
#define KEY 123
#define SIZE 1024

int main()
{

    char buf[SIZE];
    memset(buf,0,SIZE);

    int server_sockfd,client_sockfd;
    socklen_t server_len,client_len;

    struct  sockaddr_in server_sockaddr,client_sockaddr;

    /*create a socket.type is AF_INET,sock_stream*/
    server_sockfd = socket(AF_INET,SOCK_STREAM,0);

    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(PORT);
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    server_len = sizeof(server_sockaddr);

    int on;
    setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR,&on,sizeof(on));
    /*bind a socket or rename a sockt*/
    if(bind(server_sockfd, (struct sockaddr*)&server_sockaddr, server_len)==-1)
    {
        printf("bind error");
        exit(1);
    }

    if(listen(server_sockfd, 5)==-1){
        printf("listen error");
        exit(1);
    }

    client_len = sizeof(client_sockaddr);

    pid_t ppid,pid;
    int error ;
    while(1) {

      if((client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_sockaddr, &client_len)) == -1)
        {
            printf("connect error");
            exit(1);
        }
      else
        {
            printf("create connection successfully\n");
            error = recv(client_sockfd, buf, SIZE,0);
            	printf("recv [%d]: %s\n",  error, buf);
            	for(int i = 0 ; i < error; i++)
            	{
            		if(i%16 == 0)
            			printf("\n");
            		printf("%#x ",buf[i]);
            	}

            	error = send(client_sockfd, "You have conected the server", strlen("You have conected the server"), 0);
            printf("\n send:%d\n", error);
        }
 }
    return 0;
}


