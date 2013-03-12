//////////////////////
// API version of the GMSK modem for 10m / VHF / UHF communication
// using codec2
// version 0 (versionid 0x111111): 4800 bps, 1/3 repetition code FEC


// The low-level DSP functions are largly taken from the
// pcrepeatercontroller project written by
// Jonathan Naylor, G4KLX
// More info: http://groups.yahoo.com/group/pcrepeatercontroller


/*
 *      Copyright (C) 2013 by Kristoff Bonne, ON1ARF
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


// API test - modulate
// test application for modulation chain

// Release information
// version 20130310 initial release



// API TEST APPLICATION:
// take raw codec2 input from file
// start modulation process
// send codec2 data to modulation chain, return n samples of 40 ms audio
// save to file


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <errno.h>

#include "gmskmodemapi.h"


///////////////////////
// application starts here

int main () {
session * sessid=NULL;
msgchain * chain=NULL, ** pchain;
c2gmsk_params *params;
int16_t silence[480]; // 480 samples = 10 ms @ 48Khz samplerate
int16_t pcmbuffer[1920];
int tod, numsample, msgnr;
int framecount;

int fout, fin; // file out and file in 

c2gmsk_msg * msg;

int ret;
int loop;

int nsample;

unsigned char inbuffer[7];


// fill silence will all zero
memset(&silence,0,sizeof(silence));

// open out file
fout=open("test.raw",O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

if (fout < 0) {
	fprintf(stderr,"Error: could not open file for output: %d (%s)! \n",errno,strerror(errno));
	exit(-1);
}; // end if


// open input file
fin=open("c2in.c2",O_RDONLY);

if (fin < 0) {
	fprintf(stderr,"Error: could not open file for input: %d (%s)! \n",errno,strerror(errno));
	exit(-1);
}; // end if

// pchain points to location of chain
pchain=&chain;


// before creating the session, parameters need to be created
params=c2gmsk_params_init();
if (!params) {
	fprintf(stderr,"Error: Could not create parameters structure! \n");
	exit(-1);
}; // end if


// init session
sessid=c2gmsk_sess_new(params,&ret);
if (ret != C2GMSK_RET_OK) {
	fprintf(stderr,"Error: could not create C2GMSK session: %d \n",ret);
	exit(-1);
}; // end if


// create 300 ms of silence ( 30 times 10 ms)
for (loop=0; loop<30; loop++){
	ret=write(fout,silence,sizeof(silence));
}; // end for


//////////////////////////////////////////////////////////////////////
///////////////////// MOD START //////////////////////////////////
//////////////////////////////////////////////////////////////

// do mod start
// chain is returned via pointer pchain
// chain search parameters will automatically be reinitialised
// version = 0
ret=c2gmsk_mod_start(sessid,0,pchain);
chain=*pchain;

if (ret != C2GMSK_RET_OK) {
	fprintf(stderr,"Error: c2gmsk_mod_start failed: %d \n",ret);
	exit(-1);
}; // end if

// loop throu all messages
// message chain is in "chain"
// we are only interested in C2GMSK_MSG_PCM48K messages
// datasize is returned in datasize

tod=C2GMSK_MSG_PCM48K;

while ((msg = c2gmsk_msgchain_search_tod(C2GMSK_SEARCH_POSCURRENT, chain, tod, &numsample, &msgnr))) {
	printf("Processing message %d of start\n",msgnr);

	nsample=c2gmsk_msgdecode_pcm48k(msg,pcmbuffer);

	// we should have received 1920 samples
	if (nsample < 1920) {
		fprintf(stderr,"mod_start did not receive sufficient data: expected 1920: got %d \n",ret);
	}; // end if

	// write data
	ret=write(fout,pcmbuffer,(nsample<<1)); // ret = samples -> so multiply by 2 to get octets
	if (ret < 0) {
		// write should return number of octets written
		fprintf(stderr,"Error: could write not data: %d (%s)\n",errno,strerror(errno));
	}; // end if
}; // end while



//////////////////////////////////////////////////////////////////////
///////////////////// MOD VOICE1400 //////////////////////////////
//////////////////////////////////////////////////////////////

ret=read(fin, inbuffer,7);

framecount=0;
while (ret == 7) {
	// chain is returned via pointer pchain
	// chain search parameters will automatically be reinitialised
	nsample=c2gmsk_mod_voice1400(sessid,inbuffer,pchain);
	if (ret != C2GMSK_RET_OK) {
		fprintf(stderr,"Error: c2gmsk_mod failed: %d \n",ret);
		exit(-1);
	}; // end if

	chain=*pchain;

	// loop throu all messages in chain
	// message chain is in "chain"
	// we are only interested in C2GMSK_MSG_PCM48K messages
	// datasize is returned in datasize

	while ((msg = c2gmsk_msgchain_search_tod(C2GMSK_SEARCH_POSCURRENT, chain, tod, &numsample, &msgnr))) {
		printf("Processing message %d.%d of voice1400\n",framecount,msgnr);

		nsample=c2gmsk_msgdecode_pcm48k(msg,pcmbuffer);

		// we should have received 1920 samples
		if (nsample < 1920) {
			fprintf(stderr,"mod_voice1400 did not receive sufficient data: expected 1920: got %d \n",nsample);
		}; // end if

		// write data
		ret=write(fout,pcmbuffer,(nsample<<1)); // ret = samples -> so multiply by 2 to get octets
		if (ret < 0) {
			// write should return number of octets written
			fprintf(stderr,"Error: could write not data: %d (%s)\n",errno,strerror(errno));
		}; // end if
	}; // end while

	ret=read(fin, inbuffer,7);
	framecount++;
}; // end while


//////////////////////////////////////////////////////////////////////
///////////////////// MOD END   //////////////////////////////////
//////////////////////////////////////////////////////////////

// do mod end
ret=c2gmsk_mod_voice1400_end(sessid,pchain);
chain=*pchain;

if (ret != C2GMSK_RET_OK) {
	fprintf(stderr,"Error: c2gmsk_mod_voice1400_end failed: %d \n",ret);
	exit(-1);
}; // end if

tod=C2GMSK_MSG_PCM48K;

while ((msg = c2gmsk_msgchain_search_tod(C2GMSK_SEARCH_POSCURRENT, chain, tod, &numsample, &msgnr))) {
	printf("Processing message %d of voice1400_end\n",msgnr);

	nsample=c2gmsk_msgdecode_pcm48k(msg,pcmbuffer);

	// we should have received 1920 samples
	if (nsample < 1920) {
		fprintf(stderr,"mod_voice1400_end did not receive sufficient data: expected 1920: got %d \n",nsample);
	}; // end if

	// write data
	ret=write(fout,pcmbuffer,(nsample<<1)); // ret = samples -> so multiply by 2 to get octets
	if (ret < 0) {
		// write should return number of octets written
		fprintf(stderr,"Error: could write not data: %d (%s)\n",errno,strerror(errno));
	}; // end if
}; // end while


//////////////////////////////////////////////////////////////////////
///////////////////// AUDIOFLUSH  ////////////////////////////////
//////////////////////////////////////////////////////////////

// flush remaining audio in audiobuffer
ret=c2gmsk_mod_audioflush(sessid,pchain);
chain=*pchain;

if (ret != C2GMSK_RET_OK) {
	fprintf(stderr,"Error: c2gmsk_mod_audioflush failed: %d \n",ret);
	exit(-1);
}; // end if

tod=C2GMSK_MSG_PCM48K;

while ((msg = c2gmsk_msgchain_search_tod(C2GMSK_SEARCH_POSCURRENT, chain, tod, &numsample, &msgnr))) {
	printf("Processing message %d of audioflush\n",msgnr);

	nsample=c2gmsk_msgdecode_pcm48k(msg,pcmbuffer);

	// we should have received 1920 samples
	if (nsample < 1920) {
		fprintf(stderr,"mod_start did not receive sufficient data: expected 1920: got %d \n",nsample);
	}; // end if

	// write data
	ret=write(fout,pcmbuffer,(nsample<<1)); // ret = samples -> so multiply by 2 to get octets
	if (ret < 0) {
		// write should return number of octets written
		fprintf(stderr,"Error: could write not data: %d (%s)\n",errno,strerror(errno));
	}; // end if
}; // end while


////////////////////////////////////////////////////////////////
///////////////////// END SILENCE /////////////////////
//////////////////////////////////////////////

// create 1000 ms of silence ( 100 times 10 ms)
for (loop=0; loop<100; loop++){
	ret=write(fout,silence,sizeof(silence));
}; // end for

close(fout);
return(0);
}; // end if
