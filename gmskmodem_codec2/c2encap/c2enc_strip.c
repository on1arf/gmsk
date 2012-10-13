/* c2enc strip encapsulation */

// removes c2enc headers, extracts codec2 headers and streams out

// for memcpy
#include "string.h"

// for fprintf
#include "stdio.h"

// for malloc
#include <stdlib.h>

#define TRUE 1

// defines and structures for codec2-encapsulation
#include "c2encap.h"

int main (int argc, char ** argv) {
int ret;
int strict;
int status; // 0: wait for start, 1: receiving data
char typeofdata;
int datasize=0;
int c2encap_headerfound=0;

int stop;

// structure for raw codec2 data
c2encap *c2e_marker, *c2e_data; // note, c2e_mark and c2e_data will point
										// to same place

char * p_c2e_data;


// if 1st argument exists, use it for "strict" or not
if (argc >= 2) {
	if ((argv[1][0] == 's') || (argv[1][0] == 'S')) {
		strict=1;
	} else {
		strict=0;
	}; // end if
} else {
	strict=0;
}; //

// init c2encap structures
c2e_marker=malloc(sizeof(c2encap));

if (!c2e_marker) {
	fprintf(stderr,"Error: could not allocate memory for c2_encap structure");
}; // end if

c2e_data = c2e_marker; // c2_data and c2_marker point to same memory structure

p_c2e_data = c2e_data->c2data.c2data_text3; // begin of data-structure


status=0;
stop=0;

while (!stop) {

	// get c2enc header (4 chars)
	ret=fread(c2e_marker, 4, 1, stdin);

	if (ret < 1) {
		// something went wrong, break out
		break;
	}; // end if


	// read data until valid header found
	c2encap_headerfound=0;

	while (!c2encap_headerfound) {
		unsigned char *p1, *p2;

		// compair header first 3 chars
		if (!memcmp(c2e_marker,C2ENCAP_HEAD,3)) {
			// OK, found it, break out of loop
			c2encap_headerfound=1;
			break;
		}; // end if

		// not good, move up one char
		fprintf(stderr,"Warning: C2ENCAP frame received with invalid signature: %02X%02X%02X \n",c2e_marker->header[0],c2e_marker->header[1],c2e_marker->header[2]);

		p1=&c2e_marker->header[0], p2=p1+1;

		*p1=*p2; p1++; p2++;
		*p1=*p2; p1++; p2++;
		*p1=*p2; 

		// read one more octet, replacing last char
		ret=fread(p2, 1, 1, stdin);

		if (ret < 1) {
			fprintf(stderr,"Warning: unexpected end of file\n");
			break;
		}; // end if

	}; // end while (header found)

	// success ?

	if (!c2encap_headerfound) {
		// nope, we broke out for error condition, break out completely
		stop=1;
		break;
	}; // end if
	
	// read rest of data, depending on type of data

	typeofdata=c2e_data->header[3];

	if ((typeofdata == C2ENCAP_MARK_BEGIN)  || (typeofdata == C2ENCAP_MARK_END)) {
		// get c2enc data for "MARKER" type (3 chars)
		datasize=3;
		ret=fread(p_c2e_data, datasize, 1, stdin);

		if (ret < 1) {
			fprintf(stderr,"Warning: unexpected end of file\n");
			stop=1;
			break;
		}; // end if
	} else if (typeofdata == C2ENCAP_DATA_VOICE1400){
		// get c2enc data for "VOICE 1400 bps" type (7 chars)
		datasize=7;
		ret=fread(p_c2e_data, datasize, 1, stdin);

		if (ret < 1) {
			fprintf(stderr,"Warning: unexpected end of file\n");
			stop=1;
			break;
		}; // end if
	} else if ((typeofdata == C2ENCAP_DATA_VOICE1200) || (typeofdata == C2ENCAP_DATA_VOICE2400)){
		// get c2enc data for "VOICE 1200 bps" or "VOICE 2400 bps" type (6 chars)
		datasize=6;
		ret=fread(p_c2e_data, datasize, 1, stdin);

		if (ret < 1) {
			fprintf(stderr,"Warning: unexpected end of file\n");
			stop=1;
			break;
		}; // end if

	} else {
		fprintf(stderr,"Warning: Unknown type-of-data type %02X \n",typeofdata);
		// continue, hope that the "header-pattern search" can get the stream back on track
		continue;
	}; // end else - elsif - if


	// OK, we now have a valid packet
	if (!strict) {
		// not strict, just ignore markers, and send data as long as we receive data
		if ((typeofdata == C2ENCAP_DATA_VOICE1200) || (typeofdata == C2ENCAP_DATA_VOICE2400) || (typeofdata == C2ENCAP_DATA_VOICE1400)){
			fwrite(p_c2e_data,datasize,1,stdout);
			fflush(stdout);
		}; // end if

		// go to next frame
		continue;

	} else {
		// strict, check status
		if (status == 0) {
			// status is 0 (waiting for start),
			if (typeofdata == C2ENCAP_MARK_BEGIN) {
				// move up to status 1
				status=1;
			} else {
				fprintf(stderr,"Warning: Unexpected type-of-data type %02X received when in status 0. Ignoring\n",typeofdata);
			}; // end if
		} else if (status == 1) {
			// status is 1, receiving data
			if ((typeofdata == C2ENCAP_DATA_VOICE1200) || (typeofdata == C2ENCAP_DATA_VOICE2400) || (typeofdata == C2ENCAP_DATA_VOICE1400)){
				// received voice, send it out
				fwrite(p_c2e_data,datasize,1,stdout);
				fflush(stdout);
			} else if (typeofdata == C2ENCAP_MARK_END){
				// received end marker, reset status
				status=0;
			} else {
				fprintf(stderr,"Warning: Unexpected type-of-data type %02X received when in status 1. Ignoring\n",typeofdata);
			}; // end if
		} else {
			fprintf(stderr,"Warning: Unknown status level= %d. Resetting to status 0 \n",status);
			status=0;
		}; // end if

		// go to next frame
		continue;
		
	}; // end if

}; // end while (stop)

return(0);
}; // end program
