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
	//这里只是把单个数据放入缓冲区中，
	//多个数据存入因为涉及到用户地址重新组包，访问应用层数据
	//为了网络层与应用层减少耦合，这里不涉及成组数据的存放
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