# Make file


all: audiocap_c2_key audiocap_pcm_gpio

audiocap_c2_key:  Makefile c2encap.h audiocap_c2_key.c
	gcc -O3 -Wall -o audiocap_c2_key audiocap_c2_key.c -lasound -lcodec2

audiocap_pcm_gpio:  Makefile audiocap_pcm_gpio.c
	gcc -O3 -Wall -o audiocap_pcm_gpio audiocap_pcm_gpio.c -lasound 

