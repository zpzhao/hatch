# Makefile hatch project

VERSION_H = 0
VERSION_L = 01

TARGET = hatch-${VERSION_H}-${VERSION_L}

topdir=.

INC = -I${topdir}/include \
		-I${topdir}/include/utility/datastruct \
		-I${topdir}/include/interface/memory \
		-I${topdir}/include/utility/md5
		
OBJS = ${topdir}/src/main/main.o \
		${topdir}/src/CHatchTest.o \
		${topdir}/src/utility/datastruct/hat_list.o \
		${topdir}/src/interface/memory/hat_mem.o \
		${topdir}/src/utility/md5/md5.o

##########################################
# test programme
TINC = -I${topdir}/include/test/md5

TOBJS = ${topdir}/src/test/md5/test_md5.o

##########################################
##########################################
INC += ${TINC}
OBJS += ${TOBJS}
DEFINES += -DTEST_PRO
##########################################		
		
CC = g++
# CFLAGS = -O2
CPPFLAGS = -O2

CPPFLAGS += ${INC}
CPPFLAGS += ${DEFINES}

all: ${TARGET}
${TARGET} : $(OBJS) 
	$(CC) $(CPPFLAGS) $(OBJS) $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) -o $@$(X)
	
clean:
	rm -rf ${TARGET} ${OBJS}
	