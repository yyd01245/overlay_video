


LIBDIR=lib


CFLAGS= -g  -D_ENABLE_REDUCE_DECODER_  -Wno-format-security

CXXFLAGS=  ${CFLAGS} -I ${LIBDIR}/include 
#CXXFLAGS=-I libevent/include -L libevent
LIBS= ${LIBDIR}/libevent.a -lm -pthread  -lnvidia-ml
LIBA=




ifeq ($(findstring 32,${LIBDIR}),32)
LIBA+= /usr/lib/i386-linux-gnu/libbsd.a
else
LIBA+= /usr/lib/x86_64-linux-gnu/libbsd.a
endif

SRCS=$(wildcard *.cc *.c)

HDRS=asyncqueue.h colors.h logger.h Resource.h VDManager.h avshm.h avstub.h vdmHelper.h check.h list.h macros.h sysutils.h vdm.h 


OBJS=main.o VDManager.o Resource.o logger.o asyncqueue.o  cJSON/cJSON.o sysutils.o




.PHONY:all clean test tar

all:vdm fakedecoder audiodispatcher
#audioplayer audioplayer2


test:
	echo ${HDRS}
	echo ${SRCS}
	echo ${OBJS}




vdm:${OBJS} ${HDRS}
	${CXX} -o $@  $(filter %.o,$^) ${LIBS} ${LIBA} 



audiodispatcher:audiodispatcher.c
	${CC} -o $@ $^ -lpulse -lpulse-simple -pthread ${CFLAGS}

audiodispatcher2:audiodispatcher2.c
	${CC} -o $@ $^ -lpulse  -pthread ${CFLAGS}


audioplayer2:audioplayer2.c
	${CC} -o $@ $^ -lpulse  ${CFLAGS} -lm

fakedecoder:fakedecoder.c avshm.h avstub.h
	${CC} -o $@ $(filter-out %.h,$^) ${CFLAGS}  -lm -pthread


clean:
	-rm *.o
	-rm cJSON/cJSON.o
	-rm vdm
	-rm fakedecoder
	-rm audioplayer
	-rm audioplayer2






%.o:%.cc
	${CXX} -c -o $@ $< ${CXXFLAGS} -g

%.o:%.c 
	${CC} -c -o $@ $< ${CFLAGS} -g
