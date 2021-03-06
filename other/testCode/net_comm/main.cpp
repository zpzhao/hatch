#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "winsock2.h"
#pragma comment(lib, "Ws2_32.lib")

#pragma comment(lib, "wininet.lib")
  
  void main() {
    //----------------------
    // Initialize Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != NO_ERROR)
      printf("Error at WSAStartup()\n");
  
    //----------------------
    // Create a SOCKET for connecting to server
    SOCKET ConnectSocket;
    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET) {
      printf("Error at socket(): %ld\n", WSAGetLastError());
      WSACleanup();
      return;
    }
  
    //----------------------
    // The sockaddr_in structure specifies the address family,
    // IP address, and port of the server to be connected to.
    sockaddr_in clientService; 
    clientService.sin_family = AF_INET;
    clientService.sin_addr.s_addr = inet_addr("192.168.1.137");
    clientService.sin_port = htons( 5000 );
  
    //----------------------
    // Connect to server.
    if ( connect( ConnectSocket, (SOCKADDR*) &clientService, sizeof(clientService) ) == SOCKET_ERROR) {
      printf( "Failed to connect.\n" );
      WSACleanup();
      return;
    }
  
    printf("Connected to server.\n");
    char sendbuf[1024];
    memset(sendbuf,0x00,sizeof(sendbuf));
    strcpy(sendbuf,"hello");
    send(ConnectSocket,sendbuf, strlen(sendbuf),0);
    printf("send %s\n", sendbuf);
    recv(ConnectSocket, sendbuf,sizeof(sendbuf),0);
    printf("recv %s \n",sendbuf);
    closesocket(ConnectSocket);

    WSACleanup();
    return;
  }
