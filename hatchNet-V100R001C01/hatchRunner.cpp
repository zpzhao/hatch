#include "hatchRunner.h"
#include "hatchServices.h"
#include "hatchNet.h"
#include "hatchLog.h"
#include "hatchGlobal.h"

CHatchServerRecvRunner::CHatchServerRecvRunner(CHatchServices *pService)
{
	m_pServices = pService;
}

CHatchServerRecvRunner::~CHatchServerRecvRunner()
{
}

int CHatchServerRecvRunner::Run()
{
	int ret = 0;
	//ret = m_pServices->hatchPacketRecvProc();
	return ret;
}
	
int CHatchServerRecvRunner::Abort()
{
	m_pServices->hatchServicesEnd();
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

CHatchServerSendRunner::CHatchServerSendRunner(CHatchServices *pService)
{
	m_pServices = pService;
}

CHatchServerSendRunner::~CHatchServerSendRunner()
{
}

int CHatchServerSendRunner::Run()
{
	int ret = 0;
	//ret = m_pServices->hatchPacketSendProc();
	return ret;
}
	
int CHatchServerSendRunner::Abort()
{
	m_pServices->hatchServicesEnd();
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

CHatchServerPacketProcRunner::CHatchServerPacketProcRunner(CHatchServices *pService)
{
	m_pServices = pService;
}

CHatchServerPacketProcRunner::~CHatchServerPacketProcRunner()
{
}

int CHatchServerPacketProcRunner::Run()
{
	int ret = m_pServices->hatchPacketProc();
	return ret;
}
	
int CHatchServerPacketProcRunner::Abort()
{
	m_pServices->hatchServicesEnd();
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

CHatchServerTextRunner::CHatchServerTextRunner(CHatchServices *pService)
{
	m_pServices = pService;
}

CHatchServerTextRunner::~CHatchServerTextRunner()
{
}

int CHatchServerTextRunner::Run()
{
	int ret = m_pServices->hatchTextProc();
	return ret;
}
	
int CHatchServerTextRunner::Abort()
{
	m_pServices->hatchServicesEnd();
	return 0;
}

int CHatchServerTextRunner::Wait()
{
	m_pServices->hatchServicesWait();
	return 0;
}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

CHatchNetRecvRunner::CHatchNetRecvRunner(CHatchNetLayer *pNet)
{
	if(pNet == NULL)
	{
		ERR_LOG("Net object is null");
	}

	m_pNetService = pNet;
}

CHatchNetRecvRunner::~CHatchNetRecvRunner()
{
}

int CHatchNetRecvRunner::Run()
{
	if(NULL == m_pNetService)
	{
		ERR_LOG("Net object is null,netserivices aren't running.");
		return -1;
	}

	int ret = m_pNetService->hatchPacketRecvProc();
	return ret;
}

int CHatchNetRecvRunner::Abort()
{
	g_eventRnnerEnd.hatchSetEvent();

	PRO_LOG("Net recv service abort now.");
	return 0;
}

int CHatchNetRecvRunner::hatchAttach(void *p)
{
	if(NULL == p)
	{
		FAI_LOG("attach services failure.");
		return -1;
	}

	m_pNetService = (CHatchNetLayer *)p;
	return 0;
}

int CHatchNetRecvRunner::hatchDetach(void *p)
{
	m_pNetService = NULL;

	return 0;
}
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

CHatchNetSendRunner::CHatchNetSendRunner(CHatchNetLayer *pNet)
{
	if(pNet == NULL)
	{
		ERR_LOG("Net object is null");
	}

	m_pNetService = pNet;
}

CHatchNetSendRunner::~CHatchNetSendRunner()
{
}

int CHatchNetSendRunner::Run()
{
	if(NULL == m_pNetService)
	{
		ERR_LOG("Net object is null,netserivices aren't running.");
		return -1;
	}

	int ret = m_pNetService->hatchPacketSendProc();
	return ret;
}

int CHatchNetSendRunner::Abort()
{
	g_eventRnnerEnd.hatchSetEvent();

	PRO_LOG("Net recv service abort now.");
	return 0;
}

int CHatchNetSendRunner::hatchAttach(void *p)
{
	if(NULL == p)
	{
		FAI_LOG("attach services failure.");
		return -1;
	}

	m_pNetService = (CHatchNetLayer *)p;
	return 0;
}

int CHatchNetSendRunner::hatchDetach(void *p)
{
	m_pNetService = NULL;

	return 0;
}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////