/*
 * inter_tcp.h
 *
 *  Created on: 2019年7月17日
 *      Author: zpzhao
 */

#ifndef INCLUDE_INTERFACE_NET_INTER_TCP_H_
#define INCLUDE_INTERFACE_NET_INTER_TCP_H_

/* debug log print  */
#define LOG printf

extern int protocol;
extern int isfork;
extern int client;


/* socket buffers */
#define RECV_BUF_MAX	1024
#define SEND_BUF_MAX	1024


/* user define here */
#define CLIENT_SEND_CNT	10000


#define MODULE_NO_OFFSET_BITS	23		/* module number is 8bits, in 23-31 bits of 32bits errcode */
#define MODULE_NO_NET		0x10				/* module number */
#define ERR_CODE_MD_START	0x1000	  /* module errcode start number, it's up to you. */


typedef enum ERR_CODE_NET_MD
{
	/* errcode is 32bits, and highest bit is 1 which ensure negtive.  */
	ERR_NET_BEGING = (0x80000000 | (MODULE_NO_NET<<MODULE_NO_OFFSET_BITS) | ERR_CODE_MD_START),
	ERR_NET_OPEN_SOCKET,
	ERR_NET_BIND,
	ERR_HOST_ADDR,
	ERR_CLIENT_CONNECTION,
	ERR_CLIENT_FORK,
	ERR_GETSERVERBYNAME,

	ERR_NET_END
};



/* create server bind or listen, and ready accept client */
int serverOpen(char *host, char *port);
/* create udp or tcp client, and connect to server.
 * input server ip/port, and client bind port */
int clientOpen(char *host, char *port, char *bindport);

/*
 * fd_sock is a socket which already connected.
 * here to recevie and send package. analyze command.
 * author:zpzhao 2019/07/30
 */
int sink(int fd_sock);

/* filled buf with print character */
void fillSendData(char *buf, unsigned long len);

#endif /* INCLUDE_INTERFACE_NET_INTER_TCP_H_ */
