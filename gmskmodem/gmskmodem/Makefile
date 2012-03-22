all: gmskmodem 


gmskmodem: gmskmodem.c r_capture.h r_countdiff.h descramble.h dspstuff.h dvtool.h fcs.h global.h initalsa.h r_initinfile.h parsecliopts.h r_printbit.h r_processaudio.h s_rewriteheader.h s_buffer.h s_dstarheader.h s_gmskmodulate.h s_readdvtool.h s_writefile.h s_input_sal.h s_input_raw.h s_input_dstar.h s_alsaout.h s_ptt.h r_output_dal.h
	 gcc -Wall -lasound -lpthread -lrt -o gmskmodem gmskmodem.c


