// DSP related functions and data //


// Original copyright:
// (C) 2011 Jonathan Naylor G4KLX

// (C) 2011 Kristoff Bonne ON1ARF

// These is the low-level DSP functions that make up the gmsk demodulator:
// firfilter
// decode

// This code is largely based on the code from the "pcrepeatercontroller"
// project, written by Jonathan Naylor 
// More info: 
// http://groups.yahoo.com/group/pcrepeatercontroller


// Changes:
// Convert FIR-filter to integer
// Change PLL values so to make PLLINC and SMALLPLLINC match integer-boundaries
// Concert C++ to C


// Version 20111106: initial release


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



// global data

int32_t coeffs_table [] = {
-330625, 0, 359119, 733614, 1102377, 1433432, 
1683416, 1800222, 1729032, 1421346, 845922, 0, 
-1081195, -2318757, -3590008, -4735270, -5571855, -5914587, 
-5601089, -4519063, -2632073, 0, 3209518, 6721652, 
10168847, 13119499, 15119417, 15742881, 14648517, 11634040, 
6683403, 0, -7979489, -16594376, -24993945, -32195137, 
-37160267, -38888647, -36514051, -29398683, -17214075, 0, 
21805774, 47371588, 75511384, 104757736, 133463064, 159921024, 
182497456, 199759408, 210590448, 214281776, 210590448, 199759408, 
182497456, 159921024, 133463064, 104757736, 75511384, 47371588, 
21805774, 0, -17214075, -29398683, -36514051, -38888647, 
-37160267, -32195137, -24993945, -16594376, -7979489, 0, 
6683403, 11634040, 14648517, 15742881, 15119417, 13119499, 
10168847, 6721652, 3209518, 0, -2632073, -4519063, 
-5601089, -5914587, -5571855, -4735270, -3590008, -2318757, 
-1081195, 0, 845922, 1421346, 1729032, 1800222, 
1683416, 1433432, 1102377, 733614, 359119, 0, 
-330625
}; // end table


const int coeffs_size=103;
const int buffersize=2060; // coeffs_size * 20;

#define PLLMAX 3200 
#define PLLHALF 1600
// PLL increase = PLLMAX / 10
#define PLLINC 320

// INC = 32 (not used in application itself)
//#define INC 32

// SMALL PLL Increase = PLLINC / INC
#define SMALLPLLINC 10


/// function firfilter

signed long long firfilter(int16_t val) {
static int pointer;
static int init=1;

static int16_t *buffer;

//int64_t retval;
signed long long retval;

int bufferloop;

int16_t *ptr1;
int32_t *ptr2;

if (init) {
	init=0;

	// allocate memory for buffer
	buffer=malloc(buffersize * sizeof(int16_t));

	if (!buffer) {
		fprintf(stderr,"Error: could allocate memory for databuffer!\n");
		exit(-1);
	}; // end if


	// init buffer: all zero for first "coeffs_size" elements.
	memset(buffer,0,coeffs_size * sizeof(int16_t));

	// init vars

	pointer=coeffs_size;

	// END INIT
}; // end if

// main part of function starts here
buffer[pointer]=val;
pointer++;


// go throu all the elements of the coeffs table
ptr1=&buffer[pointer-coeffs_size];
ptr2=coeffs_table;

retval=0;
for (bufferloop=0;bufferloop<coeffs_size;bufferloop++) {
	retval += ((long long) *ptr1++) * ((long long) *ptr2++);
}; // end for


// If pointer has moved past of end of buffer, copy last "coeffs_size"
// elements to the beginning of the table and move pointer down
if (pointer >= buffersize) {
	memmove(buffer,&buffer[buffersize-coeffs_size],coeffs_size*sizeof(int16_t));
	pointer=coeffs_size;
}; // end if

return(retval);

};


int demodulate (int16_t audioin) {

static int init=1;
static int last;
static int m_pll;
signed long long filterret;

int bit;


if (init) {
	init=0;
 
	// init vars
	last=0;
	m_pll=0;

// end INIT part of function
}; // end if


// main part of function starts here

filterret=firfilter(audioin);

if (filterret > 0) {
	bit=1;
} else {
	bit=0;
}; // end else - if


// PLL loop: used to detect points where values passes over the "zero"

if (bit != last) {
	if (m_pll < PLLHALF) {
		m_pll += SMALLPLLINC;
	} else {
		m_pll -= SMALLPLLINC;
	}; // end else - if
}; // end if

last=bit;

m_pll += PLLINC;


if (m_pll >= PLLMAX) {
	m_pll -= PLLMAX;

	if (bit) {
		return(0);
	} else {
		return(1);
	}; // end if
			
} else {
	return(-1);
}; // end else - if

}; // end function program



