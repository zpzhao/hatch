#include "hatchProtocol.h"
#include "hatchCommonFile.h"
#include "hatchServices.h"
#include "hatchLog.h"
#include "hatchGlobal.h"


CHatchProtocol::CHatchProtocol()
{
	m_pServices = NULL;
}
	
CHatchProtocol::CHatchProtocol(CHatchServices *pService)
{
	m_pServices = pService;
}

CHatchProtocol::~CHatchProtocol()
{
	m_pServices = NULL;
}

int CHatchProtocol::hatchMsgProc(char *buf)
{
	int ret = 0;
	MsgSt* pMsg = (MsgSt *)buf;
	switch(pMsg->m_protocolType)
	{
	case PTL_CHAT:
	{
		switch(pMsg->m_msgContext.m_chMsg.m_msgContext.m_msgType)
		{
		case Login_type:
			ret = hatchMsgLogin(buf);
			break;
		case Update_type:
			ret = hatchMsgUpdate(buf);
			break;
		case Status_type:
			break;
		case Text_type:
			ret = hatchMsgText(buf);
			break;
		case BackSyn_type:
			ret = hatchMsgBackSyn(buf);
			break;
		case Logout_type:
			ret = hatchMsgLogout(buf);
			break;
		default:
			FAI_LOG("message type error.");
			return -1;
		}
	}
		break;
	case PTL_LMT:
		break;
	default:
		ERR_LOG("protocol type error.");
		ret = -1;
		break;
	}

	return ret;
}
	
int CHatchProtocol::hatchMsgLogin(char *buf)
{
	return 0;
}
	
int CHatchProtocol::hatchMsgText(char *buf)
{
	return 0;
}

int CHatchProtocol::hatchMsgUpdate(char *buf)
{
	return 0;
}

int CHatchProtocol::hatchMsgBackSyn(char *buf)
{
	return 0;
}

int CHatchProtocol::hatchMsgLogout(char *buf)
{
	return 0;
}
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

CHatchProtocolServer::CHatchProtocolServer()
	:CHatchProtocol()
{
}
	
CHatchProtocolServer::CHatchProtocolServer(CHatchServices *pService)
	:CHatchProtocol(pService)
{
}

CHatchProtocolServer::~CHatchProtocolServer()
{
}

int CHatchProtocolServer::hatchMsgLogin(char *buf)
{
	MsgSt *pMsg = (MsgSt *)buf;
	MsgAddr *pchMsg = &(pMsg->m_msgContext.m_chMsg);
	int ret = 0;
	MsgContextUpdateDown updateDown;

	updateDown.m_msgSize = sizeof(MsgContextUpdateDown);
	updateDown.m_msgType = Update_type;
	updateDown.m_msgUserAddr = pMsg->m_msgFromAddr;
	strcpy_s(updateDown.m_msgUserName,USERNAME_MAX_LENGTH,pchMsg->m_msgContext.m_msgUserName);

	//add user
	CHatchUserInfo *pUser = new CHatchUserInfo();
	if(pUser == NULL)
	{
		FAI_LOG("memory isn't enough.");
		return -1;
	}
	pUser->hatchSetAddr(&(pMsg->m_msgFromAddr.sin_addr));
	//pUser->hatchSetIndex(0);
	pUser->hatchSetPort(pMsg->m_msgFromAddr.sin_port);
	pUser->hatchSetUserName(pchMsg->m_msgContext.m_msgUserName);
	ret = CHatchUserList::hatchPushUser(pUser);
	if(ret < 0)
	{
		delete pUser;
		pUser = NULL;
		strcpy_s(pchMsg->m_msgContext.m_msgUserName,USERNAME_MAX_LENGTH,"error");
		CHatchMsgList::hatchPushSendMsgList(pMsg); //I'm not cared successful or not.
		return -1;
	}

	//add response message
	strcpy_s(pchMsg->m_msgContext.m_msgUserName,USERNAME_MAX_LENGTH,"server");
	ret = CHatchMsgList::hatchPushSendMsgList(pMsg);
	if(ret < 0)
	{//send message list is full.
		CHatchUserList::hatchPopUser(updateDown.m_msgUserName,0);
		delete pUser;
		pUser = NULL;
		delete[] pMsg;
		pMsg = NULL;
		ERR_LOG("send message list is full.");
		return ret;
	}

	//send update to all users in userlist
	char *pMsgBuf=NULL;
	PMsgSt pUpdateMsg=NULL;
	int Offset,Total,iOffset;
	iOffset = (int)(unsigned long)&(((PMsgSt)0)->m_msgContext);
	Offset = sizeof(MsgAddr) - sizeof(MsgContext) + iOffset;
	Total = sizeof(MsgContextUpdateDown) + Offset;

	pMsgBuf = new char[Total];
	if(pMsgBuf == NULL)
	{
		FAI_LOG("no memory.");
		return -1;
	}
	memset(pMsgBuf,0,Total);
	pUpdateMsg = (PMsgSt)pMsgBuf;

	pUpdateMsg->m_iSize = Total;
	pUpdateMsg->m_protocolType = PTL_CHAT;
	pUpdateMsg->m_msgFromAddr = pMsg->m_msgFromAddr;

	memcpy_s(pMsgBuf+Offset,Total-Offset,&updateDown,sizeof(MsgContextUpdateDown));

	ret = hatchMsgUpdateDown(pMsgBuf);
	return ret;
}

