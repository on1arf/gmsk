// BUFFER RELATED FUNCTIONS

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
// version 20120105: no changes


int bufferfill_audio (int16_t * in, int size, int repeat, s_globaldatastr * pglobal) {

int loop, loop2;
int nextbuffer;
int currentbuffer;
int16_t *p_in; // pointer to in;

if (size<=0) {
	return(-1);
}; // end if

loop=0; // number of samples in buffer
loop2=0; // repeat

p_in=in;


currentbuffer=pglobal->ptr_a_fillup;
nextbuffer=currentbuffer +1;
if (nextbuffer >= BUFFERSIZE_AUDIO) {
	nextbuffer=0;
}; // end if


while (loop < size) {
	// if there place in the buffer?
	
	if (nextbuffer == pglobal->ptr_a_read) {
		// no place. Wait some time (1 ms)
		usleep(1000);
		continue;
	};

	// OK, we have place to store the data
	// we are going to push data, so we "waiting" bit should become 0 to show there
	// is data in the queue
	pglobal->waiting_audio=0;

	pglobal->buffer_audio[currentbuffer] = *p_in;

	loop2++; // move up repeat counter

	if (loop2 >= repeat) {
		// move up pointer
		p_in++;

		loop++;

		// reset loop2;
		loop2=0;
	}; // end if
	

	// set global var
	pglobal->ptr_a_fillup=nextbuffer;

	// calculate next buffer
	currentbuffer++;
	if (currentbuffer >= BUFFERSIZE_AUDIO) {
		currentbuffer=0;
	}; // end if
	nextbuffer++;
	if (nextbuffer >= BUFFERSIZE_AUDIO) {
		nextbuffer=0;
	}; // end if
}; // end while 


return(0);
}; // end function bufferfill_audio

//////////////// Function bufferfill_bits

int bufferfill_bits (unsigned char * in, int invertin, int size, s_globaldatastr * pglobal) {

static const unsigned char len2mask_r2l[] = { 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f}; // end table Right to left
static const unsigned char len2mask_l2r[] = { 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe}; // end table left to Right

int loop;
int nextbuffer;
int currentbuffer;
unsigned char *p_in; // pointer to in and to mask

if (size<=0) {
	return(-1);
}; // end if

// init loop to size, count down
loop=size;
p_in=in;

currentbuffer=pglobal->ptr_b_fillup;

nextbuffer=currentbuffer+1;
if (nextbuffer >= BUFFERSIZE_BITS) {
	nextbuffer=0;
}; // end if


while (loop > 0) {
	// if there place in the buffer?
	
	if (nextbuffer == pglobal->ptr_b_read) {
		// no place. Wait some time (1 ms)
		usleep(1000);
		continue;
	};

	// we are going to push data, so we "waiting" bit should become 0 to show there
	// is data in the queue
	pglobal->waiting_bits=0;

	// OK, we have place to store the data
	pglobal->buffer_bits[currentbuffer] = *p_in;
	if (invertin) {
		pglobal->octetorderinvert[currentbuffer]=0x01;
	} else {
		pglobal->octetorderinvert[currentbuffer]=0x00;
	}; // end if


	// set mask and decrease loop
	if (loop >= 8) {
		pglobal->buffer_bits_mask[currentbuffer]=0xff;
		loop -= 8;
	} else {
		// mask is variable from 0x01 to 0x3f, depending on how many
		// bit remaining
		if (invertin) {
			pglobal->buffer_bits_mask[currentbuffer]=len2mask_l2r[(loop-1)];
		} else {
			pglobal->buffer_bits_mask[currentbuffer]=len2mask_r2l[(loop-1)];
		}; // end else - if
		loop=0;
	}; // end else - if
		
	// move up pointer
	p_in++;

	// set global var
	pglobal->ptr_b_fillup=nextbuffer;

	// calculate next buffer
	currentbuffer++;
	if (currentbuffer >= BUFFERSIZE_BITS) {
		currentbuffer=0;
	}; // end if

	nextbuffer++;
	if (nextbuffer >= BUFFERSIZE_BITS) {
		nextbuffer=0;
	}; // end if
}; // end while 

return(0);

}; // end function bufferfill_bits


// function bufferwaitdone_bits
// loop until nothing more in queue for bits
// var is set by reading process (gmskmodulate)

void bufferwaitdone_bits(s_globaldatastr * pglobal) {
int l=0;
	while (pglobal->waiting_bits==0) {
		// sleep for 1 ms 
		usleep(1000);
		l++;
	}; // end while

return;
}; 


// function bufferwaitdone_audio
// loop until nothing more in queue for audio
// var is set by reading process (writefile) or alsaout

void bufferwaitdone_audio(s_globaldatastr * pglobal) {
	while (pglobal->waiting_audio==0)  {
		// sleep for 10 ms
		usleep(10000);
	}; // end while
return;
}; 


