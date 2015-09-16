
sources=AudioRecord.c pcmmix.c

libs=-lpulse-simple -lpulse

DEBUG=-g -D_DEBUG

AudioRecord:
	gcc $(sources) $(libs) $(DEBUG) -I ../ -o AudioRecord

clean:
	-rm -f AudioRecord
