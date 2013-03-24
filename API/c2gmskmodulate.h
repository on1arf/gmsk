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
// version 20130310 initial release
// Version 20130314: API c2gmsk version / bitrate control + versionid codes
// Version 20130324: convert into .so shared library


// c2gmsk highlevel modulate functions

// functions: 
// c2gmskabuff48k_modulatebits
// c2gmsk_mod_audioflush


// ADD TO BUFFER
int c2gmskabuff48k_modulatebits (struct c2gmsk_session * sessid, unsigned char * in, int nbits, int orderinvert) {
// modulate bits, add to m_audio buffer

// local vars
int ret;
int bitcounter;
int loop;

unsigned char * p_in;
unsigned char * andbit, *andbit_begin;
const unsigned char bitsl2r[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};  // left to right
const unsigned char bitsr2l[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};      // right to left

// audio returns
// we receive 10 audio samples per bit (4800 bps gmsk stream @ 4800 Khz audio sampling)
int16_t audioret[10]; // 

// return is no bits to modulate
if (nbits <= 0) {
	return(C2GMSK_RET_OK);
}; // end if


// do we go left-to-right or right-to-left?
if (orderinvert) {
	andbit_begin=(unsigned char *)bitsl2r;
} else {
	andbit_begin=(unsigned char *)bitsr2l;
}; // end if


// if number of samples is 0 or less, just return
if (nbits <= 0) {
	return(C2GMSK_RET_OK);
}; // end if

// sanity checks
// check audio buffers
if (!in) {
	return(C2GMSK_RET_NOVALIDDATA);
}; // end if

// check if sessid really points to session
ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if


// check signature of chain
ret=checksign_chain(sessid->m_chain);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if


// check signature of audio buffer
ret=checksign_abuff_48k(&sessid->m_abuff);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if


andbit=andbit_begin;
bitcounter=0;
// set pointer to beginning of data
p_in=in;

for (loop=0; loop < nbits; loop++) {
	if ((*p_in) & (*andbit)) {
		ret=modulate1bit(sessid,1,audioret);
	} else {
		ret=modulate1bit(sessid,0,audioret);
	}; // end else - if

	if (ret != C2GMSK_RET_OK) {
		return(ret);
	}; // end if

	// write audio to audio-buffer
	ret=c2gmskabuff48_add(&sessid->m_abuff,audioret,10,sessid->m_chain);

	if (ret != C2GMSK_RET_OK) {
		return(ret);
	}; // end if
        
	// move up and wrap around at 8
	bitcounter++;
	bitcounter &= 0x07;
        
	if (bitcounter == 0) {
		// bitcounter has just wrapped, so we are at a multiple of 8
		// move up "in" pointer;
		p_in++;

		// reset AND bit pattern
		andbit=andbit_begin;
                                
	} else {
		// not wrapped, so just move up to next AND bit pattern
		andbit++;
	}; // end if
}; // end for (nbits)



// done: success
return(C2GMSK_RET_OK);
}; // end function abuffer add



/////////////////////////////////////////////////
// flush audio-buffers of modulation chain
/////////////////////////////////////////////////
int c2gmsk_mod_audioflush (struct c2gmsk_session * sessid, struct c2gmsk_msgchain ** out) {
// local vars
int ret;

// sanity checks
ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// reinit chain
ret=c2gmskchain_reinit(sessid->m_chain, C2GMSKCHAIN_DEFAULTSIZE_MOD);

if (ret != C2GMSK_RET_OK) {
	// something went wrong, exit
	return(ret);
}; // end if


ret=c2gmskabuff48_flush(&sessid->m_abuff,sessid->m_chain);

if (ret != C2GMSK_RET_OK) {
	// something went wrong, exit
	return(ret);
}; // end if


// done, OK
*out=sessid->m_chain;
return(C2GMSK_RET_OK);

}; 


