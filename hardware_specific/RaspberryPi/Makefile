# Make file for gmskmodem tools for raspberry Pi

all: ptt_gpio_raspi

ptt_gpio_raspi: ptt_gpio_raspi.c
	gcc -o ptt_gpio_raspi ptt_gpio_raspi.c -lpthread -lbcm2835

# install:	ptt_gpio_raspi needs setuid priviledge to access GPIO
#				pins of raspberry pi
install:
	install -o root -g root -m 4755 ptt_gpio_raspi /usr/bin
