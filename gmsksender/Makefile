all: gmsksender ptt_cs ptt_tx

gmsksender: gmsksender.c buffer.h dspstuff.h dstarheader.h dvtool.h fcs.h global.h gmskmodulate.h parsecliopts.h readdvtool.h rewriteheader.h writefile.h input.h alsaout.h
	 gcc -Wall -lpthread -lrt -lasound -o gmsksender gmsksender.c

ptt_cs: ptt_cs.c ptt_global.h ptt_parsecliopts.h
	gcc -Wall -o ptt_cs ptt_cs.c


ptt_tx: ptt_tx.c ptt_global.h ptt_parsecliopts.h
	gcc -Wall -o ptt_tx ptt_tx.c


