#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

CHAT_LOG theLog;
FILE *g_fpLog = NULL;

CHAT_LOG::CHAT_LOG()
{
    struct tm *time_cur;
    time_t time_t_cur;
    time(&time_t_cur);
    time_cur = localtime(&time_t_cur);
    
    g_fpLog = fopen("hat_log.log", "a+");
    if(NULL != g_fpLog)
    {
        fprintf(g_fpLog, "=============hat log start %s", asctime(time_cur));
    }
}

CHAT_LOG::~CHAT_LOG()
{
    struct tm *time_cur;
    time_t time_t_cur;
    time(&time_t_cur);
    time_cur = localtime(&time_t_cur);

    if(NULL != g_fpLog)
    {
        fprintf(g_fpLog, "=============hat log end %s", asctime(time_cur));
    }

    fclose(g_fpLog);
    g_fpLog = NULL;
}