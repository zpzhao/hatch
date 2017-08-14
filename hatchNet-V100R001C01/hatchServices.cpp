#include "hatchServices.h"
#include "hatchProtocol.h"
#include "hatchGlobal.h"
#include "hatchLog.h"

CHatchServices::CHatchServices()
{
	//default type is server
	m_iServiceType = 0;
	m_pProtocol = NULL;
}

CHatchServices::~CHatchServices()
{
	if(m_pProtocol != NULL)
		delete m_pProtocol;
	//user list
	if(g_vectUserList.size() != 0)
	{
		UserListIterator ite = g_vectUserList.begin(),itend = g_vectUserList.end();
		for(;ite != itend;ite++)
		{
			if((*ite) != NULL)
				delete (*ite);
		}
		g_vectUserList.clear();
	}
}

int CHatchServices::hatchInitialize()
{
	int retCode = 0;
	
	m_eventStartText.hatchCreateEvent();

	//第一个用户是服务器信息
	CHatchUserInfo *pUser = new CHatchUserInfo();
	pUser->hatchSetIndex(0);
	in_addr servaddr;
	servaddr.S_un.S_addr = inet_addr(SERVER_IPADRESS);
	pUser->hatchSetAddr(&servaddr);
	pUser->hatchSetPort(htons(SERVER_PORT_START));
	pUser->hatchSetStatus(User_Online);
	char sName[10];
	strcpy_s(sName,10,"server");
	pUser->hatchSetUserName(sName);
	retCode = CHatchUserList::hatchPushUser(pUser);
	if(retCode < 0)
	{
		FAI_LOG("Initialize services failure,because userlist is error.");
		return retCode;
	}

	//start services
	if(m_iServiceType)
	{
		retCode = hatchInitClient();
	}
	else
	{
		retCode = hatchInitServer();
	}
	return retCode;
}
	
int CHatchServices::hatchInitServer()
{
	int ret = 0;

	//注册对应的协议
	m_pProtocol = new CHatchProtocolServer(this);
	if(m_pProtocol == NULL)
	{
		FAI_LOG("create protocol failure.");
		return -1;
	}

	return ret;
}
	
int CHatchServices::hatchInitClient()
{
	int ret = 0;

	//add own in second position of userlist.
	CHatchUserInfo *pUser = new CHatchUserInfo();
	pUser->hatchSetIndex(0);
	in_addr clientaddr;
	clientaddr.S_un.S_addr = inet_addr("127.0.0.1");
	pUser->hatchSetAddr(&clientaddr);
	pUser->hatchSetPort(htons(CLIENT_PORT_START));
	pUser->hatchSetStatus(User_Online);
	pUser->hatchSetUserName("client_init");
	ret = CHatchUserList::hatchPushUser(pUser);
	if(ret < 0)
	{
		FAI_LOG("Initialize client user failure,because userlist is error.");
		return ret;
	}

	//注册对应的协议
	m_pProtocol = new CHatchProtocolClient(this);
	if(m_pProtocol == NULL)
	{
		FAI_LOG("create protocol failure.");
		return -1;
	}
	return 0;
}

int CHatchServices::hatchGetServiceType()
{
	return m_iServiceType;
}
	
void CHatchServices::hatchSetServiceType(int type)
{
	m_iServiceType = type;
}

int CHatchServices::hatchPacketProc(void)
{
	HANDLE hService[3];
	hService[0] = g_msgRecvsemaphore.hatchGetHandle();
	hService[1] = g_msgTempsemaphore.hatchGetHandle();
	hService[2] = g_eventRnnerEnd.hatchGetHandle();

	char *msgBuf = NULL;
	while(1)
	{
		int ret = WaitForMultipleObjects(3,hService,FALSE,INFINITE);
		switch(ret)
		{
		case WAIT_OBJECT_0:
			msgBuf = (char *)CHatchMsgList::hatchPopReadMsgList();
			if(msgBuf != NULL)
				m_pProtocol->hatchMsgProc(msgBuf);
			break;
		case WAIT_OBJECT_0+1:
			msgBuf = (char *)CHatchMsgList::hatchPopTempMsgList();
			if(msgBuf != NULL)
				m_pProtocol->hatchMsgProc(msgBuf);
			break;
		case WAIT_OBJECT_0+2://endevent
			return 0;
		default:
			FAI_LOG1("packetproc error:%u",GetLastError());
			break;
		}
	}
	return 0;
}

int CHatchServices::hatchTextProc(void)
{
	int ret = 0;
	char sTextBuffer[MSGPACKET_MAX_LENGTH];
	memset(sTextBuffer,0,MSGPACKET_MAX_LENGTH);

	int MsgHeaderLength = (int)((unsigned long)&(((PMsgSt)0)->m_msgContext.m_chMsg.m_msgContext.m_msgText));
	char *pText = sTextBuffer + MsgHeaderLength;
	int TextLength = 0;

	ret = m_eventStartText.hatchWaitEvent();
	if(ret < 0)
	{
		FAI_LOG("Text event failure.");
		hatchClientLogout();
		hatchServicesEnd();
		return -1;
	}

	char sOwnerName[USERNAME_MAX_LENGTH];
	UserListIterator ite = CHatchUserList::hatchLocateUser(NULL);
	(*ite)->hatchGetUserName(sOwnerName,USERNAME_MAX_LENGTH);

	MsgSt *pMsg = (PMsgSt)sTextBuffer;
	strcpy_s(pMsg->m_msgContext.m_chMsg.m_msgContext.m_msgUserName,USERNAME_MAX_LENGTH,sOwnerName);
	pMsg->m_msgFromAddr.sin_family = AF_INET;
	pMsg->m_msgContext.m_chMsg.m_msgContext.m_msgType = Text_type;
	pMsg->m_protocolType = PTL_CHAT;

	MsgSt *pTempMsg = NULL;
	while(1)
	{
		//std::cin.getline(pText,MSGPACKET_MAX_LENGTH-MsgHeaderLength-1);
		std::cin>>pText;
		if(strcmp(pText,"close") == 0)
		{
			PRO_LOG("client will close.");
			hatchClientLogout();
			hatchServicesEnd();
			break;
		}

		TextLength = strlen(pText) ;
		pMsg->m_iSize = TextLength + MsgHeaderLength + 1;
		pMsg->m_msgContext.m_chMsg.m_msgContext.m_msgSize = sizeof(MsgContext) + TextLength;

		pTempMsg = (MsgSt *)new char[pMsg->m_iSize];
		if(pTempMsg == NULL)
		{
			FAI_LOG("no memory.");
			return -1;
		}
		memcpy_s(pTempMsg,pMsg->m_iSize,pMsg,pMsg->m_iSize);
		ret = CHatchMsgList::hatchPushTempMsgList(pTempMsg);
		if(ret < 0)
		{
			std::cout<<"Send message failured,please send again."<<std::endl;
			delete[] pTempMsg;
			pTempMsg = NULL;
		}
	}

	return ret;
}

void CHatchServices::hatchServicesEnd()
{
	g_eventRnnerEnd.hatchSetEvent();
}

void CHatchServices::hatchServicesWait()
{
	g_eventRnnerEnd.hatchWaitEvent();
}

void CHatchServices::hatchStartText()
{
	m_eventStartText.hatchSetEvent();
}
	
void CHatchServices::hatchEndText()
{
	m_eventStartText.hatchResetEvent();
}

