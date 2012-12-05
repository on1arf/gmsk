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
int datasize=7; // default, datasize = 7, for codec2@1400bps (7 char / 40 ms)
int totalsize;

// structure for raw codec2 data
c2encap c2e_begin, c2e_voice, c2e_end; 
unsigned char * p_c2e_voice;


// if 1st argument exists, use it as capture device
if (argc >= 2) {
	if (argv[1][0] == '6') {
		datasize=6;
	} else {
		datasize=7;
	}; // end if
} else {
	datasize=7;
}; //


totalsize = datasize + 4; // add 4 for header

// init c2encap structures
memcpy(c2e_begin.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2e_begin.header[3]=C2ENCAP_MARK_BEGIN;
memcpy(c2e_begin.c2data.c2data_text3,"BEG",3);

memcpy(c2e_end.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2e_end.header[3]=C2ENCAP_MARK_END;
memcpy(c2e_end.c2data.c2data_text3,"END",3);

memcpy(c2e_voice.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2e_voice.header[3]=C2ENCAP_DATA_VOICE1400;


p_c2e_voice = c2e_voice.c2data.c2data_data7; // begin of data-structure



// get first packet
ret=fread(p_c2e_voice,datasize,1,stdin);

// do no a successfull read, stop here
if (ret <= 0) {
	fprintf(stderr,"Error: could not read from input! \n");
	exit(-1);
}; // end if

// write begin frame
ret=fwrite(&c2e_begin, C2ENCAP_SIZE_MARK, 1, stdout);
fflush(stdout);

if (ret < 1) {
	fprintf(stderr,"Error: could not write begin-marker! \n");
	exit(-1);
}; // end if

while (ret >= 1) {
	// write data
	ret=fwrite(&c2e_voice, totalsize, 1, stdout);
	fflush(stdout);


	if (ret < 1) {
		fprintf(stderr,"Error: could not write voice frame! \n");
		exit(-1);
	}; // end if


	// read next frame
	ret=fread(p_c2e_voice,datasize,1,stdin);
}; // end while


// write end frame
ret=fwrite(&c2e_end, C2ENCAP_SIZE_MARK, 1, stdout);
fflush(stdout);

if (ret < 1) {
	fprintf(stderr,"Error: could not write end-marker! \n");
	exit(-1);
}; // end if

return(0);
}; // end main application
