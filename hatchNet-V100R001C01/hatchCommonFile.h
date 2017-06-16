#ifndef HATCHCOMMONFILE_H_H
#define HATCHCOMMONFILE_H_H

#include <iostream>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <WinSock2.h>
#include <Mstcpip.h>

#include <vector>
#include <list>
#include <queue>

#include <stdio.h>

typedef void *(*pFun)(void *data);

/************************************************************
	�̳߳��������߳��������壬�ź������ֵ����
*************************************************************/
#define APPLICATION_THREAD_NUMBER 4
#define SEMAPHORE_MAX_COUNT 0x8ff


/*************************************************************
	��־·�������
*************************************************************/
#define TEST_MULTIPLE //log directory switch

/*************************************************************
	�û�������Ϣ����С����
*************************************************************/
#define USERNAME_MAX_LENGTH 32
#define MSGPACKET_MAX_LENGTH 1024

/*************************************************************
	����㶨�壬����IP���˿ڣ���ȡ��ʱ��
*************************************************************/
#define SERVICES_TYPE 0 //0 server ; 1 client

#define SERVER_IPADRESS "192.168.2.1"
#define SERVER_PORT_START 50681
#define CLIENT_PORT_START 20281
#define READ_INTERVAL_TIME 5 //5sec interval

#if SERVICES_TYPE == 0
#define SERVICES_PORT SERVER_PORT_START
#else if SERVICES_TYPE == 1
#define SERVICES_PORT SERVER_PORT_START
#endif

/*************************************************************
	����UDP����ʱ���Զ˹رպ��յ����󱨸�
*************************************************************/
#define SIO_UDP_CONNRESET     _WSAIOW(IOC_VENDOR,12) 

/*************************************************************
	�û��б����Ͷ���
*************************************************************/
class CHatchUserInfo;
typedef std::vector<CHatchUserInfo *> UserList,*PUserList;
typedef UserList::iterator UserListIterator;


#define NET_READ_MESSAGE_LIST_MAX_LENGTH 0x100
#define NET_SEND_MESSAGE_LIST_MAX_LENGTH 0x100
#define NET_TEMP_MESSAGE_LIST_MAX_LENGTH 0x100


extern const char const_RunLogPath[];
extern const char const_DevLogPath[];
extern const char const_MemLogPath[];



//structure define 
enum UserStatus
{
	User_Login	= 0x01,
	User_Online	,
	User_Hide	,
	User_Busy	,
	User_NULL
};

enum ProtocolType
{
	PTL_LMT		=	0X01,
	PTL_CHAT	=	0X02,
	PTL_NULL	=	0X00
};

enum MsgType
{
	Login_type	=	0x01,
	Update_type	,
	Status_type	,
	Text_type	,
	Logout_type	,
	BackSyn_type,	//synchronize old user to new user
	NULL_type
};

enum MsgListType
{
	MsgList_Recv	=	0x01,
	MsgList_Send	,
	MsgList_Temp	,
	MsgList_NULL
};

#pragma pack (push,1)

/*************************************************************
	LMT��Ϣ�ṹ����
*************************************************************/

struct MsgLMTData
{
	unsigned long m_iLength;
	unsigned int m_cmdCode;
	int m_retCode;
	char * m_cData[1];
};

/*************************************************************
	������Ϣ�ṹ����
*************************************************************/
struct MsgContext
{
	unsigned int m_msgSize;
	MsgType m_msgType;
	char m_msgUserName[USERNAME_MAX_LENGTH];
	char m_msgText[1];
};

//server send update when a user login successful
typedef struct MsgContextUpdateDown
{
	unsigned int m_msgSize;
	MsgType m_msgType;
	char m_msgUserName[USERNAME_MAX_LENGTH];
	sockaddr_in m_msgUserAddr;
}*PMsgContextUpdateDown,MsgContextUpdateUp,*PMsgContextUpdateUp;

typedef struct MsgAddr
{
	unsigned int m_msgSize;
	MsgContext m_msgContext;
}*PMsgAddr;

/*************************************************************
	���紫����Ϣ�ṹ����
*************************************************************/
typedef struct MsgSt
{
	unsigned int m_iSize;
	sockaddr_in m_msgFromAddr;
	ProtocolType m_protocolType;
	union
	{
		MsgAddr m_chMsg;
		MsgLMTData m_lmtMsg;
	}m_msgContext;
}*PMsgSt;
typedef std::queue<MsgSt *> MsgList,*PMsgList;


/*************************************************************
	��ܷ���ṹ���壺���磬���񣬷�����
*************************************************************/
class CHatchNetLayer;
class CHatchNet;
class CHatchRunner;

struct FrmInfo
{
	unsigned int m_iSize;
	CHatchNet *m_baseNet;
	CHatchNetLayer *m_netLayer;
	CHatchRunner *m_runner;
	sockaddr_in	m_ownerAddress;
	char m_cServiceName[16];
};
typedef std::list<FrmInfo *> FrmInfoList,*PFrmInfoList;
typedef std::list<FrmInfo *>::iterator FrmInfoListIterator;



#pragma pack(pop)



#endif