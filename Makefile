TOPDIR ?= $(shell pwd -P)

CROSS_COMPILE =
#CROSS_COMPILE = arm-linux-gnueabihf-

export COMPILING_TIME=$(shell date +%Y/%m/%d-%H:%M:%S)

ifeq (${CROSS_COMPILE}, )
	MACRO_DEFINE= -DX86_LINUX
	DEBUG=-g
	EXT_LIB = -lxml2 -lpthread
	CFLAGS = -Wall -D_REENTRANT -D_GNU_SOURCE -fPIC $(MACRO_DEFINE) \
		$(DEBUG)  -I/usr/include/libxml2 \
		-DCOMP_TIME="$(COMPILING_TIME)"
else
	LIBPATH = ../lib/
	EXT_LIB= -L$(LIBPATH) -lz -llzma -licuuc -licudata -lxml2 -lpthread
	CFLAGS = -Wall -D_REENTRANT -D_GNU_SOURCE -fPIC $(MACRO_DEFINE) \
		$(DEBUG) -I../lib -I../include \
		-DCOMP_TIME="$(COMPILING_TIME)"
endif

CC=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar

APP = ltopo
ALGSO = libltopo.so
ALGA = libltopo.a

#LIBS = -L. -lltopo
LIBS = -L. ./libltopo.a

ALL_TARGETS=$(ALGA) $(ALGSO) $(APP)

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@ 

OBJS = ltopo_list.o ltopo_alg.o ltopo_xml.o ltopo.o 


all: $(ALL_TARGETS)

$(ALGA):$(OBJS)
	$(AR) -rc $@ $(OBJS)

$(ALGSO):$(OBJS)
	$(CC) $^ -shared -o $@ $(EXT_LIB)

#$(APP):$(OBJS) ltopo_main.o
	#$(CC) $^ -o $@ $(EXT_LIB)

$(APP):ltopo_main.o
	$(CC) $^ -o $@ $(LIBS) $(EXT_LIB)

clean:
	-rm -f *.o *.so *.a
	-rm -f $(APP)
