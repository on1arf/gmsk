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


// SENDER AUDIO-buffer operations

// version 20130326 initial release
// version 20130404: added half duplex support


/////////////////////////////////
// function bufferfill_audio

int bufferfill_audio (int16_t * in, int nsamples, struct globaldata * pglobal) {

int nextbuffer;
int waitcount;
char *p_audioin; // pointer to audio in; OCTET based

static int init=0;
static int free; // how much still free in current buffer
static char *p_audioout=NULL; // pointer to audio in current buffer; OCTET based
static int currentbuffer;

const int numBytes=3840; // size of audio-buffer


int tocopy; // how much octets still to queue?
int copynow;


// init static variables
if (!init) {
	init=1;
	pglobal->pntr_s_write=0;

	free=0;
}; // end if


// some error checking
if (nsamples<=0) {
	return(-1);
}; // end if



// first check if there is place to store data?
// only do check if there is no place left in the queue

// As "free" is initialised the first time the function is called to 0, this
// code-segment will also be called the first time the function is called, thereby
// also initiasing "currentbuffer"
if (free == 0) {

	// is there place in the queue to store the audio?
	nextbuffer=pglobal->pntr_s_write+1;
	if (nextbuffer >= NUMBUFF) {
		nextbuffer=0;
	};

	waitcount=0;
	// wait untill place is available in queue
	while (nextbuffer == pglobal->pntr_s_read) {
		// sleep 1 ms
		usleep(1000);

		// break out after 1 second
		waitcount++;

		if (waitcount >= 1000) {
			// break out (note: data will be lost)
			putc('R',stderr);
			return(-2);
		}; // end if
	}; // end if

	// OK, we have place
	currentbuffer=nextbuffer;

	// set pointer to beginning of buffer
	p_audioout=pglobal->ringbuffer_s[nextbuffer];

	// reinit some vars
	free=numBytes;
}; // end if



// we now have a place to store data. Let's copy audio


// set input pointer to beginning of input buffer
p_audioin= (char *)in;


tocopy=nsamples << 1; // nsample =  samples -> multiply by 2 to go to octets

// repeat until all data processed
while (tocopy > 0) {
	// how much data to copy this time?
	if (tocopy < free) {
		// more free space then data
		copynow=tocopy;
	} else {
		// more data then free space
		copynow=free;
	}; // end else - if


	// copy audio
	memcpy(p_audioout,p_audioin,copynow);

	// we now have less free space in buffer but also less still to queue
	free -= copynow;
	tocopy -= copynow;

	p_audioin += copynow; // move up input pointer



	// is the audio buffer full?
	if (free == 0) {
		// yes, then go to next buffer in queue (if available) and reinit vars

		// move up pointer in write queue
		pglobal->pntr_s_write=currentbuffer;

		// is there place in the queue to store the audio?
		nextbuffer=currentbuffer+1;
		if (nextbuffer >= NUMBUFF) {
			nextbuffer=0;
		};

		waitcount=0;
		// wait untill place is available in queue
		while (nextbuffer == pglobal->pntr_s_read) {
			// sleep 1 ms
			usleep(1000);

			// break out after 1 second
			waitcount++;

			if (waitcount >= 1000) {
				// break out (note: data will be lost)
				putc('r',stderr);
				return(-3);
			}; // end if
		}; // end if

		// OK, we have place
		currentbuffer=nextbuffer;

		// reinit vars
		// set pointer to beginning of buffer
		p_audioout=pglobal->ringbuffer_s[currentbuffer];
		free=numBytes;

	} else {
		// buffer is not full, just move up output pointer
		p_audioout += copynow;
	}; // end if

}; // end while

return(0);
}; // end function bufferfill_audio


//////////////////
// function bufferfill silence
int bufferfill_silence (int n10thsec, struct globaldata * pglobal) {
int loop;
static int init=0;

static int16_t silence[480]; // 10 ms @ 48000 samples / sec 

if (!init) {
	init=1;

	// fill silence with all 0
	memset(silence,0,960); // 480 samples @ 2 octets / sample
}; // end if


for (loop=0; loop <n10thsec; loop++) {
	bufferfill_audio(silence, 480, pglobal);
}; // end for

return(0);
}; // end function  bufferfill_silence


///////////////////
// function bufferfill_audio_pcm48kmsg
int bufferfill_audio_pcm48kmsg_p(struct c2gmsk_msg * msg, struct globaldata * pglobal) {
int nsamples;
int16_t *pcmbuffer; // configure as pointer to buffer as c2gmsk_msgdecode_pcm48k_p


// does not copy data, only returns pointer to it

nsamples=c2gmsk_msgdecode_pcm48k_p(msg,&pcmbuffer);

if (nsamples != 1920) {
	fprintf(stderr,"Internal error in bufferfill_audio_pcm48msg: msgdecode should return 1920 samples, got %d \n",nsamples);
	exit(-1);
}; // end if

// queue data, data is
bufferfill_audio(pcmbuffer,1920, pglobal);

// done
return(0);

}; // end function bufferfill_audio_pcm48k
