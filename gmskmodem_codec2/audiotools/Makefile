all: audioplay audiocap_c2udp_keyin audiocap_c2udp_gpio audiocap_c2stdout_keyin audiocap_c2stdout_gpio findaudiodevice

audioplay: audioplay.c Makefile
	gcc -Wall -O3 -o audioplay audioplay.c -lportaudio -lcodec2 -lpthread

audiocap_c2udp_keyin: audiocap_c2udp_keyin.c Makefile
	gcc -Wall -O3 -o audiocap_c2udp_keyin audiocap_c2udp_keyin.c -lportaudio -lcodec2 -lpthread

audiocap_c2udp_gpio: audiocap_c2udp_gpio.c Makefile
	gcc -Wall -O3 -o audiocap_c2udp_gpio audiocap_c2udp_gpio.c -lportaudio -lcodec2 -lpthread

audiocap_c2stdout_keyin: audiocap_c2stdout_keyin.c Makefile
	gcc -Wall -O3 -o audiocap_c2stdout_keyin audiocap_c2stdout_keyin.c -lportaudio -lcodec2 -lpthread

audiocap_c2stdout_gpio: audiocap_c2stdout_gpio.c Makefile
	gcc -Wall -O3 -o audiocap_c2stdout_gpio audiocap_c2stdout_gpio.c -lportaudio -lcodec2 -lpthread

findaudiodevice:	findaudiodevice.c Makefile
	gcc -Wall -O3 -o findaudiodevice findaudiodevice.c -lportaudio


install:
	install -o root -g root -m 755 audioplay /usr/local/bin
	install -o root -g root -m 755 audiocap_c2udp_keyin /usr/local/bin
	install -o root -g root -m 755 audiocap_c2udp_gpio /usr/local/bin
	install -o root -g root -m 755 audiocap_c2stdout_keyin /usr/local/bin
	install -o root -g root -m 755 audiocap_c2stdout_gpio /usr/local/bin
	install -o root -g root -m 755 findaudiodevice /usr/local/bin

