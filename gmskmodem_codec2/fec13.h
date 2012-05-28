// fec13.h


// Implements a 1/3 FEC decoder based on "test of 3" principle


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
// version 20120527: initial release



int fec13decode(unsigned char * in1, unsigned char * in2, unsigned char * in3, unsigned char * ret) {
int errors=0;
int loop;
int ones;
unsigned char *p;

p=bit2octet; // point to first elem in list of "bit2octet" list. As defined in "global.h"

// clear return value
*ret=0x00;

for (loop=0; loop <= 7; loop++) {
	ones=0;

	// count number of ones in position dependent on 
	if (*in1 & *p) { ones++; }; // end if
	if (*in2 & *p) { ones++; }; // end if
	if (*in3 & *p) { ones++; }; // end if

	if ((ones == 1) || (ones == 2)) {
		errors++;
	}; // end if

	// set output bit to 1 if at least two input bits are 1 for that position
	if (ones >= 2) {
		*ret |= *p;
	}; // end if

	// move up bit position pointer;
	p++;
}; // end for


return(errors);

}; // end if
