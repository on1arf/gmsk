all: receiver receiver-dual


receiver: receiver.c capture.h countdiff.h descramble.h dspstuff.h dvtool.h fcs.h global.h initalsa.h initinfile.h parsecliopts.h printbit.h processaudio.h
	 gcc -Wall -lasound -o receiver -lpthread -lrt receiver.c


receiver-dual: receiver-dual.c capture-dual.h countdiff.h descramble.h dspstuff-dual.h dvtool.h fcs.h global-dual.h initalsa.h initinfile.h parsecliopts.h printbit.h processaudio-dual.h
	 gcc -Wall -lasound -o receiver-dual -lpthread -lrt receiver-dual.c