int CHatchServices::hatchClientLogin(char *name)
{
	int ret = 0;
	sockaddr_in SenderAddr;
    int SenderAddrSize = sizeof(SenderAddr);

	SenderAddr.sin_family = AF_INET;
	SenderAddr.sin_port = htons(SERVER_PORT_START);
	SenderAddr.sin_addr.s_addr = inet_addr(SERVER_IPADRESS);

	UserListIterator ite = CHatchUserList::hatchLocateUser(NULL);
	char sName[USERNAME_MAX_LENGTH];
	if(name == NULL)
	{
		(*ite)->hatchGetUserName(sName,USERNAME_MAX_LENGTH);
	}
	else
	{
		(*ite)->hatchSetUserName(name);
		strcpy_s(sName,USERNAME_MAX_LENGTH,name);
	}

	MsgSt *msgAddr = (PMsgSt)new char[sizeof(MsgSt)];
	if(msgAddr == NULL)
	{
		FAI_LOG1("no memory.errorCode:%d",GetLastError());
		return -1;
	}

	MsgContext *msg = &(msgAddr->m_msgContext.m_chMsg.m_msgContext);
	msg->m_msgSize = sizeof(MsgContext);
	msg->m_msgType = Login_type;
	strcpy_s(msg->m_msgUserName,USERNAME_MAX_LENGTH,sName);
	msg->m_msgText[0] = '\0';

	msgAddr->m_iSize = sizeof(MsgSt);
	msgAddr->m_msgFromAddr = SenderAddr;
	msgAddr->m_protocolType = PTL_CHAT;

	ret = CHatchMsgList::hatchPushSendMsgList(msgAddr);
	if(ret < 0)
	{
		delete[] msgAddr;
		msgAddr = NULL;
	}
	return ret;
}

int CHatchServices::hatchClientLogout(char *name)
{
	int ret = 0;
	sockaddr_in SenderAddr;
    int SenderAddrSize = sizeof(SenderAddr);

	SenderAddr.sin_family = AF_INET;
	SenderAddr.sin_port = htons(SERVER_PORT_START);
	SenderAddr.sin_addr.s_addr = inet_addr(SERVER_IPADRESS);

	UserListIterator ite = CHatchUserList::hatchLocateUser(NULL);
	char sName[USERNAME_MAX_LENGTH];
	if(name == NULL)
	{
		(*ite)->hatchGetUserName(sName,USERNAME_MAX_LENGTH);
	}
	else
	{
		(*ite)->hatchSetUserName(name);
		strcpy_s(sName,USERNAME_MAX_LENGTH,name);
	}

	MsgSt *msgAddr = (PMsgSt)new char[sizeof(MsgSt)];
	if(msgAddr == NULL)
	{
		FAI_LOG1("no memory.errorCode:%d",GetLastError());
		return -1;
	}

	MsgContext *msg = &(msgAddr->m_msgContext.m_chMsg.m_msgContext);
	msg->m_msgSize = sizeof(MsgContext);
	msg->m_msgType = Logout_type;
	strcpy_s(msg->m_msgUserName,USERNAME_MAX_LENGTH,sName);
	msg->m_msgText[0] = '\0';

	msgAddr->m_iSize = sizeof(MsgSt);
	msgAddr->m_msgFromAddr = SenderAddr;
	msgAddr->m_protocolType = PTL_CHAT;

	ret = CHatchMsgList::hatchPushSendMsgList(msgAddr);
	if(ret < 0)
	{
		delete[] msgAddr;
		msgAddr = NULL;
	}
	return ret;
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
CHatchMsgList::CHatchMsgList()
{
	//NULL
}

int CHatchMsgList::hatchPushReadMsgList(PMsgSt msg)
{
	int ret = 0;
	g_msgRecvListCriticalSection.hatchEnter();
	ret = g_msgRecvsemaphore.hatchReleaseSemaphore(1);
	if(ret == 0)
		g_msgRecvList.push(msg);	
	g_msgRecvListCriticalSection.hatchLeave();
	return ret;
}

int CHatchMsgList::hatchPushSendMsgList(PMsgSt msg)
{
	int ret = 0;
	g_msgSendListCriticalSection.hatchEnter();
	ret = g_msgSendsemaphore.hatchReleaseSemaphore(1);
	if(ret == 0)
		g_msgSendList.push(msg);		
	g_msgSendListCriticalSection.hatchLeave();
	return ret;
}

int CHatchMsgList::hatchPushTempMsgList(PMsgSt msg)
{
	int ret = 0;
	g_msgTempListCriticalSection.hatchEnter();
	ret = g_msgTempsemaphore.hatchReleaseSemaphore(1);
	if(ret == 0)
		g_msgTempList.push(msg);		
	g_msgTempListCriticalSection.hatchLeave();
	return ret;
}

PMsgSt CHatchMsgList::hatchPopReadMsgList()
{
	PMsgSt tempMsg = NULL;
	g_msgRecvListCriticalSection.hatchEnter();
	tempMsg = g_msgRecvList.front();
	g_msgRecvList.pop();
	g_msgRecvListCriticalSection.hatchLeave();
	return tempMsg;
}

PMsgSt CHatchMsgList::hatchPopSendMsgList()
{
	PMsgSt tempMsg = NULL;
	g_msgSendListCriticalSection.hatchEnter();
	tempMsg = g_msgSendList.front();
	g_msgSendList.pop();
	g_msgSendListCriticalSection.hatchLeave();
	return tempMsg;
}

PMsgSt CHatchMsgList::hatchPopTempMsgList()
{
	PMsgSt tempMsg = NULL;
	g_msgTempListCriticalSection.hatchEnter();
	tempMsg = g_msgTempList.front();
	g_msgTempList.pop();
	g_msgTempListCriticalSection.hatchLeave();
	return tempMsg;
}

int CHatchMsgList::hatchPushSendMsgGroup(MsgSt *msg)
{
	int ret = 0;
	PMsgSt msgTemp = NULL;

	g_msgUserListCriticalSection.hatchEnter();
	UserListIterator ite = g_vectUserList.begin(),itend = g_vectUserList.end();	
	ite = ite + 1 + SERVICES_TYPE;

	for(;ite != itend;ite++)
	{
		msgTemp = (PMsgSt)new char[msg->m_iSize];
		if(NULL == msgTemp)
		{
			FAI_LOG("no memory");
			delete[] msg;
			msg = NULL;
			g_msgUserListCriticalSection.hatchLeave();
			return -1;
		}
		memcpy_s(msgTemp,msg->m_iSize,msg,msg->m_iSize);
		msgTemp->m_msgFromAddr.sin_addr = (*ite)->hatchGetAddr();
		msgTemp->m_msgFromAddr.sin_port = (*ite)->hatchGetPort();
		msgTemp->m_msgFromAddr.sin_family = AF_INET;

		g_msgSendListCriticalSection.hatchEnter();
		ret = g_msgSendsemaphore.hatchReleaseSemaphore(1);
		if(ret == 0)
			g_msgSendList.push(msgTemp);
		else
		{
			delete[] msgTemp;
			msgTemp = NULL;
		}
		g_msgSendListCriticalSection.hatchLeave();
	}
	delete[] msg;
	msg = NULL;

	g_msgUserListCriticalSection.hatchLeave();
	return 0;
}

int CHatchMsgList::hatchPushReadMsgGroup(MsgSt *msg)
{
	int ret = 0;
	
	return 0;
}

int CHatchMsgList::hatchPushTempMsgGroup(MsgSt *msg)
{
	int ret = 0;
	PMsgSt msgTemp = NULL;

	g_msgUserListCriticalSection.hatchEnter();
	UserListIterator ite = g_vectUserList.begin(),itend = g_vectUserList.end();	
	ite = ite + 1 + SERVICES_TYPE;

	for(;ite != itend;ite++)
	{
		msgTemp = (PMsgSt)new char[msg->m_iSize];
		if(NULL == msgTemp)
		{
			FAI_LOG("no memory");
			delete[] msg;
			msg = NULL;
			g_msgUserListCriticalSection.hatchLeave();
			return -1;
		}
		memcpy_s(msgTemp,msg->m_iSize,msg,msg->m_iSize);
		msgTemp->m_msgFromAddr.sin_addr = (*ite)->hatchGetAddr();
		msgTemp->m_msgFromAddr.sin_port = (*ite)->hatchGetPort();
		msgTemp->m_msgFromAddr.sin_family = AF_INET;

		g_msgTempListCriticalSection.hatchEnter();
		ret = g_msgTempsemaphore.hatchReleaseSemaphore(1);
		if(ret == 0)
			g_msgTempList.push(msgTemp);
		else
		{
			delete[] msgTemp;
			msgTemp = NULL;
		}
		g_msgTempListCriticalSection.hatchLeave();
	}
	delete[] msg;
	msg = NULL;

	g_msgUserListCriticalSection.hatchLeave();
	return 0;
}