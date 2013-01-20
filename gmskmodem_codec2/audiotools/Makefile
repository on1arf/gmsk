all: audiotool

audiotool: at_audio_callback.h at_audio_convert_filter.h at_audio_convert.h at_audio_devs.h at_net_receive.h at_net_send.h at_ptt_gpio.h at_ptt_keyin.h audiotool.c Makefile
	gcc -Wall -O3 -o audiotool audiotool.c -lportaudio -lcodec2

install:
	install -o root -g root -m 755 audiotool /usr/local/bin