int CHatchProtocolServer::hatchMsgText(char *buf)
{
	return 0;
}

int CHatchProtocolServer::hatchMsgUpdateDown(char *buf)
{
	int ret = CHatchMsgList::hatchPushSendMsgGroup((MsgSt*)buf);
	return ret;
}

int CHatchProtocolServer::hatchMsgUpdate(char *buf)
{
	//check user
	MsgSt *pMsg = (MsgSt *)buf;
	int ret = 0;

	ret = CHatchUserList::hatchIsUserInList(pMsg->m_msgContext.m_chMsg.m_msgContext.m_msgUserName);
	if(!ret)
	{//error
		strcpy_s(pMsg->m_msgContext.m_chMsg.m_msgContext.m_msgUserName,USERNAME_MAX_LENGTH,"error");
	}

	ret = CHatchMsgList::hatchPushSendMsgList(pMsg);

	return 0;
}

int CHatchProtocolServer::hatchMsgBackSyn(char *buf)
{
	return 0;
}

int CHatchProtocolServer::hatchMsgLogout(char *buf)
{
	//check user
	MsgSt *pMsg = (MsgSt *)buf;
	int ret = 0;

	ret = CHatchUserList::hatchIsUserInList(pMsg->m_msgContext.m_chMsg.m_msgContext.m_msgUserName);
	if(!ret)
	{
		delete[] pMsg;
		pMsg = NULL;
		return 0;
	}

	//pop user
	CHatchUserInfo *pUser = CHatchUserList::hatchPopUser(pMsg->m_msgContext.m_chMsg.m_msgContext.m_msgUserName,0);
	if(pUser != NULL)
	{
		delete pUser;
		pUser = NULL;
	}

	//send logout to the user deleted
	PMsgSt pTempAddr = (PMsgSt)new char[pMsg->m_iSize];
	if(pTempAddr == NULL)
	{
		FAI_LOG("no memory");
		delete[] pMsg;
		pMsg = NULL;
		return -1;
	}
	memcpy_s(pTempAddr,pMsg->m_iSize,pMsg,pMsg->m_iSize);
	ret = CHatchMsgList::hatchPushSendMsgList(pTempAddr);
	if(ret < 0)
	{
		FAI_LOG("send list is full.");
		delete[] pTempAddr;
		pTempAddr = NULL;
	}

	ret = CHatchMsgList::hatchPushSendMsgGroup(pMsg);	
	return ret;
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

CHatchProtocolClient::CHatchProtocolClient()
	:CHatchProtocol()
{
}
	
CHatchProtocolClient::CHatchProtocolClient(CHatchServices *pService)
	:CHatchProtocol(pService)
{
}

CHatchProtocolClient::~CHatchProtocolClient()
{
}

int CHatchProtocolClient::hatchMsgLogin(char *buf)
{
	PMsgSt pMsg = (PMsgSt)buf;
	int ret = 0;
	UserListIterator ite = CHatchUserList::hatchLocateUser(NULL);
	if(strcmp("server",pMsg->m_msgContext.m_chMsg.m_msgContext.m_msgUserName) == 0)
	{
		(*ite)->hatchSetStatus(User_Login);
		PRO_LOG("login successful.");
		std::cout<<"login successful."<<std::endl;
		std::cout<<"Start to chat below:"<<std::endl;
		m_pServices->hatchStartText();
	}
	else
	{
		(*ite)->hatchSetStatus(User_NULL);
		PRO_LOG("login unsuccessful.");
		std::cout<<"login unsuccessful."<<std::endl;
	}
	return 0;
}
	
int CHatchProtocolClient::hatchMsgText(char *buf)
{
	//check user 
	//(1) user in list and online
	//(2) user in list and not online
	//(3) user isn't exist in list
	//(4) user name is owner,push it with group send,message type Text_type
	MsgSt *pMsg = (MsgSt *)buf;
	int ret = 0;
	char sOwnerName[USERNAME_MAX_LENGTH];
	UserListIterator ite = CHatchUserList::hatchLocateUser(NULL);
	(*ite)->hatchGetUserName(sOwnerName,USERNAME_MAX_LENGTH);

	ret = CHatchUserList::hatchIsUserInList(pMsg->m_msgContext.m_chMsg.m_msgContext.m_msgUserName);
	if(!ret)
	{//(3)
		ret = hatchMsgUpdateUp(buf);
		return ret;
	}
	
	ite = CHatchUserList::hatchLocateUser(pMsg->m_msgContext.m_chMsg.m_msgContext.m_msgUserName);
	if(strcmp(pMsg->m_msgContext.m_chMsg.m_msgContext.m_msgUserName,sOwnerName) == 0)
	{//(4)
		ret = CHatchMsgList::hatchPushSendMsgGroup(pMsg);
		return ret;
	}

	UserStatus userStatus = (*ite)->hatchGetStatus();
	switch(userStatus)
	{
	case User_Hide:
		//(2)
		(*ite)->hatchSetStatus(User_Online);
	case User_Online:
	case User_Busy:
		//(1)
		std::cout<<pMsg->m_msgContext.m_chMsg.m_msgContext.m_msgUserName<<":"<<std::endl;
		std::cout<<pMsg->m_msgContext.m_chMsg.m_msgContext.m_msgText<<std::endl;
		break;
	default:
		FAI_LOG1("Status is error,userName:%s",pMsg->m_msgContext.m_chMsg.m_msgContext.m_msgUserName);
		break;
	}	
	delete[] pMsg;

	return ret;
}

int CHatchProtocolClient::hatchMsgUpdateUp(char *buf)
{
	PMsgSt pMsg = (PMsgSt)buf;
	int ret = 0;

	MsgContextUpdateUp updateUp;
	updateUp.m_msgSize = sizeof(MsgContextUpdateUp);
	updateUp.m_msgType = Update_type;
	updateUp.m_msgUserAddr = pMsg->m_msgFromAddr;
	strcpy_s(updateUp.m_msgUserName,USERNAME_MAX_LENGTH,pMsg->m_msgContext.m_chMsg.m_msgContext.m_msgUserName);

	//send update to all users in userlist
	char *pMsgBuf=NULL;
	PMsgSt pUpdateMsg=NULL;
	int Offset,Total;
	Offset = (int)(unsigned long)&(((PMsgSt)0)->m_msgContext);
	Total = sizeof(MsgContextUpdateUp) + Offset;

	pMsgBuf = new char[Total];
	if(pMsgBuf == NULL)
	{
		FAI_LOG("no memory.");
		delete[] pMsg;
		return -1;
	}
	memset(pMsgBuf,0,Total);

	UserListIterator ite = CHatchUserList::hatchLocateUser("server");
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr = (*ite)->hatchGetAddr();
	serverAddr.sin_port = (*ite)->hatchGetPort();

	pUpdateMsg = (PMsgSt)pMsgBuf;
	pUpdateMsg->m_iSize = Total;
	pUpdateMsg->m_msgFromAddr = serverAddr;
	pUpdateMsg->m_protocolType = PTL_CHAT;
	memcpy_s(&(pUpdateMsg->m_msgContext),Total-Offset,&updateUp,updateUp.m_msgSize);
	delete[] pMsg;

	ret = CHatchMsgList::hatchPushSendMsgList(pUpdateMsg);
	return ret;
}

int CHatchProtocolClient::hatchMsgUpdate(char *buf)
{
	MsgSt *pMsg = (MsgSt *)buf;
	int ret = 0;
	UserListIterator ite = CHatchUserList::hatchLocateUser(NULL);
	char sName[USERNAME_MAX_LENGTH];
	(*ite)->hatchGetUserName(sName,USERNAME_MAX_LENGTH);
	UserStatus OwnerStatus = (*ite)->hatchGetStatus();

	switch(OwnerStatus)
	{
	case User_Login:
		if(strcmp(sName,pMsg->m_msgContext.m_chMsg.m_msgContext.m_msgUserName) == 0)
		{
			(*ite)->hatchSetStatus(User_Online);
			PRO_LOG("Online successful.");
		}
		else
		{
			(*ite)->hatchSetStatus(User_NULL);
			PRO_LOG("Online unsuccessful.");
		}
		delete[] pMsg;
		pMsg = NULL;
		break;
	case User_Online:
	case User_Hide:
	case User_Busy:
		ret = CHatchUserList::hatchIsUserInList(pMsg->m_msgContext.m_chMsg.m_msgContext.m_msgUserName);
		if(!ret)
		{//username isn't exist in user list,add it now.
			if(strcmp(pMsg->m_msgContext.m_chMsg.m_msgContext.m_msgUserName,"error") == 0)
			{
				delete[] pMsg;
				pMsg = NULL;
				return -1;
			}

			CHatchUserInfo *pUser = new CHatchUserInfo();
			if(pUser == NULL)
			{
				FAI_LOG("memory isn't enough.");
				delete[] pMsg;
				pMsg = NULL;
				return -1;
			}
			PMsgContextUpdateDown pContext = (PMsgContextUpdateDown)&(pMsg->m_msgContext.m_chMsg.m_msgContext);
			pUser->hatchSetAddr(&(pContext->m_msgUserAddr.sin_addr));
			//pUser->hatchSetIndex(0);
			pUser->hatchSetPort(pContext->m_msgUserAddr.sin_port);
			pUser->hatchSetUserName(pContext->m_msgUserName);
			pUser->hatchSetStatus(User_Online);

			ret = CHatchUserList::hatchPushUser(pUser);
			if(ret < 0)
			{
				delete pUser;
				pUser = NULL;
				delete[] pMsg;
				pMsg = NULL;
				FAI_LOG("update other user,add user to list failure.");
				return -1;
			}

			//synchronize userinfo to new user
			ite = CHatchUserList::hatchLocateUser(NULL);
			pMsg->m_msgFromAddr = pContext->m_msgUserAddr;
			pContext->m_msgType = BackSyn_type;
			pContext->m_msgUserAddr.sin_addr = (*ite)->hatchGetAddr();
			pContext->m_msgUserAddr.sin_port = (*ite)->hatchGetPort();
			strcpy_s(pContext->m_msgUserName,USERNAME_MAX_LENGTH,sName);
			ret = CHatchMsgList::hatchPushSendMsgList(pMsg);
		}
		break;
	default:
		FAI_LOG("Don't care!");
		delete[] pMsg;
		pMsg = NULL;
		break;
	}

	return ret;
}

int CHatchProtocolClient::hatchMsgBackSyn(char *buf)
{
	//synchronize userinfo to new user
	//(1)user exist in list,no process; (2)user isn't exist,add it
	MsgSt *pMsg = (MsgSt *)buf;
	int ret = 0;

	ret = CHatchUserList::hatchIsUserInList(pMsg->m_msgContext.m_chMsg.m_msgContext.m_msgUserName);
	if(ret)
	{
		delete[] pMsg;
		return 0;
	}

	//username isn't exist in user list,add it now.
	CHatchUserInfo *pUser = new CHatchUserInfo();
	if(pUser == NULL)
	{
		FAI_LOG("memory isn't enough.");
		delete[] pMsg;
		pMsg = NULL;
		return -1;
	}
	PMsgContextUpdateDown pContext = (PMsgContextUpdateDown)&(pMsg->m_msgContext.m_chMsg.m_msgContext);
	pUser->hatchSetAddr(&(pContext->m_msgUserAddr.sin_addr));
	//pUser->hatchSetIndex(0);
	pUser->hatchSetPort(pContext->m_msgUserAddr.sin_port);
	pUser->hatchSetUserName(pContext->m_msgUserName);
	pUser->hatchSetStatus(User_Online);

	ret = CHatchUserList::hatchPushUser(pUser);
	if(ret < 0)
	{
		delete pUser;
		pUser = NULL;
		delete[] pMsg;
		pMsg = NULL;
		FAI_LOG("update other user,add user to list failure.");
		return -1;
	}

	return 0;
}

int CHatchProtocolClient::hatchMsgLogout(char *buf)
{
	//check user
	MsgSt *pMsg = (MsgSt *)buf;
	int ret = 0;

	ret = CHatchUserList::hatchIsUserInList(pMsg->m_msgContext.m_chMsg.m_msgContext.m_msgUserName);
	if(!ret)
	{
		delete[] pMsg;
		pMsg = NULL;
		return 0;
	}

	UserListIterator ite = CHatchUserList::hatchLocateUser(NULL);
	char sName[USERNAME_MAX_LENGTH];
	(*ite)->hatchGetUserName(sName,USERNAME_MAX_LENGTH);
	if(strcmp(sName,pMsg->m_msgContext.m_chMsg.m_msgContext.m_msgUserName) == 0)
	{
		(*ite)->hatchSetStatus(User_NULL);
		delete[] pMsg;
		pMsg = NULL;
		m_pServices->hatchServicesEnd();//exit
		return 0;
	}

	//other user
	CHatchUserInfo *pUser = CHatchUserList::hatchPopUser(pMsg->m_msgContext.m_chMsg.m_msgContext.m_msgUserName,0);
	if(pUser != NULL)
	{
		delete pUser;
		pUser = NULL;
	}
	
	if(pMsg != NULL)
		delete[] pMsg;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

CHatchProtocolTimer::CHatchProtocolTimer()
	:CHatchProtocol()
{
}

CHatchProtocolTimer::~CHatchProtocolTimer()
{
}


///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////