/*
 * boost timer test example 
 * 2017/8/12 zzp 
 * senllang studio
 */
#include "timer1.h"
#include "public.h"
#include <iostream>
#include <boost/timer.hpp>
#include <boost/progress.hpp>
using namespace boost;
using namespace std;

// pf_tproc timer_proc = NULL;


/*
 * boost::timer
 */
int hat_timer_max_min()
{
	timer t;
	cout<<"max timespan:"<<t.elapsed_max()/3600 <<" h"<<endl;
	cout<<"min timespan:"<<t.elapsed_min() << " s"<<endl;
	return 0;

}

/*
 * boost::progress_display
 */
int hat_progress_display()
{
	progress_display pd(1000);
	for(int i=0; i < 1000; ++i)
	{
		usleep(100);
		++pd;
	}
	return 0;
}

pf_tproc timer_proc =hat_progress_display ;

int hat_timer1()
{
	int ret = 0;
	if(NULL != timer_proc)
		ret = timer_proc();
	return ret;
}
