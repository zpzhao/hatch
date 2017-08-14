
#ifndef HATCHRUNNER_H_H
#define HATCHRUNNER_H_H
#include "hatchThreadPool.h"

class CHatchServices;
class CHatchNetLayer;

/******************************************************************
	业务任务 
******************************************************************/
class CHatchServerRecvRunner :public CHatchRunner
{
public:
	CHatchServerRecvRunner(CHatchServices*);
	virtual ~CHatchServerRecvRunner();

	virtual int Run();
	virtual int Abort();
private:
	CHatchServices *m_pServices;
};


class CHatchServerSendRunner :public CHatchRunner
{
public:
	CHatchServerSendRunner(CHatchServices*);
	virtual ~CHatchServerSendRunner();

	virtual int Run();
	virtual int Abort();
private:
	CHatchServices *m_pServices;
};

class CHatchServerPacketProcRunner :public CHatchRunner
{
public:
	CHatchServerPacketProcRunner(CHatchServices*);
	virtual ~CHatchServerPacketProcRunner();

	virtual int Run();
	virtual int Abort();
private:
	CHatchServices *m_pServices;
};

class CHatchServerTextRunner :public CHatchRunner
{
public:
	CHatchServerTextRunner(CHatchServices*);
	virtual ~CHatchServerTextRunner();

	virtual int Run();
	virtual int Abort();
	virtual int Wait();
private:
	CHatchServices *m_pServices;
};


/******************************************************************
	网络底层
******************************************************************/
class CHatchNetRecvRunner :public CHatchRunner
{
public:
	CHatchNetRecvRunner(CHatchNetLayer*);
	virtual ~CHatchNetRecvRunner();

	virtual int Run();
	virtual int Abort();

	virtual int hatchAttach(void *);
	virtual int hatchDetach(void *);
private:
	CHatchNetLayer *m_pNetService;
};

class CHatchNetSendRunner :public CHatchRunner
{
public:
	CHatchNetSendRunner(CHatchNetLayer*);
	virtual ~CHatchNetSendRunner();

	virtual int Run();
	virtual int Abort();

	virtual int hatchAttach(void *);
	virtual int hatchDetach(void *);
private:
	CHatchNetLayer *m_pNetService;
};


#endif