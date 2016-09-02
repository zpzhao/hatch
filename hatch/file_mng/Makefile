# Makefile hatch project/file_mng

VERSION_H = 0
VERSION_L = 01

TARGET = hatch-fmng-${VERSION_H}-${VERSION_L}

topdir=.

INC = -I${topdir}/include \
	-I${topdir}/include/public/ \
	-I${topdir}/include/bin/addfile/ \

		
OBJS = ${topdir}/src/main/main.o \
	${topdir}/src/public/hat_filemng_pub.o \
	${topdir}/src/bin/addfile/hat_add_file.o \

DEFINES += -D_HAT_LINUX__

##########################################
# test programme
TINC = -I${topdir}/include/test/md5

TOBJS = ${topdir}/src/test/md5/test_md5.o

##########################################
##########################################
#INC += ${TINC}
#OBJS += ${TOBJS}
#DEFINES += -DTEST_PRO
##########################################		
		
CC = g++
# CFLAGS = -O2
CPPFLAGS = -O2 -g

CPPFLAGS += ${INC}
CPPFLAGS += ${DEFINES}

all: ${TARGET}
${TARGET} : $(OBJS) 
	$(CC) $(CPPFLAGS) $(OBJS) $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) -o $@$(X)
	
clean:
	rm -rf ${TARGET} ${OBJS}
	