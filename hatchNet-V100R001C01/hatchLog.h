
#ifndef HATCHLOG_H_H
#define HATCHLOG_H_H
#include <fstream>
#include <atlstr.h>
#include "hatchCommonFile.h"
#include "hatchThreadPool.h"

const int const_LogFilePathMaxLength = 512;
const int const_LogFileSaveOldMaxCount = 3;
const unsigned long const_LogFileMaxSize = 1024*1024*1024;
const int const_LogStringMaxLength = 255;


enum hatchLogFileType
{
	RUN_LOG			= 0X01,
	DEVICE_LOG			  ,
	MEMORY_LOG			  ,
	NULL_LOG		= 0X00
};

enum hatchLogLevel
{
	LOGLEVEL_PROMPT		=	0X01,
	LOGLEVEL_WARN				,
	LOGLEVEL_ERROR				,
	LOGLEVEL_FAILURE			,
	NULL_LOGLEVEL		=	0x00
};

const char const_LogLevelString[][4] = {"NUL","PRO","WAR","ERR","FAI"};

class CHatchLog
{
public:
	CHatchLog();
	virtual ~CHatchLog();

	int hatchInitialize(void);
	int hatchCreateLogFile(void);
	int hatchCreateNewLogFile(hatchLogFileType type);
	int hatchSaveOldLogFile(hatchLogFileType type,int order);

	int hatchLogFileIsExist(hatchLogFileType type,int order);
	int hatchLogFilePathIsValid(hatchLogFileType type);

	void hatchSetLogFilePath(char *run, char *dev, char *mem);	
	void hatchGetLogFilePath(char *run, char *dev, char *mem);

	int hatchCheckLogStatus(hatchLogFileType type);
	int hatchCheckLogFileSize(hatchLogFileType type);

	void hatchPrintLog(const char *format,...);
	void hatchPrintLog(enum hatchLogLevel level,char *filename,const char *format,...);
protected:
private:
	char m_cRunLogFilePath[const_LogFilePathMaxLength];
	char m_cDevLogFilePath[const_LogFilePathMaxLength];
	char m_cMemLogFilePath[const_LogFilePathMaxLength];
	
	std::ofstream m_fRunLogFile;
	std::ofstream m_fDevLogFile;
	std::ofstream m_fMemLogFile;

	char m_cLogBuffer[const_LogStringMaxLength];

	CHatchAutoCriticalSection m_criticalSection;
};


extern CHatchLog *g_pLogFile;

#define TRACE_PRINT(_x) \
	do{\
		g_pLogFile->hatchPrintLog(_x);\
	}while(0)

#ifdef __FUNCTION__
#define LOG_PRINT(level,_x) \
	do{\
		g_pLogFile->hatchPrintLog(level,__FILE__,"%s [%d][%s]",_x,__LINE__,__FUNCTION__);\
	}while(0)

#define LOG_PRINT_iy(level,_x,_iy) \
	do{\
		g_pLogFile->hatchPrintLog(level,__FILE__,_x"[%d][%s]",_iy,__LINE__,__FUNCTION__);\
	}while(0)

#define PRO_LOG(_x) LOG_PRINT(LOGLEVEL_PROMPT,_x)
#define WAR_LOG(_x) LOG_PRINT(LOGLEVEL_WARN,_x)
#define ERR_LOG(_x) LOG_PRINT(LOGLEVEL_ERROR,_x)
#define FAI_LOG(_x) LOG_PRINT(LOGLEVEL_FAILURE,_x)

#define PRO_LOG1(_x,_iy) LOG_PRINT_iy(LOGLEVEL_PROMPT,_x,_iy)
#define WAR_LOG1(_x,_iy) LOG_PRINT_iy(LOGLEVEL_WARN,_x,_iy)
#define ERR_LOG1(_x,_iy) LOG_PRINT_iy(LOGLEVEL_ERROR,_x,_iy)
#define FAI_LOG1(_x,_iy) LOG_PRINT_iy(LOGLEVEL_FAILURE,_x,_iy)

#else

#define PRO_LOG(_x) TRACE_PRINT(_x)
#define WAR_LOG(_x) TRACE_PRINT(_x)
#define ERR_LOG(_x) TRACE_PRINT(_x)
#define FAI_LOG(_x) TRACE_PRINT(_x)

#define PRO_LOG1(_x,_iy) TRACE_PRINT(_x)
#define WAR_LOG1(_x,_iy) TRACE_PRINT(_x)
#define ERR_LOG1(_x,_iy) TRACE_PRINT(_x)
#define FAI_LOG1(_x,_iy) TRACE_PRINT(_x)

#endif

#endif