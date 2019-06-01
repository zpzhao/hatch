%{
#include<stdio.h>
%}

%%
[0-0]+ printf("NUMBER\n");
[a-zA-Z][a-zA-Z0-9]* printf("word\n");
%%

int main()
{
   yylex();
   return 0;
}

int yywrap()//此函数必须由用户提供,或者声明 %option noyywrap
{
    return 1;
}

