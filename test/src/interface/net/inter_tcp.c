/*
 * inter_tcp.c
 *
 *  Created on: 2019年7月17日
 *      Author: zpzhao
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include "inter_tcp.h"


/*
 * gobal variables
 */

/* default tcp protocol */
int protocol = SOCK_STREAM;
int listenq = 5;		/* listen queue for TCP Server */
int isfork = 1;			/* after client connection , procedure with a process by fork */
int client = 0;			/* 1 is client , 0 is server */

/* send and receive buffers */
char rbuf[RECV_BUF_MAX];
char wbuf[SEND_BUF_MAX];
unsigned long rbuf_len;		/* recevied data length */
unsigned long wbuf_len;		/* send data length */


/**
 * create server socket as tcp protocol
 *  Author: zpzhao 2019/07/17
 */
int createTcpServer(int port, int options, char *listenIP)
{
	int server_sockfd = 0;

	/*create a socket.type is AF_INET,sock_stream*/
	server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

	return server_sockfd;
}

/**
 * set socket options
 *  Author: zpzhao 2019/07/17
 */
int setOptions(int optionName, char *optionVal, int optionValSize)
{
	int ret = 0;
	if(optionVal == NULL)
	{
		return -1;
	}

	//ret = setsockopt(server_sockfd, SOL_SOCKET, optionName, optionVal, optionValSize);

	return ret;
}

/**
 * create a server, which is tcp or udp protocol.
 * if protocol is tcp, bind and ready to accept.
 *
 * return fd if sucessed, otherwise -1
 * Author : zpzhao 2019/7/22
 */
