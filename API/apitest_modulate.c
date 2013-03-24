//////////////////////
// API version of the GMSK modem for 10m / VHF / UHF communication
// using codec2
// version 0 (versionid 0x27f301): 4800 bps, 1/3 repetition code FEC


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
// Version 20130314: API c2gmsk version / bitrate control + versionid codes
// Version 20130324: convert into .so shared library



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

#include <c2gmsk.h>


// functions written below
void write_audio_to_file(int, struct c2gmsk_msg *);

///////////////////////
// application starts here

int main (int argc, char ** argv) {
struct c2gmsk_session * sessid=NULL;
struct c2gmsk_msgchain * chain=NULL, ** pchain;
struct c2gmsk_msg * msg;
struct c2gmsk_param param;

int16_t silence[480]; // 480 samples = 10 ms @ 48Khz samplerate
int tod, numsample, msgnr;
int framecount;

int f_out, f_in; // file out and file in 
char * infilename, * outfilename; // names in input and output files

int ret;
int loop;

int nsample;

unsigned char inbuffer[7];


// check parameters, we need at least 2 parameters: infile and outfile
if (argc < 3) {
	fprintf(stderr,"Error: at least 2 parameters needed.\nUsage: %s infile.c2 modaudiofile.raw \n",argv[0]);
	exit(-1);
} else {
	infilename=argv[1];
	outfilename=argv[2];
}; // end else - if

// fill silence will all zero
memset(&silence,0,sizeof(silence));

// open out file
f_out=open(outfilename,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

if (f_out < 0) {
	fprintf(stderr,"Error: could not open file for output: %d (%s)! \n",errno,strerror(errno));
	exit(-1);
}; // end if


// open input file
f_in=open(infilename,O_RDONLY);

if (f_in < 0) {
	fprintf(stderr,"Error: could not open file for input: %d (%s)! \n",errno,strerror(errno));
	exit(-1);
}; // end if

// pchain points to location of chain
pchain=&chain;


// init parameters (signatures + default values)
// GLOBAL PARAMETERS
ret=c2gmsk_param_init(&param);
if (ret != C2GMSK_RET_OK) {
	fprintf(stderr,"Error: Could not initiate parameter structures!\n");
	exit(-1);
}; // end if

// fill in parameter
// global
param.expected_apiversion=0;

// modulation parameters
param.m_version=0; // version 0 for 4800 bps = versioncode 0x27F301
param.m_bitrate=C2GMSK_MODEMBITRATE_4800;

// demodulation parameters not filled in (we do not do any demodulation here)


// init session
sessid=c2gmsk_sess_new(&param,&ret,pchain);
if (ret != C2GMSK_RET_OK) {
	fprintf(stderr,"Error: could not create C2GMSK session: %d \n",ret);
	exit(-1);
}; // end if


// create 300 ms of silence ( 30 times 10 ms)
for (loop=0; loop<30; loop++){
	ret=write(f_out,silence,sizeof(silence));
}; // end for


//////////////////////////////////////////////////////////////////////
///////////////////// MOD START //////////////////////////////////
//////////////////////////////////////////////////////////////

// do mod start
// chain is returned via pointer pchain
// chain search parameters will automatically be reinitialised
// pass parameters via "param_s" structure
ret=c2gmsk_mod_start(sessid,pchain);
chain=*pchain;

if (ret != C2GMSK_RET_OK) {
	fprintf(stderr,"Error: c2gmsk_mod_start failed: %d \n",ret);
	exit(-1);
}; // end if

// loop throu all messages
// message chain is in "chain"
// we are only interested in C2GMSK_MSG_PCM48K messages
// number of audio frames is returned in numsample
// message number if returned in msgnr

tod=C2GMSK_MSG_PCM48K;

while ((msg = c2gmsk_msgchain_search_tod(C2GMSK_SEARCH_POSCURRENT, chain, tod, &numsample, &msgnr))) {
	printf("Processing message %d of start\n",msgnr);

	// write audio to file: defined bewlo
	write_audio_to_file(f_out, msg);
}; // end while



//////////////////////////////////////////////////////////////////////
///////////////////// MOD VOICE1400 //////////////////////////////
//////////////////////////////////////////////////////////////

ret=read(f_in, inbuffer,7);

framecount=0;
while (ret == 7) {
	// chain is returned via pointer pchain
	// chain search parameters will automatically be reinitialised
	nsample=c2gmsk_mod_voice1400(sessid,inbuffer,pchain);
	if (nsample != C2GMSK_RET_OK) {
		fprintf(stderr,"Error: c2gmsk_mod failed: %d \n",ret);
		exit(-1);
	}; // end if

	chain=*pchain;

	// loop throu all messages in chain
	// message chain is in "chain"
	// we are only interested in C2GMSK_MSG_PCM48K messages
	// number of audio frames is returned in numsample
	// message number if returned in msgnr

	while ((msg = c2gmsk_msgchain_search_tod(C2GMSK_SEARCH_POSCURRENT, chain, tod, &numsample, &msgnr))) {
		printf("Processing message %d.%d of voice1400\n",framecount,msgnr);

		write_audio_to_file(f_out, msg);
	}; // end while

	ret=read(f_in, inbuffer,7);
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

while ((msg = c2gmsk_msgchain_search_tod(C2GMSK_SEARCH_POSCURRENT, chain, tod, &numsample, &msgnr))) {
	printf("Processing message %d of voice1400_end\n",msgnr);

	write_audio_to_file(f_out, msg);
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

while ((msg = c2gmsk_msgchain_search_tod(C2GMSK_SEARCH_POSCURRENT, chain, tod, &numsample, &msgnr))) {
	printf("Processing message %d of audioflush\n",msgnr);

	write_audio_to_file(f_out, msg);
}; // end while


////////////////////////////////////////////////////////////////
///////////////////// END SILENCE /////////////////////
//////////////////////////////////////////////

// create 1000 ms of silence ( 100 times 10 ms)
for (loop=0; loop<100; loop++){
	ret=write(f_out,silence,sizeof(silence));
}; // end for

close(f_out);
return(0);
}; // end application
////////////////////////////////
////////// END OF APPLICATION


/////////////////////////////////////////////
/// function write_to_file

void write_audio_to_file(int file_out, struct c2gmsk_msg * msg) {
int nsample, ret;
int16_t pcmbuffer[1920];


nsample=c2gmsk_msgdecode_pcm48k(msg,pcmbuffer);

// we should have received 1920 samples
if (nsample < 1920) {
	fprintf(stderr,"mod_start did not receive sufficient data: expected 1920: got %d \n",nsample);
}; // end if

// write data
ret=write(file_out,pcmbuffer,(nsample<<1)); // ret = samples -> so multiply by 2 to get octets
if (ret < 0) {
	// write should return number of octets written
	fprintf(stderr,"Error: could write not data: %d (%s)\n",errno,strerror(errno));
}; // end if

}; // end function 
