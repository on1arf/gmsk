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

// version 20130326 initial release
// version 20130404: added half duplex support



//////////////////////////////
// audioplay callback function 
// FULL DUPLEX
static int funct_callback_fd( const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *globalin ) {

struct globaldata * pglobal;

pglobal=(struct globaldata *) globalin;

static int init=0;
static int state;
static int numBytes_p;
int numaudioframe;
int new_pntr_read, new_pntr_write;
int transmit;

///////////
// INIT: init vars first time function is called
if (!init) {
	state=0;


	// number of bytes for playback
	if (pglobal->outputParameters.channelCount == 1) {
		// mono: 1920 samples @ 2 bytes / sample
		numBytes_p=3840;
	} else {
		// stereo
		numBytes_p=7680;
	}; // end if

	init=1;
}; // end init


///////
// main part of function

// increase counter
pglobal->callbackcount++;


// part 0: common for audio playback and audio capture 
if (framesPerBuffer != 1920) {
	fprintf(stderr,"FramePerBuffer is %ld, expected 1920\n",framesPerBuffer);

	// if to small, break out
	if (framesPerBuffer < 1920) {
		return paContinue;
	}; // end if

}; // end if

transmit=0;

// part 1: audio playback (sender)
{
	// statemachine
	if (state == 0) {
		// state = 0: wait for audio, we need at least 2 buffers filled up before we start playing
		numaudioframe=pglobal->pntr_s_write - pglobal->pntr_s_read; // (how many audio frames are there in the buffer to play

		if (numaudioframe < 0) {
			// wrap around if needed
			numaudioframe += NUMBUFF;
		}; // end if

		if (numaudioframe < MINBUFF + 1) {
			// 0 or 1 audioframes is buffer. play silence and try again
			memset(outputBuffer,0,numBytes_p);

			goto endplayback;
		} else {
			// 2 or more audioframes in buffer, set state to 1 and continue below
			state=1;
		}; // end else - if
	};


	// state = 1, play audio if still available, otherwize, go back to state 0
	new_pntr_read = pglobal->pntr_s_read + 1;
	if (new_pntr_read >= NUMBUFF) {
		new_pntr_read=0 ; // wrap around at NUMBUFF
	}; // end if

	// STATE 1, senario 1: still more data
	if (new_pntr_read != pglobal->pntr_s_write) {
		// audio to play

		// stereo or mono?
		if (pglobal->outputParameters.channelCount == 2) {
			// stereo, duplicate left channel
			int loop;
			int16_t *s, *d;
						
			s=(int16_t *)pglobal->ringbuffer_s[new_pntr_read];
			d=outputBuffer;
			

			for (loop=0; loop < 1919; loop++) {
				*d=*s; d++; // copy left channel
				*d=*s; d++; s++; // duplicate left channel to right channel
			}; // end for

			*d=*s; d++; // last sample left
			*d=*s; // last sample right, no need to increase pointers anymore

		} else {
			// mono, just copy data
			memcpy(outputBuffer,pglobal->ringbuffer_s[new_pntr_read], 3840); // memcpy = octets, 1920 samples = 3840 octets
		}; // end else - if

		// move up marker
		pglobal->pntr_s_read=new_pntr_read;

		// mark transmit flag
		transmit=1;
		goto endplayback;
	};

	// STATE 1, senario 2: no more data -> set state to 0 and play silence
	memset(outputBuffer,0,numBytes_p);
	state=0;

}; // end part1: AUDIO PLAYBACK

endplayback:


// copy "transmit" to global vars, to be used by "PTT" function
pglobal->transmitptt=transmit;

// Part 2: audio capture


// if we are transmit, do not receive
if (transmit) {
	goto endcapture;
}; // end if

// just store data in ringbuffer
// is there place?

new_pntr_write = pglobal->pntr_r_write+1;

if (new_pntr_write >= NUMBUFF) {
	new_pntr_write=0;
}; // end if

if (new_pntr_write != pglobal->pntr_r_read) {

	// yes, there is place. Write data

	// stereo or mono?
	if (pglobal->inputParameters.channelCount == 2) {
		// stereo, only copy left channel
		int loop;
		int16_t *s, *d;
					
		s=(int16_t *)inputBuffer;
		d=(int16_t *)pglobal->ringbuffer_r[new_pntr_write];
			

		for (loop=0; loop < 1919; loop++) {
			*d=*s; d++; s+=2; // copy left channel
		}; // end for

		*d=*s; // last sample

	} else {
		// mono
		memcpy(pglobal->ringbuffer_r[new_pntr_write],inputBuffer,3840); // 1920 samples = 3840 octets
	}; // end else - if

	// move up marker
	pglobal->pntr_r_write=new_pntr_write;
} else {
	// no place to write audio
	putc('W',stderr);
}; // end if

// end part 2: AUDIO CAPTURE

endcapture:



return paContinue;

}; // end callback function portaudio


////////////////////////////////////////////////////////
//////////////////////////////////////////
//////////////////////////////
// audioplay callback function 
// HALF DUPLEX
static int funct_callback_hd( const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *globalin ) {

struct globaldata * pglobal;

pglobal=(struct globaldata *) globalin;

static int init=0;
static int state;
static int numBytes_p;
int numaudioframe;
int new_pntr_read, new_pntr_write;
static int last_receivefromnet;
static int sendsilence;

///////////
// INIT: init vars first time function is called
if (!init) {
	state=0;

	// init state
	last_receivefromnet=pglobal->receivefromnet;

	// init "sendsilence": timeout counter, how long can be keep on sending
	// nothing and keep the PTT active
	// even when no data received and no end received neither
	sendsilence=0;

	// number of bytes for playback
	if (pglobal->outputParameters.channelCount == 1) {
		// mono: 1920 samples @ 2 bytes / sample
		numBytes_p=3840;
	} else {
		// stereo
		numBytes_p=7680;
	}; // end if

	init=1;
}; // end init




////////////////////////////////
///////
// main part of function

// increase counter
pglobal->callbackcount++;


// part 0: common for audio playback and audio capture 
if (framesPerBuffer != 1920) {
	fprintf(stderr,"FramePerBuffer is %ld, expected 1920\n",framesPerBuffer);

	// if to small, break out
	if (framesPerBuffer < 1920) {
		return paContinue;
	}; // end if

}; // end if


// default state is "receive": capture audio
// we switch to "sending" as soon as the "receivefromnet" become high

// check state BEFORE current state
if (last_receivefromnet==0) {

	// first check if no data has come in from the network. If "yes" go
	// to "receiver from net" mode
	if (pglobal->receivefromnet) {
		// memorize this for the next cylce
		last_receivefromnet=1;

		// re-init "sendsilence" counter
		sendsilence=0;
		// break out of stream, forcing the main thread to break out its
		// loop and restart the stream as "playback"

		// always switch of PTT
		pglobal->transmitptt=0;
		return(paAbort);
	}; // end if

	// not received anything from net, so let's happyly continue reading audio

	// Part 2: audio capture
	// just store data in ringbuffer
	// is there place?

	new_pntr_write = pglobal->pntr_r_write+1;

	if (new_pntr_write >= NUMBUFF) {
		new_pntr_write=0;
	}; // end if

	if (new_pntr_write != pglobal->pntr_r_read) {

		// yes, there is place. Write data

		// stereo or mono?
		if (pglobal->inputParameters.channelCount == 2) {
			// stereo, only copy left channel
			int loop;
			int16_t *s, *d;
					
			s=(int16_t *)inputBuffer;
			d=(int16_t *)pglobal->ringbuffer_r[new_pntr_write];
			
			for (loop=0; loop < 1919; loop++) {
				*d=*s; d++; s+=2; // copy left channel
			}; // end for

			*d=*s; // last sample
		} else {
			// mono
			memcpy(pglobal->ringbuffer_r[new_pntr_write],inputBuffer,3840); // 1920 samples = 3840 octets
		}; // end else - if

		// move up marker
		pglobal->pntr_r_write=new_pntr_write;
	} else {
		// no place to write audio
		putc('w',stderr);
	}; // end if

	// DONE!!!
	// receiving, switch of PTT
	pglobal->transmitptt=0;
	return(paContinue);
}; // end if



// transmit = 1 -> we are sending audio from modem to radio

// when switch check to goi back to capture?
// 1/ data should have been received
// 2/ queue runs dry

// OR when the "receiving from queue" goes back to 0 and no data received


// statemachine
if (state == 0) {
	// not yet suffience audio in queue to start playing

	// only switch back to "receive" duplex if the "receivefromnet" is set
	// to "0" again by  the "netrececeive" thread 
	if (pglobal->receivefromnet == 0) {
		// switch to new state
		last_receivefromnet=0;

		// switch of PTT
		pglobal->transmitptt=0;
		return(paAbort);
	}; // end if


	// "receive from net" still active, read queue

	// state = 0: wait for audio, we need at least 2 buffers filled up before we start playing
	numaudioframe=pglobal->pntr_s_write - pglobal->pntr_s_read; // (how many audio frames are there in the buffer to play

	if (numaudioframe < 0) {
		// wrap around if needed
		numaudioframe += NUMBUFF;
	}; // end if

	if (numaudioframe < MINBUFF + 1) {
		// 0 or 1 audioframes is buffer. play silence and try again later
		memset(outputBuffer,0,numBytes_p);

		
		// PTT on, unless forced down by "sendsilence" timeout
		// counter;

		sendsilence++;

		if (sendsilence > TRANSMIT_SILENCE_TIMEOUT) {
			// keep on transmitting
			pglobal->transmitptt=0;
		} else {
			// timeout: stop transmitting
			pglobal->transmitptt=1;
		}; // end else - if

		// done
		return(paContinue);
	} else {
		// 2 or more audioframes in buffer, set state to 1 and continue below
		state=1;
	}; // end else - if
}; // end state=0


// state = 1, play audio from queue (if still available), otherwize, go back to state 0

// only go back to "capture" half-duplex state is queue is empty
new_pntr_read = pglobal->pntr_s_read + 1;
if (new_pntr_read >= NUMBUFF) {
	new_pntr_read=0 ; // wrap around at NUMBUFF
}; // end if

// STATE 1, senario 1: queue not empty
if (new_pntr_read != pglobal->pntr_s_write) {

	// audio to play

	// stereo or mono?
	if (pglobal->outputParameters.channelCount == 2) {
		// stereo, duplicate left channel
		int loop;
		int16_t *s, *d;
					
		s=(int16_t *)pglobal->ringbuffer_s[new_pntr_read];
		d=outputBuffer;
		

		for (loop=0; loop < 1919; loop++) {
			*d=*s; d++; // copy left channel
			*d=*s; d++; s++; // duplicate left channel to right channel
		}; // end for

		*d=*s; d++; // last sample left
		*d=*s; // last sample right, no need to increase pointers anymore

	} else {
		// mono, just copy data
		memcpy(outputBuffer,pglobal->ringbuffer_s[new_pntr_read], 3840); // memcpy = octets, 1920 samples = 3840 octets
	}; // end else - if

	// move up marker
	pglobal->pntr_s_read=new_pntr_read;


	// set "transmit" on, switches PTT
	pglobal->transmitptt=1;

	// done. Go to next audio-frame
	return(paContinue);
};

// STATE 1, senario 2: queue is empty

// is the "receivefromnetwork" flag still set

// no? (normal condition): switch back to "receive/capture" mode
// yes? (queue runs dry but stream not terminated, perhaps packetloss?)
// stay in "transmit" mode, but go back to state 0

// always reinit state
state=0;

if (pglobal->receivefromnet==0) {
	// memorize
	last_receivefromnet=0;

	return(paAbort);
}; // end if


// re-init "sendsilence" counter;
sendsilence=0;

pglobal->transmitptt=1;
memset(outputBuffer,0,numBytes_p);
return paContinue;

}; // end callback function portaudio

