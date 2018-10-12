PROJECT_ROOT = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

OBJS = dir.o CDir.o CDirConfig.o CServiceConfig.o\
		CThread.o CDIROperation.o CDIRRequestDispatcher.o\
		CServerRequest.o CBlockingQueue.o CRPCServerResponse.o CRPCServerInterface.o\
		CRPCNIOSocketServer.o CRPCServerRequestListener.o

ifeq ($(BUILD_MODE),debug)
	CPPFLAGS += -g
else ifeq ($(BUILD_MODE),run)
	CPPFLAGS += -O2
else
	$(error Build mode $(BUILD_MODE) not supported by this Makefile)
endif

# user define
CPPFLAGS += -pthread -std=c++11

all:	dir

dir:	$(OBJS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS)  -o $@ $^

%.o:	$(PROJECT_ROOT)%.cpp
	$(CXX) -c  $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

%.o:	$(PROJECT_ROOT)%.c
	$(CC) -c $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

clean:
	rm -fr dir $(OBJS)
