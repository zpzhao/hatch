//#include <stdlib.h>
//#include <stdio.h>
//#include "winsock2.h"
//#include <windows.h>
//#include <string.h>
//#include "OpenSSL.h"
//#pragma comment(lib, "wininet.lib")
//
//char *pStrPathName = "/home/test1/";
//void SendCmd(char *pBuf, int iLen)
//{
//	char* pBuf = NULL;
//	unsigned char cMainCmd = 17;
//	unsigned short usLen;
//	unsigned int uiPathNameLen = strlen(pStrPathName);
//	unsigned int uiBuffSize =  uiPathNameLen * 8;
//	wchar_t* pszBuf = new wchar_t[uiBuffSize];
//	char* pUtf8Buf = new char[uiBuffSize];
//	memset(pszBuf, 0, uiBuffSize*sizeof(wchar_t));
//	memset(pUtf8Buf, 0, uiBuffSize*sizeof(char));
//	ConvMulti2Width(pszBuf, uiBuffSize, pStrPathName);
//	ConvWidth2UTF8(pUtf8Buf, uiBuffSize, pszBuf);
//	usLen = strlen(pUtf8Buf);
//	unsigned int uiLen = sizeof(cMainCmd) + sizeof(usLen) + usLen;
//	pBuf = new char[uiLen+1];
//
//	unsigned int uiOffset = 0;
//	memcpy(pBuf+uiOffset, &cMainCmd, sizeof(cMainCmd));
//	uiOffset += sizeof(cMainCmd);
//	memcpy(pBuf+uiOffset, &usLen, sizeof(usLen));
//	uiOffset += sizeof(usLen);
//	memcpy(pBuf+uiOffset, pUtf8Buf, usLen);
//
//	int Len_rest = 0,Len = 0;
//    char tmp[8] = {0}; 
//	char in[8] = {0}; 
//	char out[8] = {0};
//	char senlen[8] = {0};
//	char *DES_buff = NULL;
//	char *DES_char = NULL;
//	int bufflen = usLen;
//	Len_rest = bufflen % 8;
//	if(Len_rest)
//	{
//	  Len = bufflen + (8 - Len_rest);
//	}
//	else 
//	{
//	  Len = bufflen;
//	}
//
//	DES_buff = (char *)malloc(Len);
//	DES_char = (char *)malloc(Len+8);
//    if ((NULL == DES_buff) || (NULL == DES_char))
//    {    
//        return;
//    }
//
//	memset(DES_char,0,Len+8);
//	memset(DES_buff,0,Len);
//	memcpy(DES_buff,lpBuf,bufflen);
//	memset(senlen,0,8);
//
//	COpenSSL m_openssl;
//	_itoa_s(Len+8,senlen,sizeof(senlen), 10);
//	memcpy(DES_char,senlen,8);
//	int count = Len/8;
//    for(int i=0;i<count;i++)
//	{
//		memset(in, 0, 8); 
//		memset(out, 0, 8); 
//		memcpy(in, DES_buff + 8 * i, 8);
//		m_openssl.DES3_ENCODE(in,out);
//        memcpy(DES_char + 8 * (i+1), out, 8);
//	}
//    // modify by liuyujing for 增加消息队列 on 2015.5.20
//    m_pMsgQueue->push_tail(DES_char,Len+8);
//}
///*
//int main(int argc, char *argv[])
//{
//	// Initialize Winsock
//	WSADATA wsaData;
//	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
//	if (iResult != NO_ERROR)
//	{
//		printf("Error at WSAStartup()\n");
//		return 0;
//	}
//
//	// Create a SOCKET for connecting to server
//	SOCKET ConnectSocket;
//	ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//	if (ConnectSocket == INVALID_SOCKET) {
//		printf("Error at socket(): %ld\n", WSAGetLastError());
//		WSACleanup();
//		return 0;
//	}
//
//	// The sockaddr_in structure specifies the address family,
//	// IP address, and port of the server to be connected to.
//	sockaddr_in clientService; 
//	clientService.sin_family = AF_INET;
//	clientService.sin_addr.s_addr = inet_addr( "192.168.1.110" );
//	clientService.sin_port = htons( 4611 );
//
//
//	if ( connect( ConnectSocket, (SOCKADDR*) &clientService, sizeof(clientService) ) == SOCKET_ERROR) {
//		printf( "Failed to connect.\n" );
//		WSACleanup();
//		return;
//	}
//
//
//	// Send an initial buffer
//	iResult = send( ConnectSocket, sendbuf, (int)strlen(sendbuf), 0 );
//	if (iResult == SOCKET_ERROR) {
//		printf("send failed: %d\n", WSAGetLastError());
//		closesocket(ConnectSocket);
//		WSACleanup();
//		return 1;
//	}
//
//	printf("Bytes Sent: %ld\n", iResult);
//
//	// shutdown the connection since no more data will be sent
//	//iResult = shutdown(ConnectSocket, SD_SEND);
//	//if (iResult == SOCKET_ERROR) {
//	//    printf("shutdown failed: %d\n", WSAGetLastError());
//	//    closesocket(ConnectSocket);
//	//    WSACleanup();
//	//    return 1;
//	//}
//
//	// Receive until the peer closes the connection
//#define RECV_BUF_MAX	1024
//
//	int iResult = 0;
//	unsigned char recvbuf[RECV_BUF_MAX] = {0};
//	unsigned long iRecvTotalBytes = 0;
//	do {
//
//		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
//		if ( iResult > 0 )
//			printf("Bytes received: %d\n", iResult);
//		else if ( iResult == 0 )
//			printf("Connection closed\n");
//		else
//			printf("recv failed: %d\n", WSAGetLastError());
//		iRecvTotalBytes += iResult;
//		printf("recv %d ,total:%d\n",iResult,iRecvTotalBytes);
//	} while( iResult > 0 );
//
//	closesocket(ConnectSocket);
//	WSACleanup();
//	return 0;
//}
//*/
