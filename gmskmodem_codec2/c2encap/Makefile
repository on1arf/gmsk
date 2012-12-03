# Make file


all: c2enc_add c2enc_strip

c2enc_add:  Makefile c2encap.h c2enc_add.c
	gcc -O3 -Wall -o c2enc_add c2enc_add.c

c2enc_strip:  Makefile c2encap.h c2enc_strip.c
	gcc -O3 -Wall -o c2enc_strip c2enc_strip.c

install:
	install -o root -g root -m 644 c2encap.h /usr/local/include
	install -o root -g root -m 755 c2enc_add /usr/local/bin
	install -o root -g root -m 755 c2enc_strip /usr/local/bin

