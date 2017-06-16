#include "hatchThreadPool.h"
#include "hatchLog.h"

CHatchCriticalSection::CHatchCriticalSection()
{
	m_iFlag = 0;
}

CHatchCriticalSection::~CHatchCriticalSection()
{
}

int CHatchCriticalSection::hatchInitCriticalSection()
{
	::InitializeCriticalSection(&m_criticalSection);
	m_iFlag = 1;

	return 0;
}

int CHatchCriticalSection::hatchEnterCriticalSection()
{
	::EnterCriticalSection(&m_criticalSection);
	return 0;
}

int CHatchCriticalSection::hatchTryEnterCriticalSection()
{
	return ::TryEnterCriticalSection(&m_criticalSection);
}

int CHatchCriticalSection::hatchLeaveCriticalSection()
{
	::LeaveCriticalSection(&m_criticalSection);
	return 0;
}

int CHatchCriticalSection::hatchDeleteCriticalSection()
{
	if(m_iFlag)
	{
		m_iFlag = 0;
		::DeleteCriticalSection(&m_criticalSection);
	}
	else
	{
		ERR_LOG("criticalsection deleted before initialize it.");
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

CHatchAutoCriticalSection::CHatchAutoCriticalSection()
	:CHatchCriticalSection()
{
	CHatchCriticalSection::hatchInitCriticalSection();
}

CHatchAutoCriticalSection::~CHatchAutoCriticalSection()
{
	CHatchCriticalSection::hatchDeleteCriticalSection();
}

void CHatchAutoCriticalSection::hatchEnter()
{
	hatchEnterCriticalSection();
}

void CHatchAutoCriticalSection::hatchLeave()
{
	hatchLeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

CHatchSemaphore::CHatchSemaphore()
{
	m_hSemaphore = NULL;
}

CHatchSemaphore::~CHatchSemaphore()
{
	CloseHandle(m_hSemaphore);
}

int CHatchSemaphore::hatchCreateSemaphore(unsigned long iMax,unsigned long iInit)
{
	m_hSemaphore = ::CreateSemaphore(NULL,iInit,iMax,NULL);
	if(m_hSemaphore == NULL)
	{
		FAI_LOG("creating semaphore object failure.");
		return -1;
	}
	return 0;
}
	
int CHatchSemaphore::hatchReleaseSemaphore(unsigned long iStep)
{
	int ret = ::ReleaseSemaphore(m_hSemaphore,iStep,NULL);
	if(ret == 0)
	{
		unsigned long Eret = GetLastError();
		switch(Eret)
		{
		case ERROR_TOO_MANY_POSTS:
			break;
		default:
			FAI_LOG1("ReleaseSemaphore is error.ErrorCode:%u",Eret);
			break;
		}
		return -1;
	}

	return 0;
}
	
int CHatchSemaphore::hatchWaitSemaphore(void)
{
	int ret = ::WaitForSingleObject(m_hSemaphore,INFINITE);
	
	if(ret != 0)
	{
		unsigned long Eret = GetLastError();
		return Eret;
	}
	return 0;
}

HANDLE CHatchSemaphore::hatchGetHandle()
{
	return m_hSemaphore;
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


CHatchEvent::CHatchEvent()
{
	m_hEvent = NULL;
}

CHatchEvent::~CHatchEvent()
{
	CloseHandle(m_hEvent);
}

int CHatchEvent::hatchCreateEvent()
{
	int ret = hatchCreateEvent(1,0);
	if(ret < 0)
	{
		FAI_LOG("create event object failure.");
		return ret;
	}

	return ret;
}
	
int CHatchEvent::hatchCreateEvent(int ManualReset,int Init)
{
	m_hEvent = ::CreateEvent(NULL,ManualReset,Init,NULL);
	if(NULL == m_hEvent)
	{
		FAI_LOG("create event object failure.");
		return -1;
	}
	return 0;
}
	
int CHatchEvent::hatchSetEvent()
{
	int ret = ::SetEvent(m_hEvent);
	if(ret)
		return 0;
	return -1;
}
	
int CHatchEvent::hatchResetEvent()
{
	int ret = ::ResetEvent(m_hEvent);
	if(ret)
		return 0;
	return -1;
}

int CHatchEvent::hatchWaitEvent()
{
	int ret = ::WaitForSingleObject(m_hEvent,INFINITE);
	if(ret == 0)
		return ret;
	else
		return -1;
}

HANDLE CHatchEvent::hatchGetHandle()
{
	return m_hEvent;
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


CHatchRunner::CHatchRunner()
{
}

CHatchRunner::~CHatchRunner()
{
}

int CHatchRunner::Run()
{
	PRO_LOG("Runner.");
	return 0;
}

int CHatchRunner::Abort()
{
	return 0;
}

int CHatchRunner::Wait()
{
	return 0;
}

int CHatchRunner::hatchAttach(void *p)
{
	return 0;
}

int CHatchRunner::hatchDetach(void *p)
{
	return 0;
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

CHatchThread::CHatchThread()
{
	m_hThreadHandle = NULL;
	m_dwThreadID = -1;
	m_dwStackSize = -1;
	m_lpParameter = reinterpret_cast<void *>(this);
}

CHatchThread::~CHatchThread()
{
	::WaitForSingleObject(m_hThreadHandle,10);
	CloseHandle(m_hThreadHandle);
}

int CHatchThread::hatchCreateThread()
{
	int ret = hatchCreateThread(NULL,0,hatchThreadProc,m_lpParameter,0);
	return ret;
}

int CHatchThread::hatchCreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes,
						SIZE_T dwStackSize,
						LPTHREAD_START_ROUTINE lpStartAddress,
						LPVOID lpParameter,
						DWORD dwCreationFlags)
{
	m_hThreadHandle = ::CreateThread(lpThreadAttributes,dwStackSize,lpStartAddress,lpParameter,dwCreationFlags,&m_dwThreadID);
	if(NULL == m_hThreadHandle)
	{
		FAI_LOG("create thread failured.");
		return -1;
	}

	return 0;
}

DWORD WINAPI CHatchThread::hatchThreadProc(LPVOID lpParameter)
{
	CHatchThreadSimplePool *pThreadPool = reinterpret_cast<CHatchThreadSimplePool*>(lpParameter);
	int ret = 0;
	EventIndex eIndex;
	CHatchRunner *pJob = NULL;

	while(1)
	{
		eIndex = pThreadPool->hatchWaitObject();
		switch(eIndex)
		{
		case Semaphore_Runner:
			pJob = pThreadPool->hatchPop();
			if(NULL != pJob)
				pJob->Run();
			continue;
		case End_Thread:
			PRO_LOG("thread end.");
			return 0;
			break;
		default:
			FAI_LOG("thread is running failure.");
			return -1;
			break;
		}
	}

	return 0;
}

int CHatchThread::hatchResumeThread()
{
	int ret = ::ResumeThread(m_hThreadHandle);
	if(ret < 0)
		FAI_LOG1("ResumeThread excute failured,thread id=%d",m_dwThreadID);
	return ret;
}

int CHatchThread::hatchSuspendThread()
{
	return 0;
}
	
int CHatchThread::hatchTerminateThread()
{
	return 0;
}
	
int CHatchThread::hatchExitThread()
{
	return 0;
}

int CHatchThread::hatchInitialize(DWORD StackSize,void *Parameter)
{
	m_dwStackSize = StackSize;
	m_lpParameter = Parameter;

	return 0;
}

HANDLE CHatchThread::hatchGetHandle()
{
	return m_hThreadHandle;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CHatchThreadSimplePool::CHatchThreadSimplePool()
{
	m_threadArray = NULL;
}

CHatchThreadSimplePool::~CHatchThreadSimplePool()
{
	if(m_threadArray != NULL)
		delete[] m_threadArray;

	while(!m_listRunner.empty())
	{
		m_listRunner.clear();
	}
}

int CHatchThreadSimplePool::hatchInitialize(int threadCount)
{
	int ret = 0;
	do
	{
		ret = m_semaphore.hatchCreateSemaphore(SEMAPHORE_MAX_COUNT,0);
		if(ret < 0)
		{
			break;
		}

		ret = m_event.hatchCreateEvent();
		if(ret < 0)
		{
			break;
		}

		ret = hatchInitThread(threadCount);
		if(ret < 0)
		{
			break;
		}

		m_iThreadCount = threadCount;
	}
	while(0);
	
	if(ret < 0)
	{
		FAI_LOG("thread pool initialize failure.");
	}

	return ret;
}

void CHatchThreadSimplePool::hatchPush(CHatchRunner *runner)
{
	m_criticalSection.hatchEnter();
	m_listRunner.push_back(runner);
	m_criticalSection.hatchLeave();

	m_semaphore.hatchReleaseSemaphore();
}

CHatchRunner *CHatchThreadSimplePool::hatchPop(void)
{
	CHatchRunner *temp = NULL;

	m_criticalSection.hatchEnter();
	if(m_listRunner.size() > 0)
	{
		temp = m_listRunner.front();
		m_listRunner.pop_front();
	}
	else
	{
		temp = NULL;
	}
	m_criticalSection.hatchLeave();

	return temp;
}
	
int CHatchThreadSimplePool::hatchInitThread(int threadCount)
{
	//create thread array
	m_threadArray = new CHatchThread[threadCount];
	if(NULL == m_threadArray)
	{
		FAI_LOG("thread array wasn't created.");
		return -1;
	}

	int i = 0,ret = 0;
	for(; i < threadCount; i++)
	{
		m_threadArray[i].hatchInitialize(0,reinterpret_cast<void *>(this));
		ret = m_threadArray[i].hatchCreateThread();
		if(ret < 0)
		{
			FAI_LOG("create thread failure.");
			delete[] m_threadArray;
			m_threadArray = NULL;
			return ret;
		}
	}

	return 0;
}

EventIndex CHatchThreadSimplePool::hatchWaitObject()
{
	int ret = 0;
	HANDLE temp[2];
	temp[0] = m_event.hatchGetHandle();
	temp[1] = m_semaphore.hatchGetHandle();
	
	ret = ::WaitForMultipleObjects(2,temp,FALSE,INFINITE);
	switch(ret)
	{
	case WAIT_OBJECT_0:
		return End_Thread;
	case WAIT_OBJECT_0 + 1:
		return Semaphore_Runner;
	default:
		return Event_NULL;
	}
}

void CHatchThreadSimplePool::hatchEnd()
{
	m_event.hatchSetEvent();
	hatchWaitThreadEnd();
}

void CHatchThreadSimplePool::hatchWaitThreadEnd()
{
	HANDLE *hArray = new HANDLE[m_iThreadCount];
	if(hArray == NULL)
	{
		FAI_LOG("momery isn't enough.");
		return;
	}

	for(int i=0; i < m_iThreadCount; i++)
	{
		hArray[i] = m_threadArray[i].hatchGetHandle();
	}

	int ret = ::WaitForMultipleObjects(m_iThreadCount,hArray,TRUE,INFINITE);
	if(ret != 0)
	{
		FAI_LOG("Wait threads ending failure.");
	}

	delete[] hArray;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CHatchTimerQueue::CHatchTimerQueue()
{
	m_hTimerQueue = NULL;
	m_TimerList.clear();
}

CHatchTimerQueue::~CHatchTimerQueue()
{
	hatchDeleteTimerQueue();
}

int CHatchTimerQueue::hatchCreateTimerQueue()
{
	m_hTimerQueue = ::CreateTimerQueue();
	if(NULL == m_hTimerQueue)
	{
		FAI_LOG("create timerQueue failure.");
		return -1;
	}
	return 0;
}
	
int CHatchTimerQueue::hatchCreateTimerQueueTimer(WAITORTIMERCALLBACK Callback,PVOID Parameter,DWORD DueTime)
{
	HANDLE htimer = NULL;
	int ret = ::CreateTimerQueueTimer(&htimer,m_hTimerQueue,Callback,Parameter,DueTime,1,0);
	if(ret == 0)
	{
		FAI_LOG("create timer failure.");
		return -1;
	}
	m_TimerList.push_back(htimer);

	return 0;
}
	
int CHatchTimerQueue::hatchDeleteTimerQueue()
{
	m_TimerList.clear();
	int ret = ::DeleteTimerQueue(m_hTimerQueue);
	if(ret == 0)
	{
		FAI_LOG("delete timer queue failure.");
		return -1;
	}

	return 0;
}
	
int CHatchTimerQueue::hatchChangeTimerQueueTimer(int index,DWORD DueTime)
{
	int ret = ::ChangeTimerQueueTimer(m_hTimerQueue,m_TimerList[index],DueTime,1);
	if(ret == 0)
	{
		FAI_LOG("Change timer failure.");
		return -1;
	}
	return 0;
}
	
int CHatchTimerQueue::hatchDeleteTimerQueueTimer(int index)
{
	//
	return 0;
}