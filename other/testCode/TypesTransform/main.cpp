#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define palloc malloc

#define UX_MIGRATION_TOOL_VERSION 1.2

#define UX_TYPE_NAME_LENGTH 128
#define UX_TYPES_NUM_MAX    256

typedef enum enType_Mapping_Load_Flag
{
    LOAD_FROM_CONFIG,
    LOAD_FROM_INIT
}TYPE_MAPING_LOAD_FLAG;

typedef enum enType_Transform_ERR
{
    UNKNOWN_ERR     =   0x10,
    CONFIG_FILE_ERR,
    UNIDENTIFIED_ERR,
    SYNTAX_ERR,
    OTHER_ERR
}TYPE_TRANSFORM_ERR;

typedef struct types_map
{
    int index;
    int flag;
    char otype_name[UX_TYPE_NAME_LENGTH];
    char utype_name[UX_TYPE_NAME_LENGTH];
}*PUX_TYPES_MAP_ST, UX_TYPES_MAP_ST;

static UX_TYPES_MAP_ST ux_types_map_list[UX_TYPES_NUM_MAX] = {{0,0,0,0}};



static void help(const char *progname);
int argsParser(int argc, char *argv[]);

PUX_TYPES_MAP_ST GetTypesMappingList()
{
    return ux_types_map_list;
}


int AddTypeMappingNode(char *divides,int index)
{
    char *posStart = NULL;
    char *posEnd = NULL;
    char flag[16] = {0};
    PUX_TYPES_MAP_ST pTypesMapList = GetTypesMappingList();

    posStart = divides;
    posEnd = strchr(posStart, ':');
    if((NULL == posStart) || (NULL == posEnd))
        return CONFIG_FILE_ERR;
    strncpy(pTypesMapList[index].otype_name, posStart, posEnd-posStart);

    posStart = posEnd + 1;
    posEnd = strchr(posStart, ':');
    if((NULL == posStart) || (NULL == posEnd))
        return CONFIG_FILE_ERR;
    strncpy(pTypesMapList[index].utype_name, posStart, posEnd-posStart);

    posStart = posEnd + 1;
    posEnd = strchr(posStart, '\0');
    if((NULL == posStart) || (NULL == posEnd))
        return CONFIG_FILE_ERR;

    memset(flag, 0x00, sizeof(flag));
    strncpy(flag, posStart, posEnd-posStart);
    pTypesMapList[index].flag = atoi(flag);
    
    pTypesMapList[index].index = index;

    return 0;
}

/*
 * load types mapping relation from localfile or tables
 */
int LoadTypesMappingRelation(int srcFlag, char *filePath)
{
    FILE *configFp = NULL;
    char configbuf[512] = {0};
    int ret = 0;
    int index = 0;

    if(NULL == filePath)
        return UNKNOWN_ERR;

    configFp = fopen(filePath, "r");
    if(NULL == configFp)
    {
        return CONFIG_FILE_ERR;
    }

    while((NULL != fgets(configbuf, 512, configFp)))
    {
        ret = AddTypeMappingNode(configbuf, index);
        if(0 != ret)
            return ret;

        index++;
        if(index > UX_TYPES_NUM_MAX)
            break;
    }

    return 0;
}

int ReloadTypesMappingRelation(int srcFlag, char *configFile)
{
    int ret = 0;
    memset(ux_types_map_list, 0x00, sizeof(ux_types_map_list));

    ret = LoadTypesMappingRelation(srcFlag, configFile);
    return ret;
}



/*
 * search type mapping list
 * 0 Success
 */
int GetMappingUXTypeName(char *oTypeName, char *uxTypeName, int *flag)
{
    int index = 0;
    int findflag = 0;

    PUX_TYPES_MAP_ST ux_types_map_list = GetTypesMappingList();

    if((NULL == oTypeName) || (NULL == uxTypeName))
    {
        return -1;
    }

    while(index < UX_TYPES_NUM_MAX)
    {
        if(0 == strcmp(ux_types_map_list[index].otype_name, oTypeName))
        {
            findflag = 1;
            break;
        }

        if(0 == ux_types_map_list[index].index)
        {
            index++;
            continue;
        }
        index ++;
    }

    if(0 == findflag)
    {
        return UNIDENTIFIED_ERR;
    }

    strcpy(uxTypeName, ux_types_map_list[index].utype_name);
    *flag = ux_types_map_list[index].flag;

    return 0;
}


int TransformColumn(char *column, char *uxSch, int *uxSchOffset)
{
    char *pos = column;
    char *typeEnd = NULL;
    char typeEndTag[] = {'(', ',', '\0'};
    int index = 0;
    int len = 0;
    char oType[UX_TYPE_NAME_LENGTH] = {0};
    char uxType[UX_TYPE_NAME_LENGTH] = {0};
    int ret = 0;
    int SchOffset = 0;
    int flag = 0;

    if((NULL == column) || (NULL == uxSch) || (NULL == uxSchOffset))
    {
        return -1;
    }
    //printf("column : %s\n", column);
    memset(oType, 0x00, sizeof(oType));
    memset(uxType, 0x00, sizeof(uxType));

    /* search type */
    pos = strchr(pos, ' ');
    if(NULL == pos)
    {
        return SYNTAX_ERR;
    }
    pos += 1;

    /* end tag '(', ',', '\0' */
    index = 0;
    while(index < sizeof(typeEndTag))
    {
        typeEnd = strchr(pos, typeEndTag[index]);
        if(NULL != typeEnd)
            break;

        index++;
    }

    if(NULL == typeEnd)
    {
        return SYNTAX_ERR;
    }

    strncpy(oType, pos, typeEnd - pos);
    ret = GetMappingUXTypeName(oType, uxType, &flag);
    if(0 != ret)
        return ret;

    /* replace types */

    /* column name */
    SchOffset = pos - column;
    strncpy(uxSch, column, SchOffset);

    /* column type */
    strcat(uxSch + SchOffset, uxType);
    SchOffset += strlen(uxType);

    /* () */
    strcpy(uxSch + SchOffset, typeEnd);
    SchOffset += strlen(typeEnd);

    *uxSchOffset += SchOffset;

    //printf("oType:%s\n", oType);
    return 0;
}

int TransformTypes(char *orclSch, char *uxSch, int *uxSqlSize)
{
    char *pos = orclSch;
    int blank = 0;
    char *uxpos = uxSch;
    char tmpbuf[128] = {0};
    int tmpoffset = 0;
    int ret = 0;

    int uxSchOffset = 0;

    if((NULL == orclSch) || (NULL == uxSch))
    {
        return -1;
    }

    while((*pos != '\0'))
    {
        if(*pos == ',')
        {
            ret = TransformColumn(tmpbuf, uxSch + uxSchOffset, &uxSchOffset);
            if(ret != 0)
                return ret;
            
            uxSch[uxSchOffset] = ',';
            uxSchOffset += 1;

            memset(tmpbuf, 0x00, sizeof(tmpbuf));
            tmpoffset = 0;
            pos++;
            continue;
        }

        if((*pos == '\t') || (*pos == '\n') || (*pos == '\r'))
            *pos = ' ';

        if(*pos == ' ')
            blank += 1;
        else
            blank = 0;
        pos++;

        if(blank > 1)
            continue;

        if((tmpoffset == 0) && (blank > 0))
            continue;

        tmpbuf[tmpoffset] = *(pos-1);
        tmpoffset++;
    }

    /* last column */
    if((tmpoffset != 0) && (*pos == '\0'))
    {
        ret = TransformColumn(tmpbuf, uxSch + uxSchOffset, &uxSchOffset);
        if(ret != 0)
            return ret;
    }
    *uxSqlSize += uxSchOffset;

    return 0;
}

int TransformSchSql(char *orclSql, int orclSqlSize, char **uxSql, int *uxSqlSize)
{
    char * search_end = NULL;
    char *search = NULL;
    char *fixed = NULL;
    int ret = 0;
    char *uxTmp = NULL;

    if((NULL == orclSql) || (orclSqlSize < 4))
    {
        return -1;
    }

    uxTmp = (char *)palloc(orclSqlSize*2);
    if(NULL == uxTmp)
    {
       /* ereport(ERROR,
                      (errcode(ERRCODE_TOO_MANY_COLUMNS),
                              errmsg("migration palloc err")));*/
        return -11;
    }
    *uxSql = uxTmp;
    *uxSqlSize = orclSqlSize*2;
    memset(uxTmp, 0x00, *uxSqlSize);

    /* skip sql header */
    search = strdup(orclSql);
    if(NULL == search)
    {
       /* ereport(ERROR,
                      (errcode(ERRCODE_TOO_MANY_COLUMNS),
                              errmsg("migration palloc err")));*/
        return -11;
    }
    fixed = strchr(search, '(');
    fixed += 1;
    search_end = strrchr(search, ')');
    *search_end = '\0';

    ret = TransformTypes(fixed, uxTmp, uxSqlSize);

    free(search);   
    return ret;
}


int TransformSql(char *orclSql, int orclSqlSize, char **uxSql, int *uxSqlSize)
{
    char * search_end = NULL;
    char *search = NULL;
    char *fixed = NULL;
    int ret = 0;
    char *uxTmp = NULL;

    if((NULL == orclSql) || (orclSqlSize < 4))
    {
        return -1;
    }

    uxTmp = (char *)palloc(orclSqlSize*2);
    if(NULL == uxTmp)
    {
       /* ereport(ERROR,
                      (errcode(ERRCODE_TOO_MANY_COLUMNS),
                              errmsg("migration palloc err")));*/
        return -11;
    }
    *uxSql = uxTmp;
    *uxSqlSize = orclSqlSize*2;
    memset(uxTmp, 0x00, *uxSqlSize);

    /* skip sql header */
    search = strdup(orclSql);
    if(NULL == search)
    {
       /* ereport(ERROR,
                      (errcode(ERRCODE_TOO_MANY_COLUMNS),
                              errmsg("migration palloc err")));*/
        return -11;
    }
    fixed = strchr(search, '(');
    fixed += 1;
    search_end = strrchr(search, ')');
    *search_end = '\0';

    *uxSqlSize = fixed - search;
    strncpy(uxTmp, search, *uxSqlSize);
    ret = TransformTypes(fixed, uxTmp + *uxSqlSize, uxSqlSize);

    *search_end = ')';
    strcpy(uxTmp + *uxSqlSize, search_end);

    free(search);   
    return ret;
}

char *GetUxSch(char *oSql, int oSqlLen)
{
    int ret = 0;
    char *uxSql = NULL;
    int uxLen = 0;

    ret = TransformSchSql(oSql, strlen(oSql), &uxSql, &uxLen);
    if(0 != ret)
    {
        return "There are not support types, please manual processing.";
    }

    return uxSql;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    char *uxSql = NULL;
    int uxLen = 0;
    char *oSql = "create table DEMO_TAB(  GONGNENGDYJS_ID VARCHAR2(192) not null,  JUESELSID_FK    VARCHAR2(192) not null,  JUESEID_FK      VARCHAR2(192),  GONGNENGDM      VARCHAR2(192),  YOUXIAOBZ       VARCHAR2(12),  REGNAME         VARCHAR2(192),  REGTIME         NUMBER,  MODNAME         VARCHAR2(192),  MODTIME         NUMBER,  a				CHAR(10 CHAR),  b				NCHAR);";

    /* args */
    ret = argsParser(argc, argv);

    ret = LoadTypesMappingRelation(1, "typesMapping.config");
    if(0 != ret)
        printf("Load config file err[%d]\n", ret);
    printf("oSql:%s \n", oSql);

    ret = TransformSql(oSql, strlen(oSql), &uxSql, &uxLen);
    if(ret == 0)
        printf("uxSql [%d] :%s\n", ret, uxSql);
    else
        printf("err [%d]\n", ret);

    if(NULL != uxSql)
        free(uxSql);

    return 0;
}


int argsParser(int argc, char *argv[])
{
    if (argc <= 1)
	{
        help(progname);
		exit(0);
    }

	if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-?") == 0)
	{
		help(progname);
		exit(0);
	}

	if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0)
	{
		puts("ux_migration (Uxsinodb) " UX_MIGRATION_TOOL_VERSION);
		exit(0);
	}

    return 0;
}

static void help(const char *progname)
{
	printf(_("%s is the Uxsinodb tool for migration data.\n\n"),
		   progname);
	printf(_("Usage:\n"
			 "  %s [OPTION]... \n\n"),
		   progname);
	printf(_("Options:\n"));
	printf(_("  -c             automatically generate C code from embedded SQL code;\n"
			 "                 this affects EXEC SQL TYPE\n"));
	printf(_("  -C MODE        set compatibility mode; MODE can be one of\n"
			 "                 \"INFORMIX\", \"INFORMIX_SE\"\n"));
	printf(_("  -D SYMBOL      define SYMBOL\n"));
	printf(_("  -h             parse a header file, this option includes option \"-c\"\n"));
	printf(_("  -i             parse system include files as well\n"));
	printf(_("  -I DIRECTORY   search DIRECTORY for include files\n"));
	printf(_("  -o OUTFILE     write result to OUTFILE\n"));
	printf(_("  -r OPTION      specify run-time behavior; OPTION can be:\n"
	 "                 \"no_indicator\", \"prepare\", \"questionmarks\"\n"));
	printf(_("  --regression   run in regression testing mode\n"));
	printf(_("  -t             turn on autocommit of transactions\n"));
	printf(_("  --version      output version information, then exit\n"));
	printf(_("  -?, --help     show this help, then exit\n"));
}
