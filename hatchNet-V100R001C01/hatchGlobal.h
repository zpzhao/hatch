
#ifndef HATCHGLOBAL_H_H
#define HATCHGLOBAL_H_H

#include "hatchCommonFile.h"
#include "hatchThreadPool.h"

/******************************************************************
	��Ϣ��������������Ϣ��������Ϣ��������Ϣ
*******************************************************************/
extern MsgList g_msgRecvList;
extern MsgList g_msgSendList;
extern MsgList g_msgTempList;

extern CHatchAutoCriticalSection g_msgRecvListCriticalSection;
extern CHatchAutoCriticalSection g_msgSendListCriticalSection;
extern CHatchAutoCriticalSection g_msgTempListCriticalSection;

extern CHatchSemaphore g_msgRecvsemaphore;		//������Ϣ�������ź���
extern CHatchSemaphore g_msgSendsemaphore;		//������Ϣ�������ź���
extern CHatchSemaphore g_msgTempsemaphore;		//������Ϣ�������ź���

/*****************************************************************
	������������¼���������
******************************************************************/
extern CHatchEvent g_eventRnnerEnd;		//�����Ƿ����������������̳߳��и��������


/*****************************************************************
	�û���Ϣ�������� ����Ӧ��ͬ������
******************************************************************/
extern UserList g_vectUserList;
extern CHatchAutoCriticalSection g_msgUserListCriticalSection;

/*****************************************************************
	��ܷ����б���
******************************************************************/
extern FrmInfoList g_frmServicesList;




#endif