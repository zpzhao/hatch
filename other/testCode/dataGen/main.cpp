#include <stdio.h>
#include <stdlib.h>

int main()
{
    FILE *fp_src,*fp_des;
    int fileCount = 2;
    unsigned long index=0;
    char desFileName[128] = {0};
    char *pdata = NULL;
    int lineCount = 0;
    char primaryKey[20];
    char buffer[8192];

    pdata = (char*) malloc(1024*8);
    if(NULL == pdata)
    {
        printf("malloc 8k err\n");
        exit(0);
    }

    if((fp_src=fopen("data.sql","r"))==NULL)
    {
        printf("file cannot open \n");
        exit(0);
    }

    do {
        sprintf(desFileName,"tbl1_data_%d.sql", fileCount);

        if((fp_des = fopen(desFileName, "w+")) == NULL)
        {
            printf("file open %d err\n", fileCount);
            break;
        }

        fseek(fp_src,0, SEEK_SET);
        while(NULL != fgets(pdata,8*1024,fp_src))
        {
            char *ptmp = strstr(pdata,"('");
            if(NULL == ptmp)
            {
                memset(pdata, 0x00, 8*1024);
                continue;
            }

            ptmp+=2; // ('
            char *ptmp1 = strstr(ptmp,"'");
            if(NULL == ptmp)
            {
                memset(pdata, 0x00, 8*1024);
                printf("data format err.\n");
                continue;
            }
            
            memset(buffer, 0x00, sizeof(buffer));
            strcpy(buffer,ptmp1);
            *ptmp='\0';

            sprintf(primaryKey, "k%ld",  index++);
            strcat(ptmp,primaryKey);
            int len = strlen(primaryKey);
            strcat(ptmp+len,buffer);

            fputs(pdata,fp_des);
            fputs("\n",fp_des);
            memset(pdata, 0x00, 8*1024);
            printf("process %ld\n", index);
        }        

        fclose(fp_des);
    }while(--fileCount > 0);

    fclose(fp_src);
    
    return 0;
}

