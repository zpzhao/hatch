#ifndef HAT_LOG_H_
#define HAT_LOG_H_

#include <stdio.h>

class CHAT_LOG
{
public:
    CHAT_LOG();
    ~CHAT_LOG();
    
private:
};

extern CHAT_LOG theLog;
extern FILE *g_fpLog;

#define DEBUG_LOG(format, ...) fprintf(g_fpLog, format, __VA_ARGS__)

#endif