/*
 * boost example startup
 * 2017/8/12 zzp uxsino 
 * zzpwcy@163.com senllang studio
 */
#include "public.h"
#include "fun_call_test.h"
#include "timer1.h"
#include <iostream>
using namespace std;

// pf_tproc g_pfproc = hat_timer1;
pf_tproc g_pfproc = hat_fun_call_test;

int main(int argc, char *argv[])
{
	cout<<"hello boost !"<<endl;

	if(NULL != g_pfproc)
	{
		g_pfproc();
	}

	cout<<"end boost world !"<<endl;
	return 0;
}


