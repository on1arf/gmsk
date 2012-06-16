# Make file

# change USEALSA to "no" to disabling ALSA and kernel-driven interrupts
# this is needed to compile on windows platforms

USEALSA = YES
#USEALSA = no



all: gmskmodem 


ifeq "$(USEALSA)" "YES" 
gmskmodem: Makefile gmskmodem.c r_capture.h r_countdiff.h descramble.h dspstuff.h dvtool.h fcs.h global.h initalsa.h r_initinfile.h parsecliopts.h r_printbit.h fec13.h r_processaudio_dstar.h r_processaudio_raw.h r_processaudio_codec2.h s_rewriteheader.h s_buffer.h s_dstarheader.h s_gmskmodulate.h s_readdvtool.h s_writefile.h s_input_sal.h s_input_raw.h s_input_dstar.h s_input_codec2.h s_alsaout.h s_ptt.h r_output_dal.h r_filein.h
	 gcc -Wall -D_USEALSA -o gmskmodem gmskmodem.c -lasound -lpthread -lrt
else
gmskmodem: Makefile gmskmodem.c r_countdiff.h descramble.h dspstuff.h dvtool.h fcs.h global.h r_initinfile.h parsecliopts.h r_printbit.h fec13.h r_processaudio_dstar.h r_processaudio_raw.h r_processaudio_codec2.h s_rewriteheader.h s_buffer.h s_dstarheader.h s_gmskmodulate.h s_readdvtool.h s_writefile.h s_input_sal.h s_input_raw.h s_input_dstar.h s_input_codec2.h s_ptt.h r_output_dal.h r_filein.h
	 gcc -Wall -lpthread -o gmskmodem gmskmodem.c
endif
