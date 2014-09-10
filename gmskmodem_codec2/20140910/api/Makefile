# makefile to create gmskmodem api



# Copyright (C) 2013 Kristoff Bonne ON1ARF

#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU Lesser General Public License version 2.1, as
#   published by the Free Software Foundation.  This program is
#   distributed in the hope that it will be useful, but WITHOUT ANY
#   WARRANTY; without even the implied warranty of MERCHANTABILITY or
#   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
#   License for more details.

#   You should have received a copy of the GNU Lesser General Public License
#   along with this program; if not, see <http://www.gnu.org/licenses/>.


# NOTE, due to prior copyright, not all code in this package is published
# under the LGPL License.
# "dspstuff_2400.c" and "dspstuff_4800' are licensed as follows:
# GNU General Public License ... version 2 of the license 

# version 20130122: initial release
# Version 20130314: API c2gmsk version / bitrate control + versionid codes
# Version 20130324: convert into .so shared library
# Version 20130601: added support for golay23 library (used in 2400 bps modem)


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




all:	gmskmodemapi

test:	apitest_modulate apitest_demodulate

	
gmskmodemapi: Makefile gmskmodemapi.c gmskmodemapi.h dspstuff_4800.h dspstuff_2400.h gmskmodulate.h  countdiff.h global.h c2gmskchain.h c2gmskabuff.h c2gmsksess.h c2gmskprintbit.h c2gmskdebugmsg.h c2gmsksupport.h c2gmskstr.h c2gmskmodulate.h c2_interleave.h c2_scramble.h c2_fec.h golay23.o
	gcc -O2 -fPIC -D_USEFLOAT=$(USEFLOAT) -D_INTMATH=$(INTMATH) -c -Wall gmskmodemapi.c
	gcc -shared -Wl,-soname,libc2gmsk.so.0 -o libc2gmsk.so.0.0.0 gmskmodemapi.o golay23.o

golay23.o: golay23.c golay23.h
	gcc -O2 -fPIC -c -Wall -o golay23.o golay23.c

install:
	install -o root -g root -m 755 libc2gmsk.so.0.0.0 /usr/lib
	ln -s -f /usr/lib/libc2gmsk.so.0.0.0 /usr/lib/libc2gmsk.so.0
	ln -s -f /usr/lib/libc2gmsk.so.0.0.0 /usr/lib/libc2gmsk.so
	install -o root -g root -m 755 c2gmsk.h /usr/include

apitest_modulate: Makefile apitest_modulate.c gmskmodemapi.o
	gcc -O2 -Wall -o apitest_modulate apitest_modulate.c -lc2gmsk

apitest_demodulate: Makefile apitest_demodulate.c gmskmodemapi.o
	gcc -O2 -Wall -o apitest_demodulate apitest_demodulate.c -lc2gmsk


