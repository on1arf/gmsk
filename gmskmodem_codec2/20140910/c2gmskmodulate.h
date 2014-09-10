// c2gmskmodulate.h

//////////////////////
// API version of the GMSK modem for 10m / VHF / UHF communication
// using codec2
// version 4800/0: 4800 bps, 1/3 repetition code FEC
// version 2400/15: 2400 bps, golay code FEC


/* Copyright (C) 2013 Kristoff Bonne ON1ARF

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 2.1, as
  published by the Free Software Foundation.  This program is
  distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
  License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.
*/


// Release information
// version 20130310 initial release
// Version 20130314: API c2gmsk version / bitrate control + versionid codes
// Version 20130324: convert into .so shared library
// Version 20130614: raw gmsk output


// c2gmsk highlevel modulate functions

// functions: 
// c2gmskabuff48k_modulatebits
// c2gmsk_mod_audioflush
// c2gmsk_mod_gmskflush
// c2gmsk_mod_outputflush


// ADD TO BUFFER
// version 20130516: remove bitorder
int c2gmskabuff48k_modulatebits (struct c2gmsk_session * sessid, unsigned char * in, int nbits) {
// modulate bits, add to m_audio buffer

// local vars
int ret;
int bitcounter;
int loop;

unsigned char * p_in;
unsigned char * andbit;
const unsigned char bits[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};      // right to left
int samplesperbit=10;

// audio returns
// we receive 10 audio samples per bit (4800 bps gmsk stream @ 4800 Khz audio sampling)
// we receive 20 audio samples per bit (2400 bps gmsk stream @ 4800 Khz audio sampling)
// so make buffer big enough for both
int16_t audioret[20]; // 

// return is no bits to modulate
if (nbits <= 0) {
	return(C2GMSK_RET_OK);
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
ret=checksign_abuff_48k(sessid->m_abuff);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if


andbit=(unsigned char *)bits;
bitcounter=0;
// set pointer to beginning of data
p_in=in;


if (sessid->m_bitrate == C2GMSK_MODEMBITRATE_2400) {
	// 2400 bps @ 48000 samples/sec = 20 samples / bit
	samplesperbit=20;
} else {
	// 4800 bps @ 48000 samples/sec = 20 1amples / bit
	samplesperbit=10;
}; // end if

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
	ret=c2gmskabuff48_add(sessid->m_abuff,audioret,samplesperbit,sessid->m_chain);

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
		andbit=(unsigned char *)bits;
                                
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



// some additional checks
// modulator disabled?
if (sessid->m_disabled != C2GMSK_NOTDISABLED) {
	return(C2GMSK_RET_OPERATIONDISABLED);
}; // end if

// just return if no audio buffer
if (!sessid->m_abuff) {
	return(C2GMSK_RET_OK);
}; // end if

// reinit chain
ret=c2gmskchain_reinit(sessid->m_chain, C2GMSKCHAIN_DEFAULTSIZE_MOD);

if (ret != C2GMSK_RET_OK) {
	// something went wrong, exit
	return(ret);
}; // end if


ret=c2gmskabuff48_flush(sessid->m_abuff,sessid->m_chain);
if (ret != C2GMSK_RET_OK) {
	// something went wrong, exit
	return(ret);
}; // end if


// done, OK
*out=sessid->m_chain;
return(C2GMSK_RET_OK);

}; // end function c2gmsk_mod_audioflush


/////////////////////////////////////////////////
// flush gmsk-buffers of modulation chain
/////////////////////////////////////////////////
int c2gmsk_mod_gmskflush (struct c2gmsk_session * sessid, struct c2gmsk_msgchain ** out) {
// local vars
int ret;

// sanity checks
ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// some additional checks
// modulator disabled?
if (sessid->m_disabled != C2GMSK_NOTDISABLED) {
	return(C2GMSK_RET_OPERATIONDISABLED);
}; // end if

// just return if no gmsk buffer
if (!sessid->m_gbuff) {
	return(C2GMSK_RET_OK);
}; // end if


// reinit chain
ret=c2gmskchain_reinit(sessid->m_chain, C2GMSKCHAIN_DEFAULTSIZE_MOD);

if (ret != C2GMSK_RET_OK) {
	// something went wrong, exit
	return(ret);
}; // end if


ret=c2gmskgbuff_flush(sessid->m_gbuff,sessid->m_chain);

if (ret != C2GMSK_RET_OK) {
	// something went wrong, exit
	return(ret);
}; // end if

// done, OK
*out=sessid->m_chain;
return(C2GMSK_RET_OK);

}; // end function c2gmsk_mod_gmskflush


///////////////////////////////////////////////////////////////
// flush output buffers (abuff or gbuff) of modulation chain
//////////////////////////////////////////////////////////////
int c2gmsk_mod_outputflush (struct c2gmsk_session * sessid, struct c2gmsk_msgchain ** out) {
// local vars
int ret;

// sanity checks
ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// some additional checks
// modulator disabled?
if (sessid->m_disabled != C2GMSK_NOTDISABLED) {
	return(C2GMSK_RET_OPERATIONDISABLED);
}; // end if

if (sessid->outputformat == C2GMSK_OUTPUTFORMAT_AUDIO) {
// outputformat = AUDIO
	ret=c2gmsk_mod_audioflush (sessid, out);

	// return is not OK
	if (ret != C2GMSK_RET_OK) {
		return(ret);
	}; // end if

} else if (sessid->outputformat == C2GMSK_OUTPUTFORMAT_GMSK) {
// outputformat = GMSK
	ret=c2gmsk_mod_gmskflush (sessid, out);

	// return is not OK
	if (ret != C2GMSK_RET_OK) {
		return(ret);
	}; // end if

}; // end elsif - if

*out=sessid->m_chain;
return(C2GMSK_RET_OK);

}; // end function c2gmsk_mod_gmskflush


