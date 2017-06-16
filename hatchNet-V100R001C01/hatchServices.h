//hatchServices.h

#ifndef HATCHSERVICES_H_H
#define HATCHSERVICES_H_H

#include "hatchCommonFile.h"
#include "hatchNet.h"
#include "hatchThreadPool.h"
#include "hatchUserInfo.h"

class CHatchProtocol;
class CHatchMsgList;
class CHatchServices
{
public:
	CHatchServices();
	virtual ~CHatchServices();

	int hatchInitialize();
	int hatchInitServer();
	int hatchInitClient();

	int hatchGetServiceType();
	void hatchSetServiceType(int type);

	void hatchServicesEnd();
	void hatchServicesWait();
	void hatchStartText();
	void hatchEndText();

	//int hatchPacketRecvProc(void);
	//int hatchPacketSendProc(void);
	int hatchPacketProc(void);
	int hatchTextProc(void);

	int hatchClientLogin(char *name=NULL);
	int hatchClientLogout(char *name=NULL);
private:

	CHatchEvent m_eventStartText;

	CHatchProtocol *m_pProtocol;
	int m_iServiceType;
};

class CHatchMsgList
{
public:
	CHatchMsgList();
public:
	//����ֻ�ǰѵ������ݷ��뻺�����У�
	//������ݴ�����Ϊ�漰���û���ַ�������������Ӧ�ò�����
	//Ϊ���������Ӧ�ò������ϣ����ﲻ�漰�������ݵĴ��
	static int hatchPushReadMsgList(PMsgSt msg);
	static int hatchPushSendMsgList(PMsgSt msg);
	static int hatchPushTempMsgList(PMsgSt msg);

	static PMsgSt hatchPopReadMsgList();
	static PMsgSt hatchPopSendMsgList();
	static PMsgSt hatchPopTempMsgList();

	static int hatchPushReadMsgGroup(MsgSt *msg);
	static int hatchPushSendMsgGroup(MsgSt *msg);
	static int hatchPushTempMsgGroup(MsgSt *msg);
private:
};

#endif