#include "hatchGlobal.h"

/******************************************************************
	��Ϣ��������������Ϣ��������Ϣ��������Ϣ,����Ӧ��ͬ������
*******************************************************************/
MsgList g_msgRecvList;
MsgList g_msgSendList;
MsgList g_msgTempList;

CHatchAutoCriticalSection g_msgRecvListCriticalSection;
CHatchAutoCriticalSection g_msgSendListCriticalSection;
CHatchAutoCriticalSection g_msgTempListCriticalSection;

CHatchSemaphore g_msgRecvsemaphore;		//������Ϣ�������ź���
CHatchSemaphore g_msgSendsemaphore;		//������Ϣ�������ź���
CHatchSemaphore g_msgTempsemaphore;		//������Ϣ�������ź���

/*****************************************************************
	�û���Ϣ�������� ����Ӧ��ͬ������
******************************************************************/
#include "hatchUserInfo.h"

UserList g_vectUserList;
CHatchAutoCriticalSection g_msgUserListCriticalSection;

/*****************************************************************
	������������¼���������
******************************************************************/
CHatchEvent g_eventRnnerEnd;			//�����Ƿ����������������̳߳��и��������


/*****************************************************************
	��ܷ����б���
******************************************************************/
FrmInfoList g_frmServicesList;

