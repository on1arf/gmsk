# makefile to create audiotool 



#      Copyright (C) 2013 by Kristoff Bonne, ON1ARF
#
#      This program is free software; you can redistribute it and/or modify
#      it under the terms of the GNU General Public License as published by
#      the Free Software Foundation; version 2 of the License.
#
#      This program is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#      GNU General Public License for more details.


# Version 20130331: initial release



all: audiotool c2echo findaudiodev

audiotool: at_audio_callback.h at_net_receive.h Makefile at_audio_convert_filter.h at_net_send.h audiotool.c at_audio_convert.h at_ptt_gpio.h at_audio_devs.h at_ptt_keyin.h
	gcc -Wall -O2 -o audiotool audiotool.c -lportaudio -lcodec2

findaudiodev: findaudiodev.c Makefile
	gcc -Wall -O2 -o findaudiodev findaudiodev.c -lportaudio 

c2echo: c2echo.c Makefile
	gcc -Wall -O2 -o c2echo c2echo.c

install:
	install -o root -g root -m 755 audiotool /usr/bin
	install -o root -g root -m 755 c2echo /usr/bin
	install -o root -g root -m 755 findaudiodev /usr/bin


