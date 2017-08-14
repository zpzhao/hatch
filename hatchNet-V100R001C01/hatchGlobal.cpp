#include "hatchGlobal.h"

/******************************************************************
	消息缓存区，接收消息，发送消息，其它消息,及对应的同步变量
*******************************************************************/
MsgList g_msgRecvList;
MsgList g_msgSendList;
MsgList g_msgTempList;

CHatchAutoCriticalSection g_msgRecvListCriticalSection;
CHatchAutoCriticalSection g_msgSendListCriticalSection;
CHatchAutoCriticalSection g_msgTempListCriticalSection;

CHatchSemaphore g_msgRecvsemaphore;		//接收消息缓冲区信号量
CHatchSemaphore g_msgSendsemaphore;		//发送消息缓冲区信号量
CHatchSemaphore g_msgTempsemaphore;		//其它消息缓冲区信号量

/*****************************************************************
	用户信息变量定义 及对应的同步变量
******************************************************************/
#include "hatchUserInfo.h"

UserList g_vectUserList;
CHatchAutoCriticalSection g_msgUserListCriticalSection;

/*****************************************************************
	程序任务结束事件变量定义
******************************************************************/
CHatchEvent g_eventRnnerEnd;			//程序是否结束，如果可信则线程池中各任务结束


/*****************************************************************
	框架服务列表定义
******************************************************************/
FrmInfoList g_frmServicesList;

