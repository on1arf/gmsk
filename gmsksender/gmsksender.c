/* sender.c */


// A program to read a .dvtool file from file, create the radio-channel
// bitpattern, modulate and play out the GMSK/D-STAR stream to a ALSA
// audio-device

// The low-level DSP and radio header decoding functions are largly
// taken from the pcrepeatercontroller project written by
// Jonathan Naylor, G4KLX
// More info: http://groups.yahoo.com/group/pcrepeatercontroller


// This program is provided both as a functional program and as an
// educational tool on how to create gmsk streams, especially when used
// for D-STAR

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


// Release information:
// version 20111213: initial release



// include files

// for stdio
#include <stdio.h>

// for exit
#include <stdlib.h>

// standard integer types
#include "stdint.h"

// for memset and memmove
#include "string.h"

// for hton
#include "arpa/inet.h"

////////// FILE INCLUDES
#include "global.h"
#include "parsecliopts.h"
#include "dvtool.h"
#include "fcs.h"
#include "rewriteheader.h"
#include "descramble.h"
#include "dspstuff.h"
#include "gmskmodulate.h"

/////////////////////////////////////////
/////////////////////////////////////////
//  MAIN APPLICATION STARTS HERE !!!  ///
/////////////////////////////////////////
/////////////////////////////////////////

