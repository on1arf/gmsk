# makefile to create gmskmodem api


# Set to 0 to use integer math
# Set to 1 to use floating point math
#USEFLOAT = 0
USEFLOAT = 1

# set to 64 to use 64 bit integer math operations
# set to 3212 to limit integer math to 32 bits, with 12 bit audio
# set to 3210 to limit integer math to 32 bits, with 10 bit audio
# this setting will be ignored if floating-point operations
# are selected (see USEFLOAT)
INTMATH = 64
#INTMATH = 3212
#INTMATH = 3210




all:	gmskmodemapi apitest_modulate apitest_demodulate

	
gmskmodemapi: Makefile gmskmodemapi.c gmskmodemapi.h a_dspstuff.h a_gmskmodulate.h  countdiff.h c2gmskchain.h c2gmskabuff.h c2gmsksess.h c2gmskprintbit.h c2gmskdebugmsg.h c2gmsksupport.h c2gmskstr.h c2gmskmodulate.h c2_interleave.h c2_scramble.h c2_fec.h 
	gcc -O2 -D_USEFLOAT=$(USEFLOAT) -D_INTMATH=$(INTMATH) -c -Wall gmskmodemapi.c

apitest_modulate: Makefile apitest_modulate.c gmskmodemapi.o
	gcc -O2 -Wall -o apitest_modulate apitest_modulate.c gmskmodemapi.o

apitest_demodulate: Makefile apitest_demodulate.c gmskmodemapi.o
	gcc -O2 -Wall -o apitest_demodulate apitest_demodulate.c gmskmodemapi.o

