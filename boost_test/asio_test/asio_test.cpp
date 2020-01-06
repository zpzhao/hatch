/*
 * asio_test.cpp
 *
 *  Created on: 2020年1月6日
 *      Author: zpzhao
 */

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <string>
#include <iostream>

using namespace boost;
using namespace asio;
using namespace std;

/**
 * asio tcp async_resolve test
 */
void PostResolve(const boost::system::error_code& err,
        boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
{
	cout<<"PostResolve"<<endl;
}

void run_service(boost::asio::io_service *ios)
{
	ios->run();
}
void async_resolve_test()
{
	boost::asio::io_service service;
	boost::asio::ip::tcp::resolver resolver(service);


	boost::asio::ip::tcp::resolver::query query(
	      "http://cd.uxsino.com",
	      "19503");

	resolver.async_resolve(query,
	                          boost::bind(&PostResolve,
	                                      asio::placeholders::error,
	                                      asio::placeholders::iterator));

	thread *t = new thread(boost::bind(&run_service, &service));

	t->join();
	//service.run();
}

int hat_asio_test()
{
	async_resolve_test();

	return 0;
}
