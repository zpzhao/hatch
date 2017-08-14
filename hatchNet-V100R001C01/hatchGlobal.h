
#ifndef HATCHGLOBAL_H_H
#define HATCHGLOBAL_H_H

#include "hatchCommonFile.h"
#include "hatchThreadPool.h"

/******************************************************************
	消息缓存区，接收消息，发送消息，其它消息
*******************************************************************/
extern MsgList g_msgRecvList;
extern MsgList g_msgSendList;
extern MsgList g_msgTempList;

extern CHatchAutoCriticalSection g_msgRecvListCriticalSection;
extern CHatchAutoCriticalSection g_msgSendListCriticalSection;
extern CHatchAutoCriticalSection g_msgTempListCriticalSection;

extern CHatchSemaphore g_msgRecvsemaphore;		//接收消息缓冲区信号量
extern CHatchSemaphore g_msgSendsemaphore;		//发送消息缓冲区信号量
extern CHatchSemaphore g_msgTempsemaphore;		//其它消息缓冲区信号量

/*****************************************************************
	程序任务结束事件变量定义
******************************************************************/
extern CHatchEvent g_eventRnnerEnd;		//程序是否结束，如果可信则线程池中各任务结束


/*****************************************************************
	用户信息变量定义 及对应的同步变量
******************************************************************/
extern UserList g_vectUserList;
extern CHatchAutoCriticalSection g_msgUserListCriticalSection;

/*****************************************************************
	框架服务列表定义
******************************************************************/
extern FrmInfoList g_frmServicesList;




#endif