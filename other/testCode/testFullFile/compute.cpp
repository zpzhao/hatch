#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
using namespace std;

int calculate(int len,char *expStr)
{  
    struct  
    {   
        char opdata[200];   
        int top;  
    }opstack;      //定义操作符栈  
    opstack.top = -1;  
    int i=0;    //遍历字符串的下标  
    int t=0;    //当前后缀表达式的长度  
    char ch = expStr[i];  
    while (ch!='\0')  
    {   
        switch (ch)   
        {    
        case '+':    
        case '-':     
            while (opstack.top != -1)     
            {      
                expStr[t] = opstack.opdata[opstack.top];      
                opstack.top--;      
                t++;     
            }     
            
            opstack.top++;     
            opstack.opdata[opstack.top] = ch;     
            break;    
        case '*':    
        case '/':     
            while (opstack.top != -1 && (opstack.opdata[opstack.top] =='*' || opstack.opdata[opstack.top] =='/') )     
            {      
                expStr[t] = opstack.opdata[opstack.top];      
                opstack.top--;      
                t++;     
            }     
            
            opstack.top++;     
            opstack.opdata[opstack.top] = ch;     
            break;    
        default:     
            expStr[t] = ch;     
            t++;     
            break;   
        }   
        
        i++;   
        ch = expStr[i];  
    }

    while (opstack.top != -1)        
        //将栈中所有的剩余的运算符出栈  
    {      
        expStr[t] = opstack.opdata[opstack.top];  
        opstack.top--;   
        t++;  
    }  
    
    expStr[t]='\0'; 

        struct  
        {   
            int numeric[200];   
            int top;  
        }data;  
        
        data.top = -1;  
        i=0;  
        
        ch = expStr[i];     
        
        while (ch!='\0')     
        {   
            if (ch>='0' && ch <= '9' )   
            {    
                data.top++;     
                data.numeric[data.top] = ch-'0';   
            }    
            else if('+' == ch)   
            {    
                int tmp = data.numeric[data.top-1]  + data.numeric[data.top];    
                data.top--;    
                data.numeric[data.top] = tmp;   
            }   
            else if('-' == ch)   
            {    
                int tmp = data.numeric[data.top-1]  - data.numeric[data.top];    
                data.top--;    
                data.numeric[data.top] = tmp;   
            }   
            else if('*' == ch)   
            {    
                int tmp = data.numeric[data.top-1]  * data.numeric[data.top];    
                data.top--;    
                data.numeric[data.top] = tmp;   
            }   
            else if('/' == ch)   
            {       
                if(data.numeric[data.top] == 0)    
                {     
                    printf("cannot be zero of the divide\n");     
                    return -10000;    
                }    
                int tmp = data.numeric[data.top-1] / data.numeric[data.top];    
                data.top--;    
                data.numeric[data.top] = tmp;   
            }
            i++;   
            ch = expStr[i];     
        }  
        return data.numeric[data.top]; 
}  

#include "windows.h"
#include "math.h"

 //C++ code to make task manager generate sine graph
const double SPLIT = 0.01;
const int COUNT = 200;
const double PI = 3.14159265;
const int INTERVAL = 300;

void cputest()
{
    DWORD busySpan[COUNT]; //array of busy times
    DWORD idleSpan[COUNT]; //array of idle times
    int half = INTERVAL / 2;
    double radian = 0.0;

    for(int i=0;i<COUNT;i++)
    {
        busySpan[i] = (DWORD)(half+(sin(PI*radian)*half));
        idleSpan[i] = INTERVAL - busySpan[i];
        radian += SPLIT;
    }

    DWORD startTime = 0;
    int j = 0;
    while(true)
    {
        j = j%COUNT;
        startTime = GetTickCount();
        while((GetTickCount()-startTime)<=busySpan[j])
            ;
        Sleep(idleSpan[j]);
        j++;
    }
}

#define INTERVEL2 1000
#define GRADE 30
void testcpu1()
{
    int starttime;
    int busytime;
    int sinval = 0;
    int half = INTERVAL / 2;

    while(1)
    {
        starttime = GetTickCount();
        busytime = (int)(half * sin(float((sinval) %= GRADE) / GRADE * 2 * PI)) + half;
        cout << busytime <<";"<<sinval<< endl;
        sinval++;
        while(GetTickCount() - starttime < busytime)
            ;
        Sleep(INTERVEL2 - busytime);
    }
}
void main() 
{
    /*char *pcExp = "2+9*3-5+8/2-1";
    int len = strlen(pcExp)+1;
    char *ptExp = (char*)malloc(len);
    if(NULL == ptExp)
    {
        printf("malloc err\n");
        return;
    }

    memset(ptExp, 0x00, len);
    strcpy(ptExp, pcExp);

    int result = calculate(len, ptExp);
    printf("%s = %d\n", pcExp, result);

    free(ptExp);   */

    testcpu1();

    return;
}


