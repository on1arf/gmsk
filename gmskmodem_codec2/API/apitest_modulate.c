//////////////////////
// API version of the GMSK modem for 10m / VHF / UHF communication
// using codec2
// codec versions 4800/0 and 2400/15


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
// Version 20130614: merge 2400 and 4800 bps modem, add auxdata, add raw-gmsk output



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
void write_to_file(int, struct c2gmsk_msg *, int);

///////////////////////
// application starts here

int main (int argc, char ** argv) {
struct c2gmsk_session * sessid=NULL;
struct c2gmsk_msgchain * chain=NULL, ** pchain;
struct c2gmsk_msg * msg;
struct c2gmsk_param param;

int16_t silence[480]; // 480 samples = 10 ms @ 48Khz samplerate
int tod, numsample, msgnr;
int framecount, msgcount;

int f_out, f_in; // file out and file in 
char * infilename, * outfilename; // names in input and output files

int ret;
int loop;

int nsample;

unsigned char inbuffer[7];

char * mytext;

int bitrate;
int outputformat;

// check parameters, we need at least 2 parameters: infile and outfile
if (argc < 5) {
	fprintf(stderr,"Error: at least 4 parameters needed.\n");
	fprintf(stderr,"Usage: %s infile.c2 modaudiofile.raw bitrate outformat <auxdata-text>\n",argv[0]);
	fprintf(stderr,"bitrate: 2400 or 4800\n");
	fprintf(stderr,"outformat: \"a\" (audio) or \"g\" (gmsk)\n");
	exit(-1);
} else {
	infilename=argv[1];
	outfilename=argv[2];

	bitrate=atoi(argv[3]);
}; // end else - if

if ((bitrate != 2400) && (bitrate != 4800)) {
	fprintf(stderr,"Error: bitrate should be 2400 or 4800. Got %d.\n",bitrate);
	exit(-1);
}; // end if

if ((argv[4][0] == 'a') || (argv[4][0] == 'A')) {
	outputformat=C2GMSK_OUTPUTFORMAT_AUDIO;
} else if ((argv[4][0] == 'g') || (argv[4][0] == 'G')) {
	outputformat=C2GMSK_OUTPUTFORMAT_GMSK;
} else {
	fprintf(stderr,"Error: output format should be audio or GMSK, got %c.\n",argv[4][0]);
	exit(-1);
}; // else else - elsif - if


if (argc > 5) {
	// extra argument: to send text
	mytext=argv[5];
} else {
	mytext=NULL;
}; // end if

// fill silence will all zero
memset(&silence,0,sizeof(silence));

// open out file
f_out=open(outfilename,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

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

// API needed for raw gmsk-output
param.expected_apiversion=20130614;

// modulation parameters
if (bitrate == 2400) {
	param.m_version=15; // version 15 for 2400 bps
	param.m_bitrate=C2GMSK_MODEMBITRATE_2400;
} else {
	param.m_version=0; // version 0 for 4800 bps
	param.m_bitrate=C2GMSK_MODEMBITRATE_4800;
}; // end else - if

// disactivate demodulation
param.d_disabled=C2GMSK_DISABLED;

// set output format
param.outputformat=outputformat;


// init session
sessid=c2gmsk_sess_new(&param,&ret,pchain);
if (ret != C2GMSK_RET_OK) {
	fprintf(stderr,"Error: could not create C2GMSK session: %d \n",ret);
	exit(-1);
}; // end if


// create 300 ms of silence ( 30 times 10 ms)
// only if output format is gmsk
if (outputformat == C2GMSK_OUTPUTFORMAT_AUDIO) {
	for (loop=0; loop<30; loop++){
		ret=write(f_out,silence,sizeof(silence));
	}; // end for
}; // end if


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
// we are only interested in C2GMSK_MSG_PCM48K, GMSK96 or GMSK192 messages
// number of audio frames is returned in numsample
// message number if returned in msgnr

if (outputformat == C2GMSK_OUTPUTFORMAT_AUDIO) {
// audio
	tod=C2GMSK_MSG_PCM48K;
} else {
// raw GMSK

	if (bitrate == 2400) {
		tod=C2GMSK_MSG_RAWGMSK_96;
	// 2400 bps
	} else {
	// 4800 bps
		tod=C2GMSK_MSG_RAWGMSK_192;
	}; // end if
}; // end if

while ((msg = c2gmsk_msgchain_search_tod(C2GMSK_SEARCH_POSCURRENT, chain, tod, &numsample, &msgnr))) {

	if (tod == C2GMSK_MSG_PCM48K) {
		printf("Processing message %d of mod_start (AUDIO)\n",msgnr);
		write_to_file(f_out, msg, tod);
	} else if (tod == C2GMSK_MSG_RAWGMSK_96) {
		printf("Processing message %d of mod_start (GMSK_96)\n",msgnr);
		write_to_file(f_out, msg, tod);
	} else if (tod == C2GMSK_MSG_RAWGMSK_192) {
		printf("Processing message %d of mod_start (GMSK_192)\n",msgnr);
		write_to_file(f_out, msg, tod);
	}; // end elsif - elsif - if

	framecount++;
}; // end while


//////////////////////////////////////////////////////////////////////
///////////////////// MOD VOICE1400 //////////////////////////////
//////////////////////////////////////////////////////////////

ret=read(f_in, inbuffer,7);

framecount=0;
while (ret == 7) {
	// send message after frame 3, if it exists (if not, mytext points to NULL)
	if ((framecount == 3) && (mytext)) {
		ret=c2gmsk_auxdata_sendmessage(sessid,mytext,7);
	};

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

	msgcount=0;

	while ((msg = c2gmsk_msgchain_search(C2GMSK_SEARCH_POSCURRENT, chain, &tod, &numsample, NULL))) {
		

		if (tod == C2GMSK_MSG_PCM48K) {
			printf("Processing message %d.%d of voice1400 (AUDIO)\n",framecount,msgcount);
			write_to_file(f_out, msg, tod);
		} else if (tod == C2GMSK_MSG_RAWGMSK_96) {
			printf("Processing message %d.%d of voice1400 (GMSK_96)\n",framecount,msgcount);
			write_to_file(f_out, msg, tod);
		} else if (tod == C2GMSK_MSG_RAWGMSK_192) {
			printf("Processing message %d.%d of voice1400 (GMSK_192)\n",framecount,msgcount);
			write_to_file(f_out, msg, tod);
		} else if (tod == C2GMSK_MSG_AUXDATA_DONE) {
			printf("message %d.%d AUXDATA DONE \n",framecount,msgcount);
		} else {
			printf("message %d.%d tod %d \n",framecount,msgcount,tod);
		};

		msgcount++;
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

// we are looking for type-of-message C2GMSK_MSG_PCM48K, RAW96 or RAW192
if (outputformat == C2GMSK_OUTPUTFORMAT_AUDIO) {
// audio
	tod=C2GMSK_MSG_PCM48K;
} else {
// raw GMSK

	if (bitrate == 2400) {
		tod=C2GMSK_MSG_RAWGMSK_96;
	// 2400 bps
	} else {
	// 4800 bps
		tod=C2GMSK_MSG_RAWGMSK_192;
	}; // end if
}; // end if

msgnr=0;
while ((msg = c2gmsk_msgchain_search_tod(C2GMSK_SEARCH_POSCURRENT, chain, tod, &numsample, &msgnr))) {
	if (tod == C2GMSK_MSG_PCM48K) {
		printf("Processing message %d of mod_end (AUDIO)\n",msgnr);
		write_to_file(f_out, msg, tod);
	} else if (tod == C2GMSK_MSG_RAWGMSK_96) {
		printf("Processing message %d of mod_end (GMSK_96)\n",msgnr);
		write_to_file(f_out, msg, tod);
	} else if (tod == C2GMSK_MSG_RAWGMSK_192) {
		printf("Processing message %d of mod_end (GMSK_192)\n",msgnr);
		write_to_file(f_out, msg, tod);
	}; // end elsif - elsif - if

	msgnr++;
}; // end while


//////////////////////////////////////////////////////////////////////
///////////////////// AUDIOFLUSH  ////////////////////////////////
//////////////////////////////////////////////////////////////

// flush remaining audio in audiobuffee
ret=c2gmsk_mod_outputflush(sessid,pchain);
chain=*pchain;

if (ret != C2GMSK_RET_OK) {
	fprintf(stderr,"Error: c2gmsk_mod_audioflush failed: %d \n",ret);
	exit(-1);
}; // end if


msgnr=0;
while ((msg = c2gmsk_msgchain_search_tod(C2GMSK_SEARCH_POSCURRENT, chain, tod, &numsample, &msgnr))) {
	if (tod == C2GMSK_MSG_PCM48K) {
		printf("Processing message %d of outputflush (AUDIO)\n",msgnr);
		write_to_file(f_out, msg, tod);
	} else if (tod == C2GMSK_MSG_RAWGMSK_96) {
		printf("Processing message %d of outputflush (GMSK_96)\n",msgnr);
		write_to_file(f_out, msg, tod);
	} else if (tod == C2GMSK_MSG_RAWGMSK_192) {
		printf("Processing message %d of outputflush (GMSK_192)\n",msgnr);
		write_to_file(f_out, msg, tod);
	}; // end elsif - elsif - if

	msgnr++;
}; // end while

/////////// END SILENCE ///////////


if (outputformat == C2GMSK_OUTPUTFORMAT_AUDIO) {
	// create 1000 ms of silence ( 100 times 10 ms)
	for (loop=0; loop<100; loop++){
		ret=write(f_out,silence,sizeof(silence));
	}; // end for
}; // end if

close(f_out);
return(0);
}; // end application


////////////////////////////////
////////// END OF APPLICATION
//////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////
////////////////////////////////////////////////



/////////////////////////////////////////////
/// function write_to_file

void write_to_file(int file_out, struct c2gmsk_msg * msg, int tod) {
int size=0, ret;

// reuse memory for both types of data
union {
	int16_t pcmbuffer[1920];
	unsigned char gmskbuffer[24];
} buff_u;


// some sanity checking
if (tod == C2GMSK_MSG_PCM48K) {
	size=c2gmsk_msgdecode_pcm48k(msg,buff_u.pcmbuffer);

	// "size" here is actually number of samples (half the number of octets)
	// we should have received 1920 samples
	if (size < 1920) {
		fprintf(stderr,"mod / write_to_file error %d: %s\n",-size,c2gmsk_printstr_ret(-size));
		return;
	}; // end if

	// double the size to get read number of octets
	size <<= 1;
} else if ((tod == C2GMSK_MSG_RAWGMSK_96) || (tod == C2GMSK_MSG_RAWGMSK_192)) {
	size=c2gmsk_msgdecode_gmsk(msg,buff_u.gmskbuffer);

	if (size < 0) {
		fprintf(stderr,"mod / write to file error %d: %s\n",-size,c2gmsk_printstr_ret(-size));
		return;
	}; // end if

} else {
	// some unknown type-of-data
	fprintf(stderr,"mod / write_to_file got unexpected type of data: %d\n",tod);
}; // end if


// write data
ret=write(file_out,&buff_u,size); // 
if (ret < size) {
	// write should return number of octets written
	fprintf(stderr,"Error: could write not data (expected %d, write %d). Error %d (%s)\n",size,ret,errno,strerror(errno));
}; // end if

}; // end function 
