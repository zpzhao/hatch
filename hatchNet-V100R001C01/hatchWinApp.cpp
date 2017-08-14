
#include "hatchWinApp.h"
#include "hatchLog.h"
#include "hatchList.h"
#include "hatchCommonFile.h"
#include "hatchRunner.h"
#include "hatchGlobal.h"
#include "hatchThreadPool.h"
#include "hatchServices.h"
#include "hatchNet.h"

using namespace std;


CHatchApp *g_pHatchApp = NULL;

CHatchApp::CHatchApp()
{
	m_appThreadPool = NULL;
	m_appServices = NULL;
	m_appNetManager = NULL;
}

CHatchApp::~CHatchApp()
{

}

int CHatchApp::hatchMain(int argc, char *argv[])
{
	m_appNetManager->hatchFactory(SERVICES_PORT,SERVER_IPADRESS,"netchat",NULL);
	m_appNetManager->hatchSetOwnerAddr();

	CHatchNetRecvRunner Recvrun(g_frmServicesList.front()->m_netLayer);
	CHatchNetSendRunner Sendrun(g_frmServicesList.front()->m_netLayer);
	CHatchServerPacketProcRunner Packetrun(m_appServices);

	m_appThreadPool->hatchPush(&Recvrun);
	m_appThreadPool->hatchPush(&Sendrun);
	m_appThreadPool->hatchPush(&Packetrun);

	char sName[USERNAME_MAX_LENGTH];

	if(SERVICES_TYPE)
	{
		CHatchServerTextRunner Textrun(m_appServices);
		m_appThreadPool->hatchPush(&Textrun);

		cout<<"UserName:";
		cin>>sName;
		cout<<endl;
		cout<<"Login :";
		m_appServices->hatchClientLogin(sName);
		m_appServices->hatchServicesWait();
	}
	else
	{
		int end = 0;
		cout<<"please input to end programme:(any key)";
		cin>>end;
	}

	Packetrun.Abort();
	m_appThreadPool->hatchEnd();
	return 0;
}

int CHatchApp::hatchInitialize(void)
{
	int retCode = 0;

	//Log Initialize
	retCode = hatchInitializeLog();
	if(0 != retCode)
	{
		cout<<"Initialize log files failure."<<endl;
		return retCode;
	}
	PRO_LOG("Log system is running now.");

	retCode = hatchInitializeGlobalVar();
	if(0 != retCode)
	{
		cout<<"Initialize global var failure."<<endl;
		FAI_LOG("Initialize global var failure.");
		return retCode;
	}
	PRO_LOG("Initialize global var successed.");

	//ThreadPool Initialize
	retCode = hatchInitializeThreadPool();
	if(0 != retCode)
	{
		cout<<"Initialize threadpool failure."<<endl;
		FAI_LOG("Initialize threadpool failure.");
		return retCode;
	}
	PRO_LOG("Thread pool is running now.");

	retCode = hatchInitializeNetLayer();
	if(0 != retCode)
	{
		cout<<"Initialze net layer failure."<<endl;
		FAI_LOG("Initialze net layer failure.");
		return retCode;
	}
	PRO_LOG("Net is start to excute.");

	retCode = hatchInitializeServices();
	if(0 != retCode)
	{
		cout<<"Initialize service failure."<<endl;
		FAI_LOG("Initialize service failure.");
		return retCode;
	}
	PRO_LOG("Services is start to excute.");

	return retCode;
}

int CHatchApp::hatchInitializeThreadPool(void)
{
	int retCode = -1;

	m_appThreadPool = new CHatchThreadSimplePool();
	if(m_appThreadPool == NULL)
	{
		retCode = -1;
		FAI_LOG("create thread pool failure.");
		return retCode;
	}
	
	retCode = m_appThreadPool->hatchInitialize(APPLICATION_THREAD_NUMBER);
	return retCode;
}

int CHatchApp::hatchDetroy(void)
{
	int retCode = 0;
	if(m_appNetManager != NULL)
	{
		delete m_appNetManager;
	}
	PRO_LOG("Net layer will stop services later.");

	if(m_appThreadPool != NULL)
	{
		delete m_appThreadPool;
	}
	PRO_LOG("Thread pool will shutdown later.");

	if(m_appServices != NULL)
	{
		delete m_appServices;
	}
	PRO_LOG("Services will be shutdown later.");
	
	//other object must be destroy above
	PRO_LOG("Log system will shutdown later.");
	if(NULL != g_pLogFile)
	{
		delete g_pLogFile;
	}

	return retCode;
}

int CHatchApp::hatchInitializeLog(void)
{
	int retCode = -1;

	g_pLogFile = new CHatchLog();
	if(NULL == g_pLogFile)
	{
		cout<<"Create log class object failure."<<endl;
		return retCode;
	}

	retCode = g_pLogFile->hatchInitialize();
	if(0 != retCode)
	{
		cout<<"Initialize log file failure."<<endl;
		return retCode;
	}

	return retCode;
}

int CHatchApp::hatchInitializeServices(void)
{
	int retCode = -1;

	m_appServices = new CHatchServices();
	if(m_appServices == NULL)
	{
		FAI_LOG("memory isn't enough.");
		return retCode;
	}

	m_appServices->hatchSetServiceType(SERVICES_TYPE);
	retCode = m_appServices->hatchInitialize();

	return retCode;
}

int CHatchApp::hatchInitializeNetLayer(void)
{
	if(m_appNetManager != NULL)
	{
		FAI_LOG("net layer has error status.");
		return -1;
	}

	m_appNetManager = new CHatchNetManager();
	if(NULL == m_appNetManager)
	{
		FAI_LOG("memory isn't enough.");
		return -1;
	}

	return 0;
}

int CHatchApp::hatchInitializeRunner(void)
{
	return 0;
}

int CHatchApp::hatchInitializeGlobalVar(void)
{
	int retCode = 0;

	//recv message list
	while(!g_msgRecvList.empty())
	{
		MsgSt *temp = g_msgRecvList.front();
		delete[] temp;
		g_msgRecvList.pop();
	}
	
	//send message list
	while(!g_msgSendList.empty())
	{
		MsgSt *temp = g_msgSendList.front();
		delete[] temp;
		g_msgSendList.pop();
	}
	//temp message list
	while(!g_msgTempList.empty())
	{
		MsgSt *temp = g_msgTempList.front();
		delete[] temp;
		g_msgTempList.pop();
	}

	g_msgRecvsemaphore.hatchCreateSemaphore(NET_READ_MESSAGE_LIST_MAX_LENGTH,0);
	g_msgSendsemaphore.hatchCreateSemaphore(NET_SEND_MESSAGE_LIST_MAX_LENGTH,0);
	g_msgTempsemaphore.hatchCreateSemaphore(NET_TEMP_MESSAGE_LIST_MAX_LENGTH,0);
	g_eventRnnerEnd.hatchCreateEvent();

	return retCode;
}