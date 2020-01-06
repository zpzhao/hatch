#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"

#pragma warning(disable : 4996)

#define LINE_BUF_SIZE 8192
#define STACK_LEVEL_SIZE 32
#define TMP_BUF_SIZE 512

char g_line_buf[LINE_BUF_SIZE] = {0};
char g_tmp_buf[TMP_BUF_SIZE] = {0};

const char *g_src_file_name = "sqlite3_1.c";
const char *g_des_file_name = "sqlite3.c";

FILE *g_fpSrc = NULL;
FILE *g_fpDes = NULL;

char *g_file_name_stack[STACK_LEVEL_SIZE] = {0};
int g_ifile_name_stack_top_index = 0;

int PushStack(const char *pDesFileName)
{
    if(NULL == pDesFileName)
    {
        return -1;
    }

    int len = strlen(pDesFileName);
    char *pDes = (char *)malloc((len + 1) * sizeof(char));
    if(NULL == pDes)
    {
        return -1;
    }

    if(STACK_LEVEL_SIZE <= g_ifile_name_stack_top_index)
    {
        return -2;
    }

    memset(pDes, 0x00, len + 1);
    strcpy(pDes, pDesFileName);
    g_file_name_stack[g_ifile_name_stack_top_index++] = pDes;

    //DEBUG_LOG("Push %d %s\n", g_ifile_name_stack_top_index - 1, g_file_name_stack[g_ifile_name_stack_top_index - 1]);
    return 0;
}

int PopStack(char **pDesFileName)
{
    if(0 >= g_ifile_name_stack_top_index)
    {
        return -1;
    }

    g_ifile_name_stack_top_index -= 1;
    if(NULL == g_file_name_stack[g_ifile_name_stack_top_index])
    {
        return -1;
    }

    memset(g_tmp_buf, 0x00, TMP_BUF_SIZE);
    strncpy(g_tmp_buf, g_file_name_stack[g_ifile_name_stack_top_index], TMP_BUF_SIZE);

    *pDesFileName = g_tmp_buf;
    free(g_file_name_stack[g_ifile_name_stack_top_index]);
    g_file_name_stack[g_ifile_name_stack_top_index] = NULL;

    //DEBUG_LOG("Pop %d %s\n", g_ifile_name_stack_top_index, g_tmp_buf);
    return 0;
}

int OpenDesFile(const char *pDesFileName)
{
    if(NULL == pDesFileName)
    {
        return -1;
    }

    if(NULL != g_fpDes)
    {
        fclose(g_fpDes);
        g_fpDes = NULL;
    }

    g_fpDes = fopen(pDesFileName, "a+");
    if(NULL == g_fpDes)
    {
        return -1;
    }

    printf("open %s\n", pDesFileName);
    return 0;
}

void PrintStack()
{
    printf("stack [%d]\n", g_ifile_name_stack_top_index);
    DEBUG_LOG("stack [%d]\n", g_ifile_name_stack_top_index);
    for(int i = 0; i < g_ifile_name_stack_top_index; i++)
    {
        if(NULL != g_file_name_stack[g_ifile_name_stack_top_index])
        {
            printf("stack[%d][%s]\n", i, g_file_name_stack[g_ifile_name_stack_top_index]);
            DEBUG_LOG("stack[%d][%s]\n", i, g_file_name_stack[g_ifile_name_stack_top_index]);
        }
    }
}

//
char* GetFileName(char *pSrc)
{
    char *pTmp = pSrc;
    char *pSuffix = NULL;
    char *pEndTmp = NULL;

    memset(g_tmp_buf, 0x00, sizeof(g_tmp_buf));

    // 查找位置,取最后的文件名
    pEndTmp = strrchr(pTmp, '.');
    if(NULL == pEndTmp)
    {
        return NULL;
    }

    int len = strlen(pEndTmp);
    if(2 > len)
    {
        return NULL;
    }

    len = pEndTmp - pTmp;
    int offset = 0;
    if(len >= TMP_BUF_SIZE)
    {
        offset = len - TMP_BUF_SIZE;
    }

    strncpy(g_tmp_buf, pTmp+offset + 1, len);

    char *pBeginTmp = strrchr(g_tmp_buf, ' ');
    if(NULL == pBeginTmp)
    {
        return NULL;
    }
    
    pTmp = g_tmp_buf;
    pBeginTmp++;
    while((*pTmp++ = *pBeginTmp++) && ('\0' != *pBeginTmp));
    *pEndTmp++; // .
    *pTmp++ = *pEndTmp++; // h /c 
    *pTmp = '\0';

    return g_tmp_buf;
}

int main(int argc, char *argv[])
{
    g_fpSrc = fopen(g_src_file_name, "r");
    if(NULL == g_fpSrc)
    {
        goto ERR_END;
    }

    if(0 != OpenDesFile(g_des_file_name))
    {
        printf("OpenDesFile err 1\n");
        goto ERR_END;
    }
    PushStack(g_des_file_name);

    int file_end = 0;
    int line_num = 0;
    while(NULL != fgets(g_line_buf, LINE_BUF_SIZE, g_fpSrc))
    {
        line_num++;
        //PrintStack();
        if(file_end)
        {
            file_end = 0;
            char *pFileEndName = NULL;
            if(0 != PopStack(&pFileEndName))
            {
                printf("PopStack empty err.\n");
                goto ERR_END;
            }

            if(0 != OpenDesFile(pFileEndName))
            {
                printf("open err %s\n", pFileEndName);
                goto ERR_END;
            }

            PushStack(pFileEndName);
        }

        do 
        {
            char *pBeginFile = strstr(g_line_buf, "/************** Begin file");
            if(NULL != pBeginFile)
            {
                DEBUG_LOG("%s\n", g_line_buf);
                char *pFileName = GetFileName(g_line_buf);
                if(NULL != pFileName)
                {
                    PushStack(pFileName);
                    if(0 != OpenDesFile(pFileName))
                    {
                        printf("OpenDesFile err %s\n", pFileName);
                        goto ERR_END;
                    }
                    break;
                }
            }

            char *pEndFile = strstr(g_line_buf, "/************** End of");
            if(NULL != pEndFile)
            {
                DEBUG_LOG("%s\n", g_line_buf);
                char *pFileName = GetFileName(g_line_buf);
                if(NULL != pFileName)
                {
                    char *pFileEndName = NULL;
                    if((0 != PopStack(&pFileEndName)) && (0 != strcmp(pFileName, pFileEndName)))
                    {
                        printf("stack err:%s\n", pFileName);
                        goto ERR_END;
                    }
                    file_end = 1;
                }
            }
        } while (0);

        fputs(g_line_buf, g_fpDes);
    }
ERR_END:
    DEBUG_LOG("err line:%d %s\n", line_num, g_line_buf);
    if(NULL != g_fpDes)
    {
        fclose(g_fpDes);
    }
    if(NULL != g_fpSrc)
    {
        fclose(g_fpSrc);
    }
    return 0;
}