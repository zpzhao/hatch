#include <stdlib.h>
#include <stdio.h>
#include <Winsock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

int if_a_string_is_a_valid_ipv4_address(const char *str)
{
    struct in_addr stAddr;
    int ret;

    ret = inet_pton(AF_INET, str, &stAddr);

    if (ret > 0)
        printf("suc %d\n", ret);
    else if (ret < 0)
        printf("err %d\n", ret);
    else 
        printf("err %d\n", ret);

    return ret;
}

int main()
{
    char str[31],temp[31];  
    int a,b,c,d;  
    while(gets(str)!=NULL)  
    {  
        if(sscanf(str, "%d.%d.%d.%d ",&a,&b,&c,&d)==4 &&   a>=0   &&   a<255 &&   b>=0   &&   b<255 &&   c>=0   &&   c<255 &&   d>=0   &&   d<255)  
        {  
            sprintf(temp, "%d.%d.%d.%d",a,b,c,d);    //把格式化的数据写入字符串temp  
            if(strcmp(temp,str)==0)   
            {  
                printf("YES\n");   
            }   
            else  
            {  
                printf("NO\n");   
            }  
        }  
        else   
        {  
            printf("NO\n");  
        }  
    }  
    return 0;   
}  