int main (int argc, char ** argv) {


// local vars
int retval;
int loop;
char retmsg[RETMSGSIZE];


FILE * filein;
FILE * fileout;

// init vars
global.invert=0;

int buffer[660], buffer2[660];
/////////////////////////////
// Command line parameters parsing
//

// parse CLI options, store info in global data
// returns 0 if all OK; -1 on error, 1 on warning
retval=parsecliopts(argc,argv,retmsg);


if (retval) {
	// retval != 0: error or warning
	fprintf(stderr,"%s",retmsg);

	// retval < 0: error -> exit
	if (retval <0 ) {
		exit(-1);
	};
}; // end if


// open input file
filein=fopen(global.fnamein,"r");

if (!(filein)) {
	fprintf(stderr,"Error: could not open input file!\n");
	exit(-1);
}; // end if

// open output file
fileout=fopen(global.fnameout,"w");

if (!(fileout)) {
	fprintf(stderr,"Error: could not open output file!\n");
	exit(-1);
}; // end if


int16_t silence=0;

// we start with 5 seconds of silence (value 0)
// send 240000 samples (5 seconds at 48000 samples / second)
for (loop=0;loop<240000;loop++) {
	retval=fwrite(&silence,sizeof(int16_t),1,fileout);
}; // end for

// next 128 bit pattern containing "1010... " (which is twice what
// the specification demands, as that only requires 64 bits), followed
// by the frame syncronisation pattern "1110110 01010000"

// send with bit inversion
retval=modulateandout_octets(startsync_pattern, sizeof(startsync_pattern),fileout,retmsg,1);

if (retval) {
	// retval != 0: error or warning
	fprintf(stderr,"%s",retmsg);

	// retval < 0: error -> exit
	if (retval <0 ) {
		exit(-1);
	};
}; // end if


// read header from input file
str_dvtoolheader dvtoolheader;

retval=fread(&dvtoolheader, 10, 1, filein);

if (retval != 1) {
	fprintf(stderr,"Error: could not read dvtool header from input file!\n");
	exit(-1);
}; // end if

// check header signature
retval=memcmp(&dvtoolheader.headertext,"DVTOOL",6);

if (retval) {
	fprintf(stderr,"Error: did not find DVTOOL signature in input file!\n");
	exit(-1);
}; // end if



// read dvtool header configuration frame
str_configframe configframe;

retval=fread(&configframe, 58, 1, filein);

if (retval != 1) {
	fprintf(stderr,"Error: could not read dvtool configuration frame from input file!\n");
	exit(-1);
}; // end if

// check length
if (configframe.length != 56) {
	fprintf(stderr,"Error: incorrect length for DVTOOL configuration frame in input file. Got %d, expected 56\n",configframe.length);
	exit(-1);
}; // end if


// check header signature
retval=memcmp(&configframe.headertext,"DSVT",4);

if (retval) {
	fprintf(stderr,"Error: did not find correct signature in configuration frame!\n");
	exit(-1);
}; // end if


// check config-or-data
if (configframe.configordata != 0x10) {
	fprintf(stderr,"Error: incorrect Config-or-Data field in DVTOOL configuration frame in input file. Got %02X, expected 0x10\n",configframe.configordata);
	exit(-1);
}; // end if

// check data-or-voice
if (configframe.dataorvoice != 0x20) {
	fprintf(stderr,"Error: DVTOOL configuration frame in input file not marked as voice. Got %02X, expected 0x20\n",configframe.dataorvoice);
	exit(-1);
}; // end if

// check frame counter
if (configframe.framecounter != 0x80) {
	fprintf(stderr,"Error: DVTOOL configuration frame in input file has incorrect framecounter. Got %02X, expected 0x80\n",configframe.framecounter);
	exit(-1);
}; // end if


// rewrite header
rewriteheader(&configframe);

// encode header
// convert octets into bits
char *p;
char c;
int bitcounter;

p=(char *) &configframe.flags[0];
c=*p;
bitcounter=0;

for (loop=0;loop<328;loop++) {
	if (c & bit2octet[bitcounter]) {
		buffer[loop]=1;
	} else {
		buffer[loop]=0;
	}; // end else - if
	bitcounter++;

	if (bitcounter >= 8) {
		p++;
		c=*p;
		bitcounter=0;
	}; // end if
}; // end for

// 2 last bits are zero
buffer[328]=0; buffer[329]=0;


// FEC encoding
retval=FECencoder(buffer,buffer2);

// interleaving
interleave(buffer2,buffer);

// scramble
scramble(buffer,buffer2);

retval=modulateandout_bits(buffer2, 660,fileout,retmsg);
if (retval) {
	// retval != 0: error or warning
	fprintf(stderr,"%s",retmsg);

	// retval < 0: error -> exit
	if (retval <0 ) {
		exit(-1);
	};
}; // end if


// read rest of dvtool file, frame per frame
str_dvframe dvframe;


// init vars
int totalframecounter=0;
int framecounter=0;
int stop=0;
int endsend=0;

while (! (stop)) {
	retval=fread(&dvframe,29,1,filein);

	if (retval != 1) {
		fprintf(stderr,"Error: Unexpected end of file after frame %d\n",totalframecounter);
		stop=1;

		endsend=0;	// endsend=0: still need to send frame with first part
						// of endmarker, followed by frame with 2nd marker
		break;
	};

	// check frame

	// check length
	if (dvframe.length != 27) {
		fprintf(stderr,"Error: incorrect dvframe length for frame %d, got %d expected 27\n",totalframecounter,dvframe.length);
		stop=1;

		endsend=0;	// endsend=0: still need to send frame with first part
						// of endmarker, followed by frame with 2nd marker
		break;
	}; // end if
	

	// check signature
	if (memcmp(&dvframe.headertext,"DSVT",4)) {
		fprintf(stderr,"Error: incorrect dvframe signature for frame %d\n",totalframecounter);
		stop=1;

		endsend=0;	// endsend=0: still need to send frame with first part
						// of endmarker, followed by frame with 2nd marker
		break;
	}; // end if

	// config or data
	if (dvframe.configordata != 0x20) {
		fprintf(stderr,"Error: Config-or-data of dvframe %d not correct. Got %02X, expected 0x20\n",totalframecounter,dvframe.configordata);
		stop=1;

		endsend=0;	// endsend=0: still need to send frame with first part
						// of endmarker, followed by frame with 2nd marker
		break;
	}; // end if

	// data or voice
	if (dvframe.dataorvoice != 0x20) {
		fprintf(stderr,"Error: Data-or-voice of dvframe %d not correct. Got %02X, expected 0x20\n",totalframecounter,dvframe.configordata); 
		stop=1;

		endsend=0;	// endsend=0: still need to send frame with first part
						// of endmarker, followed by frame with 2nd marker
		break;
	}; // end if

	// check framecounter
	if ((dvframe.framecounter & 0x1f) != framecounter) {
		fprintf(stderr,"Warning: Framecounter of dvframe %d not correct. Got %d, expected %d\n",totalframecounter,(dvframe.framecounter & 0x1f),framecounter); 
		// correct framecounter
		framecounter=dvframe.framecounter & 0x1f;
	}; // end if


	if (framecounter == 0) {
		// frame 0: slow-data should contain sync-pattern (0x554d16) and no scrambling
		if (global.sync) {
		// if "resync" option is set, overwrite sync pattern
			dvframe.slowspeeddata[0] = 0x55;
			dvframe.slowspeeddata[1] = 0x4d;
			dvframe.slowspeeddata[2] = 0x16;
		}; // end if
	} else {
		// not frame 0
		if (global.zap) {
			// zap: clear slow-data part
			// rewrite with 0x000000, but also apply scrambling
			dvframe.slowspeeddata[0] = 0x70;
			dvframe.slowspeeddata[1] = 0x4f;
			dvframe.slowspeeddata[2] = 0x93;
		} else {
			// apply scrambling
			dvframe.slowspeeddata[0] ^= 0x70;
			dvframe.slowspeeddata[1] ^= 0x4f;
			dvframe.slowspeeddata[2] ^= 0x93;
		}; // end else - if
	}; // end if

	// is it the last frame (6th bit of framecounter is set)
	if (dvframe.framecounter & 0x40) {
		stop=1;

		// endsend=1 -> frame with first part of end marker send
		// need to send one frame with 2nd part 
		endsend=1;

		// replace slowspeeddata with "end" marker
		dvframe.slowspeeddata[0] = 0xfe;
		dvframe.slowspeeddata[1] = 0xaa;
		dvframe.slowspeeddata[2] = 0xaa;
	}; 


	// modulate and send, no reverse (as the .dvtool file is already reversed)
	retval=modulateandout_octets((unsigned char *) &dvframe.ambe[0], 12,fileout,retmsg,0);

	// if stop (last frame) break out of loop here;
	if (stop) {
		break;
	}; // end if

	totalframecounter++;

	framecounter++;

	// reset framecounter every 21 frames
	if (framecounter > 20) {
		framecounter=0;
	}; // end if
	
}; // end while stop


// main loop done.

// check "endsend"

if (endsend < 1) {
// endsend = 0 -> no frame with first part of endmarker yet send
// send fake frame

	// dummy frame
	dvframe.ambe[0]=0xfe; dvframe.ambe[1]=0xaa; dvframe.ambe[2]=0xaa; 
	dvframe.ambe[3]=0xfe; dvframe.ambe[4]=0xaa; dvframe.ambe[5]=0xaa; 
	dvframe.ambe[6]=0xfe; dvframe.ambe[7]=0xaa; dvframe.ambe[8]=0xaa; 

	// first part of end marker: slowdata contains 0xfeaaaa
	dvframe.slowspeeddata[0] = 0xfe;
	dvframe.slowspeeddata[1] = 0xaa;
	dvframe.slowspeeddata[2] = 0xaa;


	// modulate and send, no reverse (as the .dvtool file is already reversed)
	retval=modulateandout_octets((unsigned char *) &dvframe.ambe[0], 12,fileout,retmsg,0);

	endsend=1;
};


if (endsend <= 1) {
// send part of end marker: ambe[0] up to ambe[2] contain 0xaa135e
	dvframe.ambe[0]=0xaa; dvframe.ambe[1]=0x13; dvframe.ambe[2]=0x5e; 

// rest of frame (not specified in formal specs) contain 0xaa
	dvframe.ambe[3]=0xaa; dvframe.ambe[4]=0xaa; dvframe.ambe[5]=0xaa; 
	dvframe.ambe[6]=0xaa; dvframe.ambe[7]=0xaa; dvframe.ambe[8]=0xaa; 
}


// done: add some more silence (1 second)
for (loop=0;loop<48000;loop++) {
	retval=fwrite(&silence,sizeof(int16_t),1,fileout);
}; // end for


// done, close all files
fclose(filein);
fclose(fileout);

return(0);
}; // end main application
