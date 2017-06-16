
#ifndef HATCHPROTOCOL_H_H
#define HATCHPROTOCOL_H_H

class CHatchServices;
class CHatchProtocol
{
public:
	CHatchProtocol();
	CHatchProtocol(CHatchServices *pService);
	virtual ~CHatchProtocol();

	virtual int hatchMsgProc(char *buf);
	virtual int hatchMsgLogin(char *buf);
	virtual int hatchMsgUpdate(char *buf);
	virtual int hatchMsgText(char *buf);
	virtual int hatchMsgBackSyn(char *buf);
	virtual int hatchMsgLogout(char *buf);
protected:
	CHatchServices *m_pServices;
};

//server protocol define
class CHatchProtocolServer : public CHatchProtocol
{
public:
	CHatchProtocolServer();
	CHatchProtocolServer(CHatchServices *pService);
	virtual ~CHatchProtocolServer();

	virtual int hatchMsgLogin(char *buf);
	virtual int hatchMsgUpdate(char *buf);
	virtual int hatchMsgText(char *buf);
	virtual int hatchMsgBackSyn(char *buf);
	virtual int hatchMsgLogout(char *buf);
	virtual int hatchMsgUpdateDown(char *buf);
private:
};

//client protocol define
class CHatchProtocolClient : public CHatchProtocol
{
public:
	CHatchProtocolClient();
	CHatchProtocolClient(CHatchServices *pService);
	virtual ~CHatchProtocolClient();

	virtual int hatchMsgLogin(char *buf);
	virtual int hatchMsgUpdate(char *buf);
	virtual int hatchMsgText(char *buf);
	virtual int hatchMsgBackSyn(char *buf);
	virtual int hatchMsgLogout(char *buf);
	virtual int hatchMsgUpdateUp(char *buf);
private:
};

//check between some times,timer protocol define
class CHatchProtocolTimer : public CHatchProtocol
{
public:
	CHatchProtocolTimer();
	~CHatchProtocolTimer();
private:
};

#endif