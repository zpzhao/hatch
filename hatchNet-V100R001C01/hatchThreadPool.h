
#ifndef HATCHTHREADPOOL_H_H
#define HATCHTHREADPOOL_H_H
#include "hatchCommonFile.h"


class CHatchCriticalSection
{
public:
	CHatchCriticalSection();
	virtual ~CHatchCriticalSection();

	int hatchInitCriticalSection();
	int hatchEnterCriticalSection();
	int hatchTryEnterCriticalSection();
	int hatchLeaveCriticalSection();
	int hatchDeleteCriticalSection();
protected:
private:
	CRITICAL_SECTION m_criticalSection;
	int m_iFlag;
};

class CHatchAutoCriticalSection : public CHatchCriticalSection
{
public:
	CHatchAutoCriticalSection();
	~CHatchAutoCriticalSection();

	void hatchEnter();
	void hatchLeave();
private:
	int hatchInitCriticalSection();
	int hatchDeleteCriticalSection();
};

class CHatchSemaphore
{
public:
	CHatchSemaphore();
	~CHatchSemaphore();

	int hatchCreateSemaphore(unsigned long iMax,unsigned long iInit);
	int hatchReleaseSemaphore(unsigned long iStep = 1);
	int hatchWaitSemaphore(void);
	HANDLE hatchGetHandle();
private:
	HANDLE m_hSemaphore;
};

class CHatchEvent
{
public:
	CHatchEvent();
	~CHatchEvent();

	int hatchCreateEvent();
	int hatchCreateEvent(int ManualReset,int Init);
	int hatchSetEvent();
	int hatchResetEvent();
	int hatchWaitEvent();
	HANDLE hatchGetHandle();
private:
	HANDLE m_hEvent;
};

class CHatchRunner
{
public:
	CHatchRunner();
	virtual ~CHatchRunner();

	virtual int Run();
	virtual int Abort();
	virtual int Wait();

	virtual int hatchAttach(void *);
	virtual int hatchDetach(void *);
private:
};


typedef std::list<CHatchRunner*> HatchRunner;

class CHatchThread
{
public:
	CHatchThread();
	virtual ~CHatchThread();

	int hatchCreateThread();
	int hatchCreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes,
						SIZE_T dwStackSize,
						LPTHREAD_START_ROUTINE lpStartAddress,
						LPVOID lpParameter,
						DWORD dwCreationFlags);
	static DWORD WINAPI hatchThreadProc(LPVOID lpParameter);
	int hatchResumeThread();
	int hatchSuspendThread();
	int hatchTerminateThread();
	int hatchExitThread();

	int hatchInitialize(DWORD StackSize,void *Parameter);
	HANDLE hatchGetHandle();
protected:

private:
	HANDLE m_hThreadHandle;
	DWORD m_dwThreadID;
	DWORD m_dwStackSize; 
	void * m_lpParameter;
};


enum EventIndex
{
	Semaphore_Runner	= 0x00,
	End_Thread				  ,
	Event_NULL
};

class CHatchThreadSimplePool
{
public:
	CHatchThreadSimplePool();
	~CHatchThreadSimplePool();

	int hatchInitialize(int threadCount);
	void hatchPush(CHatchRunner *runner);
	CHatchRunner *hatchPop(void);
	int hatchInitThread(int threadCount);
	void hatchEnd();
	
	EventIndex hatchWaitObject();
	void hatchWaitThreadEnd();
	
private:
	HatchRunner m_listRunner;
	CHatchAutoCriticalSection m_criticalSection;
	CHatchSemaphore m_semaphore;
	CHatchEvent m_event;
	CHatchThread *m_threadArray;
	int m_iThreadCount;
};


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

typedef std::vector<HANDLE> HandleList,*PHandleList;
class CHatchTimerQueue
{
public:
	CHatchTimerQueue();
	~CHatchTimerQueue();

	int hatchCreateTimerQueue();
	int hatchCreateTimerQueueTimer(WAITORTIMERCALLBACK Callback,PVOID Parameter,DWORD DueTime);
	int hatchDeleteTimerQueue();
	int hatchChangeTimerQueueTimer(int index,DWORD DueTime);
	int hatchDeleteTimerQueueTimer(int index);
private:
	HANDLE m_hTimerQueue;
	HandleList m_TimerList;
};

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#endif HATCHTHREADPOOL_H_H
