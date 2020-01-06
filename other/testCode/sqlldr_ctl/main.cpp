#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    FILE *fp_tables = NULL;
    FILE *fp_ctls = NULL;
    FILE *fp_sqlldr = NULL;
    char lines[32768] = {0};
    char ctl[512] = {0};
    char tablename[512] = {0};
    char colums[32768] = {0};
    int count = 0;
    char sqlldr_sh[128] = {0};
    int sh_count = 0;

    fp_tables = fopen("tables_column.log", "r");
    if(NULL == fp_tables)
    {
        printf("tables file err.\n");
        return -1;
    }

    sprintf(sqlldr_sh, "run_sqlldr%.4d.sh", sh_count);
    fp_sqlldr = fopen(sqlldr_sh, "w+");
    if(NULL == fp_sqlldr)
    {
        printf("shell %s open err\n", sqlldr_sh);
        return -2;
    }

    while(NULL != fgets(lines, sizeof(lines), fp_tables))
    {
        count ++;
        if('.' == lines[0])
        {
            strcpy(ctl, lines+2);
            strncpy(tablename,ctl, strlen(ctl) - 5);
            
            //if(strlen(tablename) < 2)
                printf("table : %s :%d \n",tablename,count);

        }
        else
        {
            // lines 
            strncpy(colums, lines, strlen(lines) - 2);
            if(strlen(tablename) < 2)
                printf("table : %s :%d \n",tablename,count);
            sprintf(ctl, "imp\\%s.ctl", tablename);
            fp_ctls = fopen(ctl, "w+");
            if(NULL == fp_ctls)
            {
                printf("%s ctl file open err.\n",ctl);
                return -1;
            }

            /*
            load data
            infile '/usr/uxdb/data/BUILDING_INFO.csv'
            into table "BUILDING_INFO"
            fields terminated by ','
            (ZZJGDM,DWMC) 
            */
            fprintf(fp_ctls, "load data\ninfile '/usr/uxdb/data/%s.csv'\ninto table \"%s\"\nfields terminated by ','\n(%s)", tablename,tablename,colums);
            fclose(fp_ctls);
            fp_ctls = NULL;

            /* sqlldr userid=cjcl/123456@BTJ control=./imp/BUILDING_INFO.ctl */
            fprintf(fp_sqlldr, "sqlldr userid=cjcl/123456@BTJ control=./imp/%s.ctl\n", tablename);

            memset(ctl, 0x00, sizeof(ctl));
            memset(tablename, 0x00, sizeof(tablename));
            memset(colums, 0x00, sizeof(colums));
        }       
        
        memset(lines, 0x00, sizeof(lines));
    }

    if(NULL != fp_tables)
        fclose(fp_tables);

    if(NULL != fp_sqlldr)
        fclose(fp_sqlldr);

    if(NULL != fp_ctls)
        fclose(fp_ctls);
    return 0;
}