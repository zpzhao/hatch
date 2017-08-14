#include "hatchUserInfo.h"
#include "hatchLog.h"
#include "hatchGlobal.h"

CHatchUserInfo::CHatchUserInfo()
{
	memset(&m_userIPAddr,0,sizeof(in_addr));
	m_userPort = 0;
	m_ulIndex = -1;
	memset(m_sUserName,0,sizeof(m_sUserName));
}

CHatchUserInfo::~CHatchUserInfo()
{

}

void CHatchUserInfo::hatchSetAddr(in_addr *addr)
{
	if(NULL == addr)
		return ;
	m_userIPAddr = *addr;
}

in_addr CHatchUserInfo::hatchGetAddr(void)
{
	return m_userIPAddr;
}

void CHatchUserInfo::hatchSetUserName(char *name)
{
	if(NULL == name)
		return ;
	strncpy_s(m_sUserName,USERNAME_MAX_LENGTH,name,USERNAME_MAX_LENGTH);
}

void CHatchUserInfo::hatchGetUserName(char *name,int namelen)
{
	strncpy_s(name,namelen,m_sUserName,USERNAME_MAX_LENGTH);
}
	
unsigned long CHatchUserInfo::hatchGetIndex(void)
{
	return m_ulIndex;
}
	
void CHatchUserInfo::hatchSetIndex(unsigned long index)
{
	m_ulIndex = index;
}

void CHatchUserInfo::hatchSetPort(unsigned int port)
{
	m_userPort = port;
}
	
unsigned int CHatchUserInfo::hatchGetPort(void)
{
	return m_userPort;
}

UserStatus CHatchUserInfo::hatchGetStatus(void)
{
	return m_eUserStatus;
}
	
void CHatchUserInfo::hatchSetStatus(UserStatus status)
{
	m_eUserStatus = status;
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

int CHatchUserList::hatchPushUser(CHatchUserInfo *user)
{
	char uName[USERNAME_MAX_LENGTH]={'\0'};
	user->hatchGetUserName(uName,USERNAME_MAX_LENGTH);
	UserListIterator ite = hatchLocateUser(uName);

	g_msgUserListCriticalSection.hatchEnter();
	if(ite != g_vectUserList.end())
	{
		g_msgUserListCriticalSection.hatchLeave();
		return -1;
	}
	g_vectUserList.push_back(user);
	g_msgUserListCriticalSection.hatchLeave();
	return 0;
}
	
CHatchUserInfo* CHatchUserList::hatchPopUser(char *name,int index)
{
	CHatchUserInfo *tmpUser=NULL;
	UserListIterator ite = hatchLocateUser(name);
	g_msgUserListCriticalSection.hatchEnter();
	if(ite != g_vectUserList.end())
	{		
		tmpUser = *ite;
		g_vectUserList.erase(ite);
	}
	g_msgUserListCriticalSection.hatchLeave();
	return tmpUser;
}

UserListIterator CHatchUserList::hatchLocateUser(char *name)
{
	char uName[USERNAME_MAX_LENGTH]={'\0'};
	g_msgUserListCriticalSection.hatchEnter();	
	UserListIterator ite = g_vectUserList.begin(),itend = g_vectUserList.end();
	if(name == NULL) //client own
	{
		++ite;
		g_msgUserListCriticalSection.hatchLeave();
		return ite;
	}

	for(;ite != itend;ite++)
	{
		(*ite)->hatchGetUserName(uName,USERNAME_MAX_LENGTH);
		if(strcmp(uName,name) == 0)
		{
			//WAR_LOG("user is already in the userlist.");
			break;
		}
	}
	g_msgUserListCriticalSection.hatchLeave();
	return ite;
}

int CHatchUserList::hatchIsUserInList(char *name)
{
	int ret = 0;
	UserListIterator ite = hatchLocateUser(name);
	g_msgUserListCriticalSection.hatchEnter();	
	if(ite != g_vectUserList.end())
	{
		g_msgUserListCriticalSection.hatchLeave();
		ret = 1;
	}
	g_msgUserListCriticalSection.hatchLeave();
	return ret;
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////