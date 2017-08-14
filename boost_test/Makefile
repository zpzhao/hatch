# test dfs

top_builddir=./
top_builddir_p=.

# compiler define
CC = g++
GCC = yes
AR = ar

# user configures
PTHREAD_LIBS		=  -lpthread  
BOOST_LIBS		= -L$(top_builddir)../../boost_1_59_0/stage/lib/
BOOST_INC		= -I$(top_builddir)../../boost_1_59_0/boost/

# configures
CPPFLAGS = -Wall -g -O0
LIBS = $(PTHREAD_LIBS) $(BOOST_LIBS)

# user files and objects
INC = -I$(top_builddir_p)/ \
	-I$(top_builddir_p)/time_test/ \
	-I$(top_builddir_p)/fun_call_test/ 


OBJS = main.o \
	$(top_builddir_p)/time_test/timer1.o \
	$(top_builddir_p)/fun_call_test/fun_call_test.o 

INC += $(BOOST_INC)
CPPFLAGS += $(INC) $(LIBS)

all: test

#test : main.o 
#	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) $(INC)  main.o -o test 
	
test: $(OBJS)
	$(CC) $(CPPFLAGS) $(OBJS) -o $@$(X) 

	
clean:
	rm -rf $(OBJS) test
