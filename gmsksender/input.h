/* INPUT FROM FILE */


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
// 20120105: Initial release

// This is split of in a seperate thread as this may run in parallel
// with the interrupt driven "alsa out" function which can "starve"
// the main application of CPU cycles

void * funct_input (void * globaldatain) {

// global data
globaldatastr *pglobal;
pglobal=(globaldatastr *) globaldatain;


// open file
if (pglobal->raw) {
	// open file, read raw data
	FILE * filein;
	size_t numread;
	unsigned char buffer[12]; // 20 ms @ 4.8 bps = 96 bits = 12 octets

	if (pglobal->infromstdin) {
		filein=stdin;
	} else {
		filein=fopen(pglobal->fnamein,"r");

		if (!(filein)) {
			fprintf(stderr,"Error: could not open input file!\n");
			exit(-1);
		}; // end if
	}; // end else - if

	numread=fread(buffer,1,12,filein);

	// repeat as long as we receive data
	while (numread) {
		// multiply by 8 to convert octets to bits		
		numread<<=3;
		bufferfill_bits(buffer, global.rawinvert, numread,pglobal);

		numread=fread(buffer,1,12,filein);
	}; // end while

	// close input file
	if (!(pglobal->infromstdin)) {
		fclose(filein);
	}; // end if

} else {
// DVTOOL file

	// next 128 bit pattern containing "1010... " (which is twice what
	// the specification demands, as that only requires 64 bits), followed
	// by the frame syncronisation pattern "1110110 01010000"

	// send with bit-inversion
	// NOTE, bufferfill_bits length is in BITS, not octets
	bufferfill_bits(startsync_pattern, 1, sizeof(startsync_pattern)*8, pglobal);

	// wait for both bits and audio buffer empty
	bufferwaitdone_bits(pglobal);

	// read input file (will write audio to audio-queue)
	funct_readdvtool(pglobal);

}; // end if

// wait for "bits" queue to end
bufferwaitdone_bits(pglobal);

// end program when all buffers empty
bufferwaitdone_audio(pglobal);

exit(0);

}; // end function input
