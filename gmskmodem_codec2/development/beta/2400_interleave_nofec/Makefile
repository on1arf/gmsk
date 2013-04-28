# Make file

# change USEALSA to "no" to disabling ALSA and kernel-driven interrupts
# this is needed to compile on windows platforms

USEALSA = YES
#USEALSA = no

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


all: gmskmodem 


ifeq "$(USEALSA)" "YES"
gmskmodem: Makefile gmskmodem.c r_capture.h r_countdiff.h dspstuff.h global.h initalsa.h r_initinfile.h parsecliopts.h r_printbit.h r_processaudio_codec2.h s_buffer.h  s_gmskmodulate.h s_writefile.h s_input_sal.h s_input_codec2.h s_alsaout.h s_ptt.h r_output_dal.h r_filein.h c2encap.h c2_scramble.h c2_fec13.h c2_interleave.h
	 gcc -O3 -Wall -D_USEALSA -D_INTMATH=$(INTMATH) -D_USEFLOAT=$(USEFLOAT) -o gmskmodem gmskmodem.c -lasound -lpthread
else
gmskmodem: Makefile gmskmodem.c r_countdiff.h dspstuff.h global.h r_initinfile.h parsecliopts.h r_printbit.h r_processaudio_codec2.h s_buffer.h s_gmskmodulate.h s_writefile.h s_input_sal.h s_input_codec2.h s_ptt.h r_output_dal.h r_filein.h c2encap.h c2_scramble.h c2_fec13.h c2_interleave.h
	 gcc -O3 -Wall -D_INTMATH=$(INTMATH) -D_USEFLOAT=$(USEFLOAT) -o gmskmodem gmskmodem.c -lpthread
endif
