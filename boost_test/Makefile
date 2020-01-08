# test dfs

top_builddir=./
top_builddir_p=.

# compiler define
CC = g++
GCC = yes
AR = ar

# user configures
BOOST_HOME = /home/zpzhao/uxdb_dev/develop/uxdb/boost_1_64_0
PTHREAD_LIBS		=  -lpthread  
BOOST_LIBS		= $(BOOST_HOME)/stage/lib/libboost_system.a $(BOOST_HOME)/stage/lib/libboost_thread.a  
BOOST_INC		= -I$(BOOST_HOME)/

# configures
CPPFLAGS = -Wall -g -O0
LIBS = $(PTHREAD_LIBS) $(BOOST_LIBS)

# user files and objects
INC = -I$(top_builddir_p)/ \
	-I$(top_builddir_p)/time_test/ \
	-I$(top_builddir_p)/fun_call_test/ \
	-I$(top_builddir_p)/asio_test/ 


OBJS = main.o \
	$(top_builddir_p)/time_test/timer1.o \
	$(top_builddir_p)/fun_call_test/fun_call_test.o \
	$(top_builddir_p)/asio_test/asio_test.o 

INC += $(BOOST_INC)
CPPFLAGS += $(INC)

all: test

#test : main.o 
#	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) $(INC)  main.o -o test 
	
test: $(OBJS)
	$(CC) $(CPPFLAGS) $(OBJS) -o $@$(X) $(LIBS)

	
clean:
	rm -rf $(OBJS) test
