#MAKEFILE
MAINDIR = $(ROOT_DIR)/main
SOCKETDIR = $(ROOT_DIR)/socket
COMMONDIR=$(ROOT_DIR)/common
CONFIGDIR=$(ROOT_DIR)/config
ENCRYPTDIR=$(ROOT_DIR)/encrypt
ANALYSISDIR=$(ROOT_DIR)/analysis
PROTOCOLDIR=$(ROOT_DIR)/protocol
SERIVICEDIR=$(ROOT_DIR)/service
THIRDPARTYDIR=$(ROOT_DIR)/thirdparty
WATCHDOGDIR=$(ROOT_DIR)/watchdog
ROOT_DIR=$(shell pwd)

INCDIR = inc
SRCDIR = src

DEFS = -DDB_TYPE_USED=1 -DSQLITE_OMIT_LOAD_EXTENSION -D_GUN_LINUX_

CFLAGS += -I $(MAINDIR)/$(INCDIR)/ -I $(COMMONDIR)/$(INCDIR)/ -I $(CONFIGDIR)/$(INCDIR)/ -I $(ENCRYPTDIR)/$(INCDIR)/ \
-I $(SOCKETDIR)/$(INCDIR)/ -I $(ANALYSISDIR)/$(INCDIR)/ -I $(PROTOCOLDIR)/$(INCDIR)/ \
-I $(SERIVICEDIR)/$(INCDIR)/ -I $(THIRDPARTYDIR)/$(INCDIR)/ -I $(WATCHDOGDIR)/$(INCDIR)/ 

CFLAGS += $(DEFS)

LDFLAGS += -lssl -lcrypto -pthread -lm
LDFLAGS += $(DFSLIBS)

#ver = debug
ifeq ($(ver), debug)
	COMFLAGS = -fpic -c -g -O0 -Wall -DUX_DEBUG 
else
	COMFLAGS = -fpic -c -O2 -Wall
endif

SOCOMFLAGS = -shared

CC=gcc

SUBDIRS=main socket common config encrypt analysis protocol service thirdparty watchdog
BIN=UxdbLicense

OBJS_DIR=debug/obj

BIN_DIR=debug/bin

CUR_SOURCE=${wildcard *.c}

CUR_OBJS=${patsubst %.c, %.o, $(CUR_SOURCE)}

export CC BIN OBJS_DIR BIN_DIR ROOT_DIR CFLAGS LDFLAGS COMFLAGS SOCOMFLAGS

all:$(SUBDIRS) $(CUR_OBJS) DEBUG

$(SUBDIRS):ECHO
	make -C $@
DEBUG:ECHO
	mkdir -p debug/bin
	make -C debug
ECHO:
	@echo $(SUBDIRS)

$(CUR_OBJS):%.o:%.c
	$(CC) -c $^ -o $(ROOT_DIR)/$(OBJS_DIR)/$@
.PHONY:clean
clean:  	
	@rm $(OBJS_DIR)/*.o
	@rm -rf $(BIN_DIR)
