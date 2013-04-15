/* WRITE FILE */


// This code is part of the "gmsksender" application

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


void * funct_writefile (void * globaldatain) {

// global data
s_globaldatastr *s_pglobal;
s_pglobal=(s_globaldatastr *) globaldatain;

// local vars
FILE * fileout;
int nextbuffer;

// init vars
s_pglobal->waiting_audio=-1;

// write file reads audio from the "audio" buffer and writes it to a file
// open file
fileout=fopen(s_pglobal->fnameout,"w");

if (!(fileout)) {
	fprintf(stderr,"Error: could not open output file!\n");
	exit(-1);
}; // end if


// read data from buffer

// find next buffer
nextbuffer=s_pglobal->ptr_a_read +1;
if (nextbuffer >= BUFFERSIZE_AUDIO) {
	nextbuffer=0;
}; 


// endless loop

while (FOREVER) {
	// is there something to send?

	if (nextbuffer == s_pglobal->ptr_a_fillup) {
		s_pglobal->waiting_audio=1;
		// nothing to write. Sleep for 10 ms

		usleep(10000);
		continue;
	}; // end if

	// OK, we have data
	s_pglobal->waiting_audio=0;

	fwrite(&s_pglobal->buffer_audio[nextbuffer],sizeof(int16_t),1,fileout);

	// update pointer in global data
	s_pglobal->ptr_a_read=nextbuffer;

	// move up pointer
	nextbuffer++;

	if (nextbuffer >= BUFFERSIZE_AUDIO) {
		nextbuffer=0;
	}; 

}; // end while (endless loop);

// out of loop, we should never reach this

// re-init vars
s_pglobal->waiting_audio=-1;

}; // end function funct_writefile
