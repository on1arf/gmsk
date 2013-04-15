/* global.h */


// global data + defines, some other vars

// version 20130326 initial release
// version 20130404: added half duplex support


/*
 *      Copyright (C) 2011 by Kristoff Bonne, ON1ARF
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; version 2 of the License.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 */


// DEFINEs
// GLOBAL PARAMETERS

// options
#define DISABLE_AUDIOLEVELCHECK 1 // set to "1" to disable audio level check, else set to "0"
#define NUMBUFF 30 // number of audio buffers 30 * 40 ms = 1.2 sec

#define DEFAULT_SILENCEBEGIN 20 // 0.2 seconds silence at beginning
#define DEFAULT_SILENCEEND 50 // 1 second silence at end

// change to 1 for strict c2encap streaming protocol checking
#define C2ENCAP_STRICT 0

// transmit timeout. How long keep PTT active when stream stops but no data received
// standard = 250 (= 250 * 40 ms = 10 sec)
#define TRANSMIT_SILENCE_TIMEOUT 50

// end options. Do not change anything below unless you know what
// you are doing
/////////////////////////////////////////////////////////////////

#define PARSECLIRETMSGSIZE 600	// maximum size of return message of parsecli

#define PERIOD 960 // 20 ms of audio at 48Khz sampling
#define RATE 48000

#define PTTGPIO_INITSTATE 0 // PTT_GPIO INITIAL STATE
#define MINBUFF 4 // minimum number of buffers to be received before processing

#define FOREVER 1


// create C2ENCAP_STICT if not defined
// set to "0"
#ifndef C2ENCAP_STRICT
	#define C2ENCAP_STRICT 1
#endif


// global data structure definition for RECEIVER
struct globaldata {
	char *ringbuffer_s[NUMBUFF];
	char *ringbuffer_r[NUMBUFF];
	int pntr_r_read, pntr_r_write, pntr_s_read, pntr_s_write;
	char * udpout_host;
	int udpout_port;
	int udpin_port;

	int dumpstream;
	int dumpaverage;

	int ipv4only;
	int ipv6only;

	// portaudio device parameters
	char * adevice;
	int exact;
	int forcestereo_r, forcestereo_s;
	PaStreamParameters inputParameters, outputParameters;
	int audioinvert_s; // send via cli
	int audioinvert_r; // set by receiver
	int halfduplex; // half duplex audio

	// PTT switching        
	char * pttcsdevice;
	char * ptttxdevice;
	char * pttlockfile;
	int ptt_invert;
	int ptt_gpiopin;
	int receivefromnet; // set if stream is being received
	int transmitptt;

	// id file and frequency
	char * idfile;
	int idfreq;


	// flags from CLI options to application
	int silencebegin;
	int silenceend;

	// c2gmsk API
	struct c2gmsk_session * c2gmsksessid;


	// other data
	int callbackcount;
	int verboselevel;
	int networksendready;
};

// END GLOBAL STRUCTURES


