/*
 * dir.cpp
 *
 *  Created on: 2018年9月4日
 *      Author: zpzhao
 */

#include "CDIRRequestDispatcher.h"
#include "CDir.h"
#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
	std::cout<<"dir start running ..."<<std::endl;

	/*
	 * configure file must be input.
	 */
	std::string configfile("test.configure");
	if(argc > 1)
	{
		configfile.assign(argv[1]);
	}

	std::cout<<"start configure "<<configfile<<std::endl;

	/*
	 * load configurefile ,parse configure parameters.babudb configure so do it.
	 */
	//TODO:

	/*
	 * start dispatch thread
	 */
	//TODO:
	CDIRRequestDispatcher *dirDisp = new CDIRRequestDispatcher();
	dirDisp->startup();
	dirDisp->join();

	delete dirDisp;

	return 0;
}


