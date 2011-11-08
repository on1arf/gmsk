all: receiver


receiver: global.h dspstuff.h descramble.h fcs.h dvtool.h printbit.h countdiff.h capture.h processaudio.h
	 gcc -Wall -lasound -o receiver -lpthread -lrt receiver.c


