
CC=gcc

SRCS=tencodenv.c encodenv.c bgra2nv12.c gpu_prober.c #formats.c 

OBJS=$(SRCS:.c=.o) 


######################
##MACROS
######################


#-D_DEBUG_
# Enable more output;
# Ctrl-C give profile informa-lippccem64ttion (e.g. Encode Speed Statistic)
LIBS=-lcuda -lnvidia-encode  -pthread -lcudart -lippccem64t -lnvidia-ml

#CFLAGS=-I /usr/local/cuda/include -L /usr/local/cuda/lib64 -g   -D_DEBUG_ -DCUDA_CONVERT -DMAPPED_ONLY #-D_TEST_
CFLAGS=-I /usr/local/cuda/include -L /usr/local/cuda/lib64 -L. -O2 -fPIC  

DATE=$(shell date +%m-%d)




.PHONY:all codecnv encodenv decodenv digest clean cleanall install


all: encodenv
#encodenv

tdec:decodenv.c
	gcc decodenv.c -I /usr/local/cuda/include/  -L /usr/local/cuda/lib -lnvcuvid -lcuda -o decodenv -lbsd


codecnv:libencodenv.so libdecodenv.so

encodenv:libencodenv.so

decodenv:libdecodenv.so


libdecodenv.so:decodenv.c
	${CC} -o $@ -fPIC -shared $^

#libformats.so
libencodenv.so:$(filter-out t%.c,${SRCS}) 
	${CC} -fPIC -shared -o $@ ${CFLAGS}  $^ ${LIBS} 



tencodenv:tencodenv.c
	${CC} -g -o $@ -Wl,--rpath=./ -L. $^ -lformats -lencodenv -lippccem64t 


#gcc decodenv.c -I /usr/local/cuda/include/  -L /usr/local/cuda/lib -lnvcuvid -lcuda -o decodenv


%.o:%.c
	${CC} -c -o $@ ${CFLAGS}  $< ${LIBS}


%.o:%.cu
	${NVCC} -c -o $@ ${CUFLAGS}  $< ${CULIBS}

lib%.a:%.cu
	${NVCC} -lib -o $@ ${CUFLAGS}  $< ${CULIBS}

lib%.so:%.cu
	${NVCC} -shared -Xcompiler -fPIC -o $@ ${CUFLAGS}  $< ${CULIBS}

tar:
	tar -jcf ../encodec_nv-${DATE}.tar.bz2 ../encodec_nv/


install:
	sudo cp *.so /usr/local/lib


clean:
	-rm *.o *.a *.so
	-rm tencodenv

cleanall: clean
	-rm *.264





