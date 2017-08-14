/*
 * boost timer test example 
 * 2017/8/12 zzp 
 * senllang studio
 */
#include "fun_call_test.h"
#include "public.h"
#include <iostream>
#include <typeinfo>
// #include <boost/utility/result_of.hpp>
// using namespace boost;
using namespace std;

// pf_tproc funcall_proc = NULL;


class Cfather
{
public:
	void echo() { cout<<"father"<<endl; }
		
};


class Cson:public Cfather
{
public:
	void echo() { cout<<"son "<<endl;  }
};


class CSharpe
{
public:
	virtual void show() { cout<<"no sharpe"<<endl; };
};

class CSqure:public CSharpe
{
public:
	virtual void show() { cout<<"squre " <<endl; }
};

template <typename T>
T getpSqure(T t1)
{
	return t1;	
}

/*
 * boost::result_of
 */
int hat_result_of()
{
	Cfather fc, *pfc;
	Cson sc, *psc;
	CSharpe sharpe, *psharpe;
	CSqure	squre, *psqure;

	
//	result_of<getpsqure(int)>::type x = getpSqure(5);
	//cout<<"typeid(x)"<<typeid(x).name()<<endl;
	cout<<"typeid(fc)"<<typeid(fc).name()<<endl;
	pfc=&sc;
	cout<<"typeid(pfc)"<<typeid(pfc).name()<<endl;
	psharpe = &squre;
	cout<<"typeid(psharpe)"<<typeid(psharpe).name()<<endl;
	
	return 0;

}

pf_tproc funcall_proc =  hat_result_of;

/*
 * boost::progress_display
 */
int hat_progress_displa1y()
{
/*	progress_display pd(1000);
	for(int i=0; i < 1000; ++i)
	{
		usleep(100);
		++pd;
	}
*/	return 0;
}

//pf_tproc funcall_proc =hat_progress_display ;

int hat_fun_call_test()
{
	int ret = 0;
	if(NULL != funcall_proc)
		ret = funcall_proc();
	return ret;
}
