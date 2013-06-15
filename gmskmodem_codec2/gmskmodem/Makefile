# makefile to create gmskmodem 



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


# Version 20130331: initial release
# Version 20130606: license changed to LGPL



all:	gmskmodem

gmskmodem: Makefile at_audio_convert_filter.h audioin_demod_netout.h global.h gmskmodem.c netin_mod_audioout.h paudio_devs.h at_audio_convert.h buffer.h parsecliopts.h paudio_callback.h ptt.h
	gcc -Wall -O2 -o gmskmodem gmskmodem.c -lportaudio -lc2gmsk

install:
	install -o root -g root -m 755 gmskmodem /usr/bin


