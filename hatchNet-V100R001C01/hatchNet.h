//hatchNet.h

#ifndef HATCHNET_H_H
#define HATCHNET_H_H
#include "hatchCommonFile.h"
#include "hatchThreadPool.h"

class CHatchNet
{
public:
	explicit CHatchNet();
	CHatchNet(const CHatchNet &net);
	virtual ~CHatchNet();

	CHatchNet & operator=(const CHatchNet &net);
	SOCKET hatchGetSocket();

	int hatchCreateSocket(int type,int protocol);
	int hatchBind(struct sockaddr *name,int namelen);
	SOCKET hatchAccept(struct sockaddr *addr,int *addrlen);
	int hatchConnect(struct sockaddr *name,int namelen);
	int hatchListen(int backlog);
	int hatchRecv(char *buf,int len,int flags);
	int hatchSend(char *buf,int len,int flags);
	int hatchRecvFrom(char *buf,int len,int flags,struct sockaddr *from,int *fromlen);
	int hatchSendTo(char *buf,int len,int flags,struct sockaddr *to,int tolen);
	int hatchSelectRecv();

	int hatchGetHostName(char *name,int namelen);
	struct hostent* hatchGetHostByAddr(char *addr,int len,int type);
	struct hostent* hatchGetHostByName(char *name);
private:
	SOCKET m_hSocket;
};

class CHatchNetLayer
{
public:
	CHatchNetLayer();
	virtual ~CHatchNetLayer();

	int hatchAttachNetSerivces(CHatchNet &net);
	int hatchDetachNetServices(CHatchNet &net);

	virtual int hatchPacketRecvProc();
	virtual int hatchPacketSendProc();
	virtual int hatchPacketTempProc();


protected:
	
private:
	CHatchNet *m_pNetServices;
};


class CHatchNetManager
{
public:
	CHatchNetManager();
	~CHatchNetManager();

	int hatchFactory(int port,char *ip,char *sname,CHatchRunner *runner);
	int hatchCreateNetServices(FrmInfo *frm);
	int hatchAttachRunner(FrmInfo *frm);
	int hatchDettachRunner(FrmInfo *frm);

	int hatchAddServiceList(FrmInfo *frm);
	int hatchDelServiceList(FrmInfo *frm);

	int hatchSetOwnerAddr(void);
protected:
	//
private:
	//
};

#endif