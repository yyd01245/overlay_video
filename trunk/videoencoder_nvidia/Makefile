files=videoencoder.cpp 

encnvdir=encodec_nv
encnvso=${encnvdir}/libencodenv.so
encnvsrc=${encnvdir}/encodenv.c


LIBS=-lpthread -ldl -lippccem64t 

DEBUG=-g -D_DEBUG 

DATE=$(shell date +%Y%m%d)


CWD=$(shell pwd)
DIR=$(notdir ${CWD})
TARNAME=$(notdir $(shell pwd))-${DATE}.tar.gz


.PHONY:all install uninstall clean tar


all:libvideoencoder.so ${encnvso}
	@echo Make "<"$^">"
#	@echo ${TARNAME}     ${DIR}

${encnvso}:${encnvsrc}
	(cd ${encnvdir} && make encodenv && sudo make install)



libvideoencoder.so: $(files) 
	g++ -shared -fPIC $< ${LIBS} ${DEBUG} -o libvideoencoder.so

install:libvideoencoder.so
	cp *.so /usr/local/lib/
	cp ${encnvdir}/*.so /usr/local/lib/
	cp videoencoder.h /usr/local/include/
	
#	-strip -x libvideoencoder.so
	
uninstall:
	rm -rf /usr/local/lib/libvideoencoder.so
	rm -rf /usr/local/lib/libencodenv.so
	rm -rf /usr/local/lib/libformats.so
	rm -rf /usr/local/include/videoencoder.h
	
tar:
	@echo TAR ${TARNAME}
	tar cf ../${TARNAME} ../${DIR}


clean:
	-rm -f videoencoder
	-rm -f libvideoencoder.so
	(cd ${encnvdir} && make clean all)

