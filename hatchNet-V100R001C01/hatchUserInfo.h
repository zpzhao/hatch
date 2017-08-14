#ifndef HATCHUSERINFO_H_H
#define HATCHUSERINFO_H_H
#include "hatchCommonFile.h"

class CHatchUserInfo
{
public:
	CHatchUserInfo();
	~CHatchUserInfo();

	void hatchSetAddr(in_addr *addr);
	in_addr hatchGetAddr(void);
	void hatchSetUserName(char *name);
	void hatchGetUserName(char *name,int namelen);
	unsigned long hatchGetIndex(void);
	void hatchSetIndex(unsigned long index);
	void hatchSetPort(unsigned int port);
	unsigned int hatchGetPort(void);
	UserStatus hatchGetStatus(void);
	void hatchSetStatus(UserStatus status);
private:
	in_addr m_userIPAddr;
	unsigned int m_userPort;
	char m_sUserName[USERNAME_MAX_LENGTH];
	UserStatus m_eUserStatus;
	unsigned long m_ulIndex;
};

class CHatchUserList
{
public:
	CHatchUserList();
public:
	static int hatchPushUser(CHatchUserInfo *user);
	static CHatchUserInfo* hatchPopUser(char *name,int index);
	static UserListIterator hatchLocateUser(char *name);
	static int hatchIsUserInList(char *name);
private:
};

#endif