#include "hatchLog.h"
#include "hatchNet.h"
#include "hatchGlobal.h"
#include "hatchServices.h"

#pragma comment(lib,"Ws2_32.lib")

CHatchNet::CHatchNet()
	:m_hSocket(0)
{
	WSADATA wsaData;
	// Initialize Winsock
    WSAStartup(MAKEWORD(2,2), &wsaData);
}

CHatchNet::~CHatchNet()
{
	if(INVALID_SOCKET != m_hSocket)
	{
		closesocket(m_hSocket);
	}

	WSACleanup();
}

int CHatchNet::hatchCreateSocket(int type,int protocol)
{
	m_hSocket = socket(AF_INET,type,protocol);
	if(INVALID_SOCKET == m_hSocket)
	{
		FAI_LOG("create socket error.");
		return -1;
	}
	unsigned long dwInbuf = 0;
	unsigned long dwOutBuf[20];
	unsigned long dwResult;
	if(type == SOCK_DGRAM)
	{
		WSAIoctl(m_hSocket,SIO_UDP_CONNRESET,&dwInbuf,sizeof(dwInbuf),&dwOutBuf,sizeof(dwOutBuf),&dwResult,NULL,NULL);
	}
	return 0;
}

int CHatchNet::hatchBind(struct sockaddr *name,int namelen)
{
	int ret = 0;
	sockaddr_in *sName = (sockaddr_in*)name;
	unsigned int port = ntohs(sName->sin_port);
	do
	{
		ret = bind(m_hSocket,name,namelen);
		if(SOCKET_ERROR == ret)
		{
			unsigned long Eret = GetLastError();
			switch(Eret)
			{
			case WSAEADDRINUSE:
				if(port < 65535)
				{
					sName->sin_port = htons(++port);
					name = (sockaddr*)sName;
					continue;
				}
			default:
				FAI_LOG1("bind socket error.errorCode:%u",Eret);
				return -1;
			}
		}
		return ret;
	}while(1);
}

SOCKET CHatchNet::hatchAccept(struct sockaddr *addr,int *addrlen)
{
	SOCKET hSocket = accept(m_hSocket,addr,addrlen);
	if(INVALID_SOCKET == hSocket)
	{
		FAI_LOG("accept socket error.");
		return -1;
	}
	return hSocket;
}
	
int CHatchNet::hatchConnect(struct sockaddr *name,int namelen)
{
	int ret = connect(m_hSocket,name,namelen);
	if(SOCKET_ERROR == ret)
	{
		FAI_LOG("connect socket error.");
		return -1;
	}
	return 0;
}
	
int CHatchNet::hatchListen(int backlog)
{
	int ret = listen(m_hSocket,backlog);
	if(SOCKET_ERROR == ret)
	{
		FAI_LOG1("listen socket error.errorCode:%d",GetLastError());
		return -1;
	}
	return 0;
}
	
int CHatchNet::hatchRecv(char *buf,int len,int flags)
{
	int ret = recv(m_hSocket,buf,len,flags);
	if(SOCKET_ERROR == ret)
	{
		FAI_LOG1("recv socket error.errorcode:%d.",GetLastError());
		return -1;
	}
	return ret;
}

int CHatchNet::hatchSend(char *buf,int len,int flags)
{
	int ret = send(m_hSocket,buf,len,flags);
	if(SOCKET_ERROR == ret)
	{
		FAI_LOG1("send socket error.errorcode:%d.",GetLastError());
		return -1;
	}
	return ret;
}
	
int CHatchNet::hatchRecvFrom(char *buf,int len,int flags,struct sockaddr *from,int *fromlen)
{
	int ret = recvfrom(m_hSocket,buf,len,flags,from,fromlen);
	if(SOCKET_ERROR == ret)
	{
		FAI_LOG1("recvfrom socket error.errorcode:%d.",GetLastError());
		return -1;
	}
	return ret;
}
	
int CHatchNet::hatchSendTo(char *buf,int len,int flags,struct sockaddr *to,int tolen)
{
	int ret = sendto(m_hSocket,buf,len,flags,to,tolen);
	if(SOCKET_ERROR == ret)
	{
		FAI_LOG1("sendto socket error.errorcode:%d.",GetLastError());
		return -1;
	}
	return ret;
}

int CHatchNet::hatchGetHostName(char *name,int namelen)
{
	int ret = ::gethostname(name,namelen);
	return ret;
}

struct hostent* CHatchNet::hatchGetHostByAddr(char *addr,int len,int type)
{
	struct hostent* ret = ::gethostbyaddr(addr,len,type);
	return ret;
}
	
struct hostent* CHatchNet::hatchGetHostByName(char *name)
{
	struct hostent* ret = ::gethostbyname(name);
	return ret;
}

int CHatchNet::hatchSelectRecv()
{
	int ret = 0;
	fd_set readfd;
	FD_ZERO(&readfd);
	FD_SET(m_hSocket,&readfd);
	timeval readinterval;
	readinterval.tv_sec = READ_INTERVAL_TIME;
	readinterval.tv_usec = 0;

	ret = ::select(1,&readfd,NULL,NULL,&readinterval);

	return ret;
}


///////////////////////////////////////////////////////////////////////////////
// CHatchNetLayer class implement
//////////////////////////////////////////////////////////////////////////////

CHatchNetLayer::CHatchNetLayer()
{
	//初始化消息缓冲区
}

CHatchNetLayer::~CHatchNetLayer()
{

}

int CHatchNetLayer::hatchPacketRecvProc()
{
	char msgBuf[MSGPACKET_MAX_LENGTH] = {'\0'};
	HANDLE hService = g_eventRnnerEnd.hatchGetHandle();
	PMsgSt pMsg = NULL;
	int ret = -1,ret1 = 0,fromlen;
	sockaddr_in fromaddr;
	fromlen = sizeof(fromaddr);
	unsigned int iMsgLength = sizeof(MsgSt);
	unsigned int iOffset = (unsigned int)((unsigned long)&(((MsgSt*)0)->m_protocolType));

	while(1)
	{
		ret = WaitForSingleObject(hService,0);
		if(ret == WAIT_OBJECT_0)
		{
			break;
		}

		ret = m_pNetServices->hatchSelectRecv();
		switch(ret)
		{
		case -1://socket error
			FAI_LOG1("select socket error.errorcode:%d.",GetLastError());
			return ret;
		case 0: //time reached
			break;
		default:
			ret = m_pNetServices->hatchRecvFrom(msgBuf,MSGPACKET_MAX_LENGTH,0,(sockaddr*)&fromaddr,&fromlen);
			if(ret <= 0)
			{
				continue;
			}

			pMsg = (PMsgSt)new char[ret+iOffset];
			if(pMsg == NULL)
			{
				FAI_LOG("no memory");
				return -1;
			}

			memset(pMsg,0,ret + iOffset);
			pMsg->m_iSize = ret + iOffset;
			pMsg->m_msgFromAddr = fromaddr;

			memcpy_s(&(pMsg->m_protocolType),ret + iOffset,msgBuf,ret);			
			ret1 = CHatchMsgList::hatchPushReadMsgList(pMsg);
			if(ret1 < 0)
			{
				//recv list is full.
				delete[] pMsg;
				pMsg = NULL;
			}
			break;
		}
	}
	return 0;
}
	
int CHatchNetLayer::hatchPacketSendProc()
{
	HANDLE hService[3];
	hService[0] = g_msgSendsemaphore.hatchGetHandle();
	hService[1] = g_eventRnnerEnd.hatchGetHandle();
	PMsgSt msgBuf = NULL;
	int ret = 0;
	unsigned int iOffset = (unsigned int)((unsigned long)&(((MsgSt*)0)->m_protocolType));

	while(1)
	{
		ret = WaitForMultipleObjects(2,hService,FALSE,INFINITE);
		switch(ret)
		{
		case WAIT_OBJECT_0:
			msgBuf = CHatchMsgList::hatchPopSendMsgList();
			if(msgBuf == NULL)
			{
				continue;
			}
			m_pNetServices->hatchSendTo((char *)&(msgBuf->m_protocolType),msgBuf->m_iSize - iOffset,0,(sockaddr*)&(msgBuf->m_msgFromAddr),sizeof(msgBuf->m_msgFromAddr));
			delete[] msgBuf;
			msgBuf = NULL;
			break;
		case WAIT_OBJECT_0+1://endevent
			return 0;
		default:
			FAI_LOG1("hatchPacketSendProc error:%u",GetLastError());
			break;
		}
	}

	return ret;
}

int CHatchNetLayer::hatchPacketTempProc()
{
	return 0;
}

int CHatchNetLayer::hatchAttachNetSerivces(CHatchNet &net)
{
	m_pNetServices = &net;
	return 0;
}
	
int CHatchNetLayer::hatchDetachNetServices(CHatchNet &net)
{
	m_pNetServices = NULL;
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// CHatchNetManager class implement
//////////////////////////////////////////////////////////////////////////////


CHatchNetManager::CHatchNetManager()
{
}

CHatchNetManager::~CHatchNetManager()
{
}

int CHatchNetManager::hatchCreateNetServices(FrmInfo *frm)
{
	int ret = 0;

	if(NULL == frm)
	{
		FAI_LOG("frm is null");
		return -1;
	}

	//创建socket，并绑定到指定端口
	frm->m_baseNet = new CHatchNet();
	if(NULL == frm->m_baseNet)
	{
		FAI_LOG("create base net failure.");
		return -1;
	}

	ret = frm->m_baseNet->hatchCreateSocket(SOCK_DGRAM,IPPROTO_UDP);
	if(ret < 0)
	{
		FAI_LOG("create socket failure.");
		return ret;
	}

	frm->m_ownerAddress.sin_family = AF_INET;
	//frm->m_ownerAddress.sin_port = htons(frm->m_ownerAddress.sin_port);
	frm->m_ownerAddress.sin_addr.S_un.S_addr = INADDR_ANY;
	ret = frm->m_baseNet->hatchBind(reinterpret_cast<sockaddr*>(&(frm->m_ownerAddress)),sizeof(sockaddr));
	if(ret < 0)
	{ 
		FAI_LOG("bind socket failure.");
		return ret;
	}

	//指定真正地址
#if SERVICES_TYPE == 1
	char hname[128];
	hostent *thost = NULL;
	frm->m_baseNet->hatchGetHostName(hname,128);
	thost = frm->m_baseNet->hatchGetHostByName(hname);
	if(thost != NULL)
	{
		frm->m_ownerAddress.sin_addr.S_un.S_addr = *(unsigned long *)(thost->h_addr_list[0]);
	}
	else
	{
		ERR_LOG1("Local ip get error. errCode:%d",GetLastError());
	}
#endif

	//绑定网络层
	frm->m_netLayer = new CHatchNetLayer();
	if(NULL == frm->m_netLayer)
	{
		FAI_LOG("create net layer failure.");
		return -1;
	}

	ret = frm->m_netLayer->hatchAttachNetSerivces(*(frm->m_baseNet));
	if(ret < 0)
	{
		FAI_LOG("Attach net layer failure.");
		return -1;
	}
	 
	return ret;
}

int CHatchNetManager::hatchAttachRunner(FrmInfo *frm)
{
	if((NULL == frm) || (NULL == frm->m_netLayer) || (NULL == frm->m_runner))
	{
		FAI_LOG("attach runner failure.");
		return -1;
	}

	int ret = frm->m_runner->hatchAttach(frm->m_netLayer);

	return ret;
}

int CHatchNetManager::hatchDettachRunner(FrmInfo *frm)
{
	if((NULL == frm) || (NULL == frm->m_netLayer) || (NULL == frm->m_runner))
	{
		FAI_LOG("attach runner failure.");
		return -1;
	}

	int ret = frm->m_runner->hatchDetach(frm->m_netLayer);

	return ret;
}

int CHatchNetManager::hatchFactory(int port,char *ip,char *sname,CHatchRunner *runner)
{
	FrmInfo *tFrm = new FrmInfo;
	if(NULL == tFrm)
	{
		FAI_LOG("there aren't enough memory.");
		return -1;
	}

	//initialize structure
	memset(tFrm,0,sizeof(FrmInfo));
	tFrm->m_iSize = sizeof(FrmInfo);
	tFrm->m_ownerAddress.sin_port = htons(port);
	tFrm->m_ownerAddress.sin_family = AF_INET;

	if(NULL == sname)
	{
		strcpy_s(tFrm->m_cServiceName,16,"default");
	}
	else
	{
		strcpy_s(tFrm->m_cServiceName,16,sname);
	}

	int ret = hatchCreateNetServices(tFrm);
	if(ret < 0)
	{
		FAI_LOG("create net service failure.");
		return -1;
	}

	ret = hatchAddServiceList(tFrm);
	
	return ret;
}

int CHatchNetManager::hatchAddServiceList(FrmInfo *frm)
{
	if(NULL == frm)
	{
		FAI_LOG("add net service failure.");
		return -1;
	}

	g_frmServicesList.push_back(frm);

	return 0;
}

int CHatchNetManager::hatchDelServiceList(FrmInfo *frm)
{
	if(NULL == frm)
	{
		FAI_LOG("del net service failure.");
		return -1;
	}

	FrmInfoListIterator ite = g_frmServicesList.begin();
	FrmInfoListIterator ited = g_frmServicesList.end();
	for(;ite != ited; ite++)
	{
		if(frm == *ite)
		{
			g_frmServicesList.erase(ite);
			break;
		}
	}

	return 0;
}

int CHatchNetManager::hatchSetOwnerAddr(void)
{
	//如果是客户端，些函数有效，并将客户端用户地址更新
#if SERVICES_TYPE == 1
	UserListIterator ite = CHatchUserList::hatchLocateUser(NULL);
	(*ite)->hatchSetAddr(&(g_frmServicesList.front()->m_ownerAddress.sin_addr));
	(*ite)->hatchSetPort(g_frmServicesList.front()->m_ownerAddress.sin_port);
#endif

	return 0;
}
