#include <iostream>
using namespace std;

#include "hatchWinApp.h"

int main(int argc, char *argv[])
{
	int retCode = 0;

	g_pHatchApp = new CHatchApp();
	if(NULL == g_pHatchApp)
	{
		cout<<"Frame create failure."<<endl;
		goto ERRRET;
	}

	retCode = g_pHatchApp->hatchInitialize();
	if(retCode != 0)
	{
		cout<<"Initialize application has occured error!"<<endl;
		goto ERRRET;
	}

	retCode = g_pHatchApp->hatchMain(argc,argv);
	if(retCode != 0)
	{
		goto ERRRET;
	}

	retCode = g_pHatchApp->hatchDetroy();
	if(retCode != 0)
	{
		cout<<"Destroy application failure."<<endl;
		goto ERRRET;
	}

	cout<<"Application has runned successfully."<<endl;
	return retCode;

ERRRET:
	cout<<"Application has error,and will exit letter. retCode:"<<retCode<<endl;
	return retCode;
}