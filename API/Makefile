# makefile to create gmskmodem api



all:	gmskmodemapi apitest_modulate apitest_demodulate
	
gmskmodemapi: Makefile gmskmodemapi.c gmskmodemapi.h a_dspstuff.h a_gmskmodulate.h  countdiff.h c2gmskchain.h c2gmskabuff.h c2gmsksess.h c2gmskprintbit.h c2gmskdebugmsg.h c2gmsksupport.h c2gmskstr.h c2gmskmodulate.h c2_interleave.h c2_scramble.h c2_fec13.h 
	gcc -O2 -c -Wall gmskmodemapi.c

apitest_modulate: Makefile apitest_modulate.c gmskmodemapi.o
	gcc -O2 -o apitest_modulate apitest_modulate.c gmskmodemapi.o

apitest_demodulate: Makefile apitest_demodulate.c gmskmodemapi.o
	gcc -O2 -o apitest_demodulate apitest_demodulate.c gmskmodemapi.o

