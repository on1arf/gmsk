//////////////////////
// API version of the GMSK modem for 10m / VHF / UHF communication
// using codec2
// version 0 (versionid 0x27f301): 4800 bps, 1/3 repetition code FEC


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


// Release information
// version 20120527: initial release
// version 20130310: initial release API version, no change
// Version 20130314: API c2gmsk version / bitrate control + versionid codes
// Version 20130324: convert into .so shared library


// c2_fec.h
// Implements a 1/3 FEC decoder based on "best of 3" principle (1/3 repetion code)


// fec13 decode: old version, compair bit per bit. Not used anymore, just kept as reference 
//int fec13decode(unsigned char in1, unsigned char in2, unsigned char in3, unsigned char * ret) {
//int errors=0;
//int loop;
//int ones;
//unsigned char p;

//// clear return value
//*ret=0x00;

//for (loop=0; loop <= 7; loop++) {
//	p=bit2octet[loop];
//	ones=0;
//
//	// count number of ones in position dependent on 
//	if (in1 & p) { ones++; }; // end if
//	if (in2 & p) { ones++; }; // end if
//	if (in3 & p) { ones++; }; // end if
//
//	if ((ones == 1) || (ones == 2)) {
//		errors++;
//	}; // end if
//
//	// set output bit to 1 if at least two input bits are 1 for that position
//	if (ones >= 2) {
//		*ret |= p;
//	}; // end if
//
//}; // end for
//
//
//return(errors);
//
//}; // end if

//////////////////// NEW VERSION .. HOPEFULLY A BIT FASTER

// BASIC IDEA
// INPUT: B1, B2, B3
// TMP DATA: D1, D2

// 1/ first: assume B1 is correct
// 2/ compare B1 with B2 (exor) -> D1: 0: bits are equal, D1: 1: bits are different
// 3/ compare B1 with B3 (exor) -> D2: 0: bits are equal, D2: 1: bits are different
// 4/ if D1 and D2 are both 1 (AND), B1 is NOT correct -> invert it (exor)

// By doing it with bit logic; this can be done 8 bits or 32 bits at a time

uint8_t fec13decode_8bit(uint8_t in1, uint8_t in2, uint8_t in3, uint8_t * ret) {
uint8_t d1, d2;
uint8_t errors;
uint8_t t;

t=in1;

d1=t ^ in2;

d2=t ^ in3;

errors= d1 & d2;
*ret = t ^ errors;
return(errors);
}; // end FUNCTION fec13decode_8bit


uint32_t fec13decode_32bit(uint32_t in1, uint32_t in2, uint32_t in3, uint32_t * ret) {
uint32_t d1, d2;
uint32_t errors;
uint32_t t;


t=in1;

d1=t ^ in2;
d2=t ^ in3;

errors= d1 & d2;
*ret = t ^errors;

return(errors);
}; // end FUNCTION fec13decode_32bit

int count1s_8bit(uint8_t in) {
if (in) {
#ifdef __GCC__
	return(__builtin_popcount(in));
#else
	int loop;

	int c=0;
	for (loop=0; loop<8; loop++) {
		if (in & 0x01) {
			c++;
		}; // end if
		in>>=1;
	}; // end for
	return(c);
#endif
} else {
	return(0);
}; // end else - if
}; // end  function count1s_8bit



int count1s_int(uint32_t in) {
// GCC has support for "popcount", which will be executed in hardware if present
if (in) {
#ifdef __GCC__
	return(__builtin_popcount(in));
#else
	int loop;

	int numbits=sizeof(int)<<3; // numbits = number of octets * 8

	int c=0;
	for (loop=0; loop<numbits; loop++) {
		if (in & 0x01) {
			c++;
		}; // end if
		in>>=1;
	}; // end for
	return(c);
	
} else {
	return(0);
}; // end else - if
#endif
}; // end  function count1s_int


int findbestmatch (int tofind, unsigned int table[], int tablesize, int mask, int * mindistance) {
// find best match in table
// used to find correct versionid in table of possible codes
// return if match with less then "mindistance" found
// return distance in "mindistance"

int loop;
int diff;
int diffcount;

int min;
int minindex=0;

min=(sizeof(int) << 3); // possible largest value of "min" van all 1's, i.t. number of
										// octets * 8
for (loop=0; loop < tablesize; loop++) {
	diff=((tofind ^ table[loop]) & mask);

	diffcount=count1s_int(diff);

	if (diffcount <= *mindistance) {
		// return distance and index of found value
		*mindistance = diffcount;
		return(loop);
	}; // we have a match !

	// no match, check if less then minimum
	if (diffcount < min) {
		min=diffcount;
		minindex=loop;
	}; // end if

}; // end if

// done: return
*mindistance=min;
return(minindex);

}; // end if