int serverOpen(char *host, char *port)
{
	int fd_serv = 0;
	int fd_cli = 0;
	struct sockaddr_in server_in, client_in;
	struct servent *pserv_port = NULL;
	unsigned long inaddr = 0;
	int inaddr_len = 0;
	int pid = 0; /* when if fork=1 , to use  */
	int port_num = 0;

	/* address process */
	bzero(&server_in, sizeof(server_in));
	server_in.sin_family = AF_INET;

	/* port */
	if((port_num = atoi(port)) == 0)
	{
		pserv_port = getservbyname(port, protocol == SOCK_STREAM ? "tcp" : "udp");
		if(NULL == pserv_port)
		{
			LOG("getserverbyname err.errcode[%d]\n", errno);
			return ERR_GETSERVERBYNAME;
		}
		server_in.sin_port = pserv_port->s_port;
	}
	else
	{
		server_in.sin_port = htons(port_num);
	}

	/* host */
	if(NULL == host)
	{
		server_in.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	else
	{
		if((inaddr = inet_addr(host)) == INADDR_NONE)
		{
			LOG("host address is err.\n");
			return ERR_HOST_ADDR;
		}

		server_in.sin_addr.s_addr = inaddr;
	}

	/* init socket fd */
	fd_serv = socket(AF_INET, protocol, 0);
	if(fd_serv < 0)
	{
		LOG("create socket err, errcode[%s],pro[%d]\n",strerror(errno), protocol);
		return ERR_NET_OPEN_SOCKET;
	}

	/* bind  */
	if(bind(fd_serv, &server_in, sizeof(server_in)) < 0)
	{
		LOG("bind socket err, errcode[%s],pro[%d]\n", strerror(errno), protocol);
		return ERR_NET_BIND;
	}

	if(SOCK_DGRAM == protocol)
	{
		/* nothing to do if udp protocol */
		return fd_serv;
	}

	/* tcp protocol conntinue */
	/* listen */
	listen(fd_serv, listenq);

	/* wait for client connections, per connection will fork a sepecial process; */
	for(;;)
	{
		/* accept client connections */
		inaddr_len = sizeof(client_in);
		fd_cli = accept(fd_serv, (struct sockaddr *)&client_in, &inaddr_len);
		if(fd_cli < 0)
		{
			LOG("accept err.ecode[%s]\n", strerror(errno));
			sleep(1);
			continue;
		}

		if(isfork)
		{
			pid = fork();
			if(pid < 0)
			{
				LOG("fork err.errcode[%s]\n", errno);
				return ERR_CLIENT_FORK;
			}
			else if(pid > 0)
			{
				/* parent , continue to accept */
				close(fd_cli);
				continue;
			}
			else
			{
				/* child */
				close(fd_serv);

				/* child process don't cycle accept. */
				break;
			}
		}

		/* connection end */
	}

	/* client fd returned */
	return fd_cli;
}

int clientOpen(char *host, char *port, char *bindport)
{
	int cli_fd = 0;
	int port_num = 0;
	struct servent *pserv_port = NULL;
	struct sockaddr_in serv_in, cli_in;
	unsigned long inaddr;

	/* server addr. host, port */
	bzero(&serv_in, sizeof(serv_in));
	serv_in.sin_family = AF_INET;

	/* port */
	if ((port_num = atoi(port)) == 0)
	{
		pserv_port = getservbyname(port, protocol == SOCK_STREAM ? "tcp" : "udp");
		if (NULL == pserv_port)
		{
			LOG("getserverbyname err.errcode[%d]\n", errno);
			return ERR_GETSERVERBYNAME;
		}
		serv_in.sin_port = pserv_port->s_port;
	}
	else
	{
		serv_in.sin_port = htons(port_num);
	}

	/* host */
	if (NULL == host)
	{
		serv_in.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	else
	{
		if ((inaddr = inet_addr(host)) == INADDR_NONE)
		{
			LOG("host address is err.\n");
			return ERR_HOST_ADDR;
		}

		serv_in.sin_addr.s_addr = inaddr;
	}

	/* socket initial */
	cli_fd = socket(AF_INET, protocol, 0);
	if(cli_fd < 0)
	{
		LOG("clientOpen socket open err.ecode[%d]\n",errno);
		return ERR_NET_OPEN_SOCKET;
	}

	/* bind port */
	if(bindport || (protocol == SOCK_DGRAM))
	{
		bzero(&cli_in, sizeof(cli_in));
		cli_in.sin_family = AF_INET;
		/* TODO: here use wildcare ipaddress, also useful ip instead it. */
		cli_in.sin_addr.s_addr = htonl(INADDR_ANY);

		bind(cli_fd, (struct sockaddr *)&cli_in, sizeof(cli_in));
	}

	/* connect to server. required by tcp, optional udp. */
	if(connect(cli_fd, (struct sockaddr *)&serv_in, sizeof(serv_in)) < 0)
	{
		close(cli_fd);
		return ERR_CLIENT_CONNECTION;
	}

	return cli_fd;
}

/*
 * fd_sock is a socket which already connected.
 * here to recevie and send package. analyze command.
 * author:zpzhao 2019/07/30
 */
int sink(int fd_sock)
{
	int n = 0;
	unsigned long xno = 0;

	if(client)
	{
		/*
		 * client stage: first send, recv
		 */
		bzero(wbuf,sizeof(wbuf));
		// fillSendData(wbuf, SEND_BUF_MAX);
		sprintf(wbuf, "%d", getpid());

		for(int i = 0; i < CLIENT_SEND_CNT; i++)
		{
			n = send(fd_sock, wbuf, SEND_BUF_MAX,0);
			if(n < SEND_BUF_MAX)
			{
				LOG("send [%d][%d], client[%d][%s]\n",n , SEND_BUF_MAX, client,wbuf);
			}

			n = recv(fd_sock,rbuf, RECV_BUF_MAX,0);
			LOG("recv [%d], context[%s]\n",n , rbuf);
		}
	}
	else
	{
		/* server cycle */
		for(;;)
		{
			bzero(rbuf, sizeof(rbuf));

			n = recv(fd_sock, rbuf, RECV_BUF_MAX, 0);
			if(n < 0)
			{
				LOG("recv err [%s]\n", strerror(errno));
				break;
			}
			else if(n == 0)
			{
				/* connection closed by peer. */
				break;
			}
			else if(n < SEND_BUF_MAX)
			{
				LOG("recv [%d][%d], server\n", n, SEND_BUF_MAX);
			}
			LOG("recv client[%s] request\n", rbuf);

			sprintf(wbuf, "%d", xno++);
			n = send(fd_sock, wbuf, SEND_BUF_MAX, 0);

		}
	}


	return 0;
}

void fillSendData(char *buf, unsigned long len)
{
	char c = 0;
	char *ptr = buf;

	if((NULL == ptr) || (len == 0))
		return ;

	while(len-- > 0)
	{
		while(isprint(c & 0x7F) == 0)
			c++;
		*ptr++ = (c++ & 0x7F);
	}
}
