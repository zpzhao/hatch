/*
 * CHatchTest.cpp
 *
 *  Created on: 2016-3-15
 *      Author: zhaozongpeng
 */

#include "CHatchTest.h"

#ifdef TEST_PRO
#include "test_md5.h"
#endif

CHatchTest::CHatchTest() {
	// TODO Auto-generated constructor stub

}

CHatchTest::~CHatchTest() {
	// TODO Auto-generated destructor stub
}

int CHatchTest::hatRun()
{

	test_md5();
	return 0;
}


