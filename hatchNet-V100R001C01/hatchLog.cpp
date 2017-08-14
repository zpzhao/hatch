
#include <fstream>
#include "hatchLog.h"

using namespace std;

CHatchLog *g_pLogFile = NULL;

CHatchLog::CHatchLog()
{
	m_cRunLogFilePath[0] = '\0';
	m_cDevLogFilePath[0] = '\0';
	m_cMemLogFilePath[0] = '\0';
}

CHatchLog::~CHatchLog()
{
	m_cRunLogFilePath[0] = '\0';
	m_cDevLogFilePath[0] = '\0';
	m_cMemLogFilePath[0] = '\0';

	if(m_fRunLogFile)
	{
		m_fRunLogFile.flush();
		m_fRunLogFile.close();
	}

	if(m_fDevLogFile)
	{
		m_fDevLogFile.flush();
		m_fDevLogFile.close();
	}

	if(m_fMemLogFile)
	{
		m_fMemLogFile.flush();
		m_fMemLogFile.close();
	}
}

int CHatchLog::hatchInitialize(void)
{
	int retCode = 0;

	hatchSetLogFilePath(const_cast<char *>(const_RunLogPath),
						const_cast<char *>(const_DevLogPath),
						const_cast<char *>(const_MemLogPath));

	retCode = hatchCreateLogFile();

	return retCode;
}
	
int CHatchLog::hatchCreateLogFile(void)
{
	int temp = 0,retCode = -1;
	
	retCode = hatchCreateNewLogFile(RUN_LOG);
	retCode = retCode < (temp = hatchCreateNewLogFile(DEVICE_LOG)) ? retCode : temp;
	retCode = retCode < (temp = hatchCreateNewLogFile(MEMORY_LOG)) ? retCode : temp;

	return retCode;
}

int CHatchLog::hatchCreateNewLogFile(hatchLogFileType type)
{
	int retCode = -1,temp ,tryCount = 10;

	switch(type)
	{
	case RUN_LOG:
		if(hatchLogFilePathIsValid(RUN_LOG))
		{
			cout<<"Run log file path is invalid."<<endl;
			return retCode;
		}

		temp = tryCount;
		while(temp--)
		{
			m_fRunLogFile.open(m_cRunLogFilePath,ios_base::out | ios_base::trunc );
			if(m_fRunLogFile)
			{
				break;
			}
		}

		if(temp > 0)
			retCode = 0;

		break;
	case DEVICE_LOG:
		if(hatchLogFilePathIsValid(DEVICE_LOG))
		{
			cout<<"Device log file path is invalid."<<endl;
			return retCode;
		}

		temp = tryCount;
		while(temp--)
		{
			m_fDevLogFile.open(m_cDevLogFilePath,ios_base::out | ios_base::trunc );
			if(m_fDevLogFile)
			{
				break;
			}
		}

		if(temp > 0)
			retCode = 0;

		break;
	case MEMORY_LOG:
		if(hatchLogFilePathIsValid(MEMORY_LOG))
		{
			cout<<"Memory log file path is invalid."<<endl;
			return retCode;
		}

		temp = tryCount;
		while(temp--)
		{
			m_fMemLogFile.open(m_cMemLogFilePath,ios_base::out | ios_base::trunc );
			if(m_fMemLogFile)
			{
				break;
			}
		}

		if(temp > 0)
			retCode = 0;

		break;
	default:
		return retCode;
	}
	
	cout<<"created log file successfully."<<endl;
	return retCode;
}

int CHatchLog::hatchSaveOldLogFile(hatchLogFileType type,int order)
{
	return 0;
}
	
int CHatchLog::hatchLogFileIsExist(hatchLogFileType type,int order)
{
	return 0;
}
	
void CHatchLog::hatchSetLogFilePath(char *run, char *dev, char *mem)
{
	int tempPathLength = 0;

	if(NULL != run)
	{
		tempPathLength = strlen(run);
		if((tempPathLength > const_LogFilePathMaxLength) || (tempPathLength < 1))
		{
			cout<<"The path of running log file is invalid."<<endl;
			return;
		}
		strcpy_s(m_cRunLogFilePath,const_LogFilePathMaxLength,run);
		m_cRunLogFilePath[tempPathLength] = '\0';
	}

	if(NULL != dev)
	{
		tempPathLength = strlen(dev);
		if((tempPathLength > const_LogFilePathMaxLength) || (tempPathLength < 1))
		{
			cout<<"The path of device log file is invalid."<<endl;
			return;
		}
		strcpy_s(m_cDevLogFilePath,const_LogFilePathMaxLength,dev);
		m_cDevLogFilePath[tempPathLength] = '\0';
	}

	if(NULL != mem)
	{
		tempPathLength = strlen(mem);
		if((tempPathLength > const_LogFilePathMaxLength) || (tempPathLength < 1))
		{
			cout<<"The path of memory log file is invalid."<<endl;
			return;
		}
		strcpy_s(m_cMemLogFilePath,const_LogFilePathMaxLength,mem);
		m_cMemLogFilePath[tempPathLength] = '\0';
	}

	return;
}	

void CHatchLog::hatchGetLogFilePath(char *run, char *dev, char *mem)
{
	int tempPathLength = 0;
	
	try
	{
		if(NULL != run)
		{
			strcpy_s(run,const_LogFilePathMaxLength,m_cRunLogFilePath);
		}

		if(NULL != dev)
		{
			strcpy_s(dev,const_LogFilePathMaxLength,m_cDevLogFilePath);
		}

		if(NULL != mem)
		{
			strcpy_s(mem,const_LogFilePathMaxLength,m_cMemLogFilePath);
		}
	}catch(...)
	{
		//不做处理
	}

	return;
}
	
int CHatchLog::hatchCheckLogStatus(hatchLogFileType type)
{
	return 0;
}
	
int CHatchLog::hatchCheckLogFileSize(hatchLogFileType type)
{
	return 0;
}

int CHatchLog::hatchLogFilePathIsValid(hatchLogFileType type)
{
	int retCode = -1;
	int tempLogFilePathLength = 0;

	switch(type)
	{
	case RUN_LOG:
		tempLogFilePathLength = strlen(m_cRunLogFilePath);
		break;
	case DEVICE_LOG:
		tempLogFilePathLength = strlen(m_cDevLogFilePath);
		break;
	case MEMORY_LOG:
		tempLogFilePathLength = strlen(m_cMemLogFilePath);
		break;
	default:
		return retCode;
	}

	if(tempLogFilePathLength > 0)
		retCode = 0;

	return retCode;
}

void CHatchLog::hatchPrintLog(const char *format,...)
{
	m_criticalSection.hatchEnter();

	char *pBuf = m_cLogBuffer;
	va_list args;
	va_start(args,format);

	SYSTEMTIME LocalTime;
	unsigned long dwLastError = GetLastError();
	unsigned long dwThreadId;
	dwThreadId = GetCurrentThreadId();

	GetLocalTime(&LocalTime);
	memset(pBuf,0,const_LogStringMaxLength);

	sprintf_s(pBuf,const_LogStringMaxLength, "[%.08X] %.04u-%.02u-%.02u %.02u:%02u:%02u ",
			dwThreadId,
			LocalTime.wYear,							
			LocalTime.wMonth,							
			LocalTime.wDay,								
			LocalTime.wHour,							
			LocalTime.wMinute,							
			LocalTime.wSecond);

	vsprintf_s(pBuf+31,const_LogStringMaxLength - 31,format,args);
	m_fRunLogFile<<m_cLogBuffer<<endl;

	va_end(args);	

	m_criticalSection.hatchLeave();
}

void CHatchLog::hatchPrintLog(enum hatchLogLevel level,char *filename,const char *format,...)
{
	m_criticalSection.hatchEnter();

	char *pBuf = m_cLogBuffer;
	va_list args;
	va_start(args,format);

	SYSTEMTIME LocalTime;
	unsigned long dwLastError = GetLastError();
	unsigned long dwThreadId;
	dwThreadId = GetCurrentThreadId();

	GetLocalTime(&LocalTime);
	memset(pBuf,0,const_LogStringMaxLength);

	sprintf_s(pBuf,const_LogStringMaxLength, "[%.04s][%.08X] %.04u-%.02u-%.02u %.02u:%02u:%02u ",
			const_LogLevelString[level],
			dwThreadId,
			LocalTime.wYear,							
			LocalTime.wMonth,							
			LocalTime.wDay,								
			LocalTime.wHour,							
			LocalTime.wMinute,							
			LocalTime.wSecond);

	vsprintf_s(pBuf + 36,const_LogStringMaxLength - 36,format,args);

	CString str;
	str= filename;
	str = str.Right(str.GetLength() - str.ReverseFind('\\') - 1);
	str = '[' + str + ']';
	str = pBuf + str;

	strncpy_s(pBuf,const_LogStringMaxLength,str.GetBuffer(),const_LogStringMaxLength);


	m_fRunLogFile<<pBuf<<endl;

	va_end(args);
	
	m_criticalSection.hatchLeave();
}