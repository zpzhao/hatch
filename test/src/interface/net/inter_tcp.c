/*
 * inter_tcp.c
 *
 *  Created on: 2019年7月17日
 *      Author: zpzhao
 */


/**
 * create server socket as tcp protocol
 *  Author: zpzhao 2019/07/17
 */
int createTcpServer(int port, int options, char *listenIP)
{
	int server_sockfd = 0;
	socklen_t server_len = 0;

	struct sockaddr_in server_sockaddr;

	/*create a socket.type is AF_INET,sock_stream*/
	server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_port = htons(PORT);
	server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	server_len = sizeof(server_sockaddr);


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

	ret = setsockopt(server_sockfd, SOL_SOCKET, optionName, optionVal, optionValSize);

	return ret;
}


