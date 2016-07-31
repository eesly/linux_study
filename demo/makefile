DEBUG=0
STATIC=0
CC=gcc
CXX=g++

CPPFLAGS+=-fPIC
LDFLAGS+=-fPIC
LIBS+=-Wl,--as-needed -lpthread -lrt -ldl
INCPATH+=-I. -I.. -I../common/
ifeq ($(DEBUG),1)
	CPPFLAGS+= -g3 -DDEBUG
	LDFLAGS+= -g3 -DDEBUG
else
	CPPFLAGS+= -O3 -Os -DNDEBUG
	LDFLAGS+= -O3 -Os -DNDEBUG
endif

ifeq ($(STATIC),1)
	LIBS+=-static-libgcc -static-libstdc++
endif

COMMON_OBJS=../common/CThread.o

DEMO_DAEMON=DemoForDaemon.o
DEMO_TLS=DemoForTLS.o

ALL_OBJS=$(COMMON_OBJS) $(DEMO_DAEMON) $(DEMO_TLS)

.PHONY:all
all: demo_daemon demo_tls

demo_daemon: $(DEMO_DAEMON)
	$(CXX) $(CPPFLAGS) -o demo_daemon $^ $(LIBS)

demo_tls: $(COMMON_OBJS) $(DEMO_TLS) 
	$(CXX) $(CPPFLAGS) -o demo_tls $^ $(LIBS)

.PHONY:clean
clean:
	rm -f demo_* $(ALL_OBJS)
