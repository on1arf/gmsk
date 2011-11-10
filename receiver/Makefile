all: receiver


receiver: receiver.c capture.h countdiff.h descramble.h dspstuff.h dvtool.h fcs.h global.h initalsa.h initinfile.h parsecliopts.h printbit.h processaudio.h
	 gcc -Wall -lasound -o receiver -lpthread -lrt receiver.c


