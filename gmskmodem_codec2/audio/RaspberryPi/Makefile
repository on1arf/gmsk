# Make file


all: audiocap_pcm_gpio_raspi

audiocap_pcm_gpio_raspi: Makefile audiocap_pcm_gpio_raspi.c
	gcc -O3 -Wall -o audiocap_pcm_gpio_raspi audiocap_pcm_gpio_raspi.c -lasound -lbcm2835

# install:	audiocap_*_gpio_raspi needs setuid priviledge to access GPIO
#				pins of raspberry pi

install:
	install -o root -g root -m 6755 audiocap_pcm_gpio_raspi /usr/local/bin
