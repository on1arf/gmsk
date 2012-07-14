# Make file

# change USEALSA to "no" to disabling ALSA and kernel-driven interrupts
# this is needed to compile on windows platforms

USEALSA = YES
#USEALSA = no

# Set to 0 to use integer math
# Set to 1 to use floating point math
USEFLOAT = 1




all: gmskmodem 


ifeq "$(USEALSA)" "YES" 
gmskmodem: Makefile gmskmodem.c c2_interleave.h c2_scramble.h r_capture.h r_countdiff.h descramble.h dspstuff.h dvtool.h fcs.h global.h initalsa.h r_initinfile.h parsecliopts.h r_printbit.h c2_fec13.h r_processaudio_dstar.h r_processaudio_raw.h r_processaudio_codec2.h s_rewriteheader.h s_buffer.h s_dstarheader.h s_gmskmodulate.h s_readdvtool.h s_writefile.h s_input_sal.h s_input_raw.h s_input_dstar.h s_input_codec2.h s_alsaout.h s_ptt.h r_output_dal.h r_filein.h
	 gcc -O3 -Wall -D_USEALSA -D_USEFLOAT=$(USEFLOAT) -o gmskmodem gmskmodem.c -lasound -lpthread -lrt
else
gmskmodem: Makefile gmskmodem.c c2_interleave.h c2_scramble.h r_countdiff.h descramble.h dspstuff.h dvtool.h fcs.h global.h r_initinfile.h parsecliopts.h r_printbit.h c2_fec13.h r_processaudio_dstar.h r_processaudio_raw.h r_processaudio_codec2.h s_rewriteheader.h s_buffer.h s_dstarheader.h s_gmskmodulate.h s_readdvtool.h s_writefile.h s_input_sal.h s_input_raw.h s_input_dstar.h s_input_codec2.h s_ptt.h r_output_dal.h r_filein.h
	 gcc -O3 -Wall -D_USEFLOAT=$(USEFLOAT) -lpthread -o gmskmodem gmskmodem.c
endif
