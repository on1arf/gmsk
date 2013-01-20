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




//////////////////////////////
// audioplay callback function 
static int funct_callback( const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *globalin ) {

globaldatastr * pglobal;
pglobal=(globaldatastr *) globalin;
static int init=1;
static int state;
static int numBytes_p;
static int  numBytes_c;
int numaudioframe;
int new_p_ptr_read, new_c_ptr_write;

///////////
// INIT: init vars first time function is called
if (init) {
	state=0;

	if (pglobal->inputParameters.channelCount == 2) {
		numBytes_c=7680;
	} else {
		numBytes_c=3840;
	}; // end else - if

	if (pglobal->outputParameters.channelCount == 2) {
		numBytes_p=7680;
	} else {
		numBytes_p=3840;
	}; // end else - if

	init=0;
}; // end init


///////
// main part of function

// increase counter
pglobal->callbackcount++;


// part 0: common for audio playback and audio capture 
if (framesPerBuffer != 1920) {
	fprintf(stderr,"FramePerBuffer is %ld, expected 1920\n",framesPerBuffer);
}; // end if


// part 1: audio playback
{
	// statemachine
	if (state == 0) {
		// state = 0: wait for audio, we need at least 2 buffers filled up before we start playing
		numaudioframe=pglobal->p_ptr_write - pglobal->p_ptr_read; // (how many audio frames are there in the buffer to play

		if (numaudioframe < 0) {
			// wrap around if needed
			numaudioframe += NUMBUFF;
		}; // end if

		if (numaudioframe < MINBUFF + 1) {
			// 0 or 1 audioframes is buffer. play silence and try again
			memset(outputBuffer,0,numBytes_p);

	//		putc('S',stderr);
			goto endplayback;
		} else {
	//	putc('1',stderr);
			// 2 or more audioframes in buffer, set state to 1 and continue below
			state=1;
		}; // end else - if
	};


	// state = 1, play audio if still available, otherwize, go back to state 0
	new_p_ptr_read = pglobal->p_ptr_read + 1;
	if (new_p_ptr_read >= NUMBUFF) {
		new_p_ptr_read=0 ; // wrap around at NUMBUFF
	}; // end if

	// STATE 1, senario 1: still more data
	if (new_p_ptr_read != pglobal->p_ptr_write) {
//	putc('R',stderr);
		// audio to play
		memcpy(outputBuffer,pglobal->p_ringbuffer[new_p_ptr_read], numBytes_p);
		// move up marker
		pglobal->p_ptr_read=new_p_ptr_read;
		goto endplayback;
	};

	// STATE 1, senario 2: no more data -> set state to 0 and play silence
	memset(outputBuffer,0,numBytes_p);
	state=0;

}; // end part1: AUDIO PLAYBACK

endplayback:




// Part 2: audio capture

// just store data in ringbuffer

// is there place?

new_c_ptr_write = pglobal->c_ptr_write+1;

if (new_c_ptr_write >= NUMBUFF) {
	new_c_ptr_write=0;
}; // end if

if (new_c_ptr_write != pglobal->c_ptr_read) {

	// yes, there is place. Write data
	memcpy(pglobal->c_ringbuffer[new_c_ptr_write],inputBuffer,numBytes_c);

	// move up marker
	pglobal->c_ptr_write=new_c_ptr_write;
} else {
	// no place to write audio
	putc('W',stderr);
}; // end if

// end part 2: AUDIO CAPTURE



return paContinue;

}; // end callback function portaudio write

