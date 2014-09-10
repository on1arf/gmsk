//////////////////////
// API version of the GMSK modem for 10m / VHF / UHF communication
// using codec2
// version 4800/0 (versionid 0x27f301): 4800 bps, 1/3 repetition code FEC
// version 2400/15 (versionid 0x1d499b): 2400 bps, interleave, golay FEC


// The low-level DSP functions are largly taken from the
// pcrepeatercontroller project written by
// Jonathan Naylor, G4KLX
// More info: http://groups.yahoo.com/group/pcrepeatercontroller


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



// gmskmodemAPI.c
// main functions of API for modulation and demodulation

// Release information
// version 20130310: initial release
// Version 20130314: API c2gmsk version / bitrate control + versionid codes
// Version 20130324: convert into .so shared library
// Version 20130506: added 2400 bps modem version 15 (0x1D499B)
// Version 20130601: added support for golay FEC in modem 2400/15
// Version 20130606: added support for auxiliary data in modem 2400/15
// Version 20130614: added support for raw gmsk input / output


//////////////////////

// some defines used for local data
#define C2GMSKCHAIN_DEFAULTSIZE_MOD  800
#define C2GMSKCHAIN_DEFAULTSIZE_DEMOD 200

// some defines for sanity checks
#define C2GMSKCHAIN_MAXSIZE 1048576 // 1 MB


// if nothing defined, use FLOAT
#ifndef _USEFLOAT 
	#ifndef _INTMATH
		#define _USEFLOAT 1
	#endif
#endif

// for int16_t
#include <stdint.h>

// for malloc
#include <stdlib.h>

// for memcpy
#include <string.h>

/////////////////////
#include "stdio.h"

// for assert
#include "assert.h"


// global definitions (inherented from non-API version, modified for API)
#include "global.h"

// include definitions
#include "gmskmodemapi.h"

// functions
// DSP and low-level related functions (inherented from non-API version, modified for API)
#include "dspstuff_4800.h" // gmsk modulation filters for 4800 bps
#include "dspstuff_2400.h" // gmsk modulation filters for 2400 bps (uses demodulator filter of 4800 bps modem) 
#include "gmskmodulate.h" // modulate1bit

// functions for pattern matching
#include "countdiff.h"

// table of c2gmsk versionid-mapping
#include "c2_vercode.h"

// functions dealing internal structure and queing functions
#include "c2gmskdebugmsg.h" // support functions for debug message queuing
#include "c2gmsksess.h" // support functions for sessions ("new" and "destroy")
#include "c2gmskchain.h" // functions for memory chains
#include "c2gmskabuff.h" // functions for 40ms audio buffers
#include "c2gmskmodulate.h" // functions for audio buffers gmsk modulation
#include "c2gmskprintbit.h" // support functions for printbit functions
#include "c2gmskstr.h" // text version of defines
#include "c2gmskgbuff.h" // GMSK bitstream buffers

#include "c2gmsksupport.h" // C2GMSK user-level API SUPPORT FUNCTIONS


#include "c2_interleave.h" // interleaving matrix for 1400 bps voice
#include "c2_scramble.h" // scramble matrix for 1400 bps voice
#include "c2_fec.h" // fec1/3 repetition code

// for golay FEC
#include "golay23.h"

// for auxiliary data
#include "c2_auxdata.h"


///////////////////////////////////////////////
//// MODULATION CHAIN:
//// c2gmsk_mod_init
//// c2gmsk_mod_start
//// c2gmsk_mod_voice1400
//// c2gmsk_mod_voice1400_end

int c2gmsk_mod_voice1400 (struct c2gmsk_session *sessid, unsigned char *c2dataframe, struct c2gmsk_msgchain **out) {
// MODULATE CHAIN. 1400 bps voice frame

unsigned char outbuffer[24]; // 192 bits for 4800 bps modem
unsigned char outbuffer2[12]; // 96 bits for 2400 bps modem
int ret;
int loop;

unsigned char *c1, *c2;

// sanity checks
ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// check "disabled" flag
if (sessid->m_disabled == C2GMSK_DISABLED) {
	return(C2GMSK_RET_OPERATIONDISABLED);
}; // end if

if (!out) {
	return(C2GMSK_RET_NOVALIDRETPOINT);
}; // end if


// reinit chain ... this will also reinit the search parameters for this chain
ret=c2gmskchain_reinit(sessid->m_chain, C2GMSKCHAIN_DEFAULTSIZE_MOD);
if (ret != C2GMSK_RET_OK) {
	// something went wrong, exit
	return(ret);
}; // end if


// check status, "modulate voice" should only be called in state = 0
if (sessid->m_state != 1) {
	return(C2GMSK_RET_BADSTATE);
}; // end if


// some error checking
assert((sessid->m_versrate == 0x0020) || (sessid->m_versrate == 0x001f));

// 4800 bps / version 0 code
if (sessid->m_versrate == 0x0020) {

	// first 3 chars (sync pattern) are "1110 0111  0000 1000  0101 1100" if not end
	outbuffer[0]=0xe7; outbuffer[1]=0x08; outbuffer[2]=0x5c;


	// rest is 3 times codec2 frame @ 1400 bps
	// octets 3 up to 9, 10 up to 16 and 17 up to 23 contain copy of codec2 frame
	memcpy(&outbuffer[3],c2dataframe,7);

	// copy with interleaving
	for (loop=0; loop<=6; loop++) {
		outbuffer[interl2[loop]]=c2dataframe[loop];
	}; // end for

	// copy with interleaving
	for (loop=0; loop<=6; loop++) {
		outbuffer[interl3[loop]]=c2dataframe[loop];
	}; // end for

	// apply scrambling to make stream more random

	// we have up to 8 known exor-patterns
	sessid->m_v1400_framecounter &= 0x7;

	c1=&scramble_exor[sessid->m_v1400_framecounter][0];
	// scrambling chars 3 up to 24 (starting at 0)
	c2=&outbuffer[3];

	// scramble 21 chars
	for (loop=0; loop < 21; loop++) {
		*c2 ^= *c1;
		c2++; c1++;
	}; // end for

	sessid->m_v1400_framecounter++; // no need to do range-checking, is done above on next iteration

	// audio or gmskbits?
	if (sessid->outputformat == C2GMSK_OUTPUTFORMAT_AUDIO) {
		// audio
		// modulate frame + add to queue
		ret=c2gmskabuff48k_modulatebits(sessid, outbuffer, 192);
	} else {
		// GMSK
		ret=c2gmskgbuff_add(sessid, outbuffer, 192);
	}; // end else - if

	if (ret != C2GMSK_RET_OK) {
		return (ret);
	}; // end if

} else if (sessid->m_versrate == 0x001f) {
	int ret;
	// 2400 bps / version 15

	// prepare frame
	int scramblestart=3;

	// char for auxiliary data
	unsigned char c;

	// framecount: used to select scrambling pattern and if a sync pattern needs to be send or not
	// can go from 0 to 7: 
	sessid->m_v1400_framecounter &= 0x7;

	// char auxdata
	ret=auxdata_send1(sessid,&c);

	if (ret != C2GMSK_RET_OK) {
		return(ret);
	}; // end if

	// STEP 1 and 2: interleaving (for better FEC) + FEC
	if (sessid->m_v1400_framecounter) {
		// not first frame of group of 8: no sync

		// copy and interleave voice bits
		c2_interleave_2400_1400(c2dataframe,outbuffer,0,1);
		scramblestart=0;

		// apply FEC
		// frame without SYNC
		// bits 12 to 23 are golay code for bits  0 to 11
		// bits 36 to 47 are golay code for bits 24 to 35
		// bits 60 to 71 are golay code for bits 48 to 59

		golay23_encode(outbuffer); // bits 0 to 23
		golay23_encode(&outbuffer[3]); // bits 24 to 47
		golay23_encode(&outbuffer[6]); // bits 48 to 71
		

		// store auxdata
		// not a sync frame, auxdata is at positions 92 to 95
		// first clear 4 MSB bits (just to be sure), then store data into that place
		outbuffer[11] &= 0x0f;
		outbuffer[11] |= ((c & 0x0f) << 4);

	} else {
		// first frame of group of 8: include sync

		// copy sync pattern to first 3 octets
		outbuffer[0]=0xe7; outbuffer[1]=0x08; outbuffer[2]=0x5c;

		// copy and interleave voice bits
		c2_interleave_2400_1400(c2dataframe,&outbuffer[3],1,1);
		scramblestart=3;


		// apply FEC
		// frame with SYNC
		// bits 36 to 47 are golay code for bits  0 to 11 (first part after sync)
		golay23_encode(&outbuffer[3]); // bits 24 to 35

		// store auxdata
		// not a sync frame, auxdata is at positions 68 to 71
		// first clear 4 MSB bits (just to be sure), then store data into that place
		outbuffer[8] &= 0x0f;
		outbuffer[8] |= ((c & 0x0f) << 4);
	}; // end else - if

	// STEP 3: SCRAMBLING


	// apply scrambling to make stream more random
	// do not apply scrambling to 1st 3 char if sync pattern present
	c1=&scramble_exor[sessid->m_v1400_framecounter][scramblestart];
	c2=&outbuffer[scramblestart];

	// scramble up to 12th chars
	for (loop=scramblestart; loop < 12; loop++) {
		*c2 ^= *c1;
		c2++; c1++;
	}; // end for


	// STEP 4:
	// interleave bits for transmission

	if (sessid->m_v1400_framecounter) {
	// not frame 0 -> no sync
		interleaveTX_tx(outbuffer,outbuffer2,0);
	} else {
	// frame 0 -> sync
		// modulate frame + add to queue
		interleaveTX_tx(outbuffer,outbuffer2,1);
	}; // end else - if

	// audio or gmskbits?
	if (sessid->outputformat == C2GMSK_OUTPUTFORMAT_AUDIO) {
		// audio
		// modulate frame + add to queue
		ret=c2gmskabuff48k_modulatebits(sessid, outbuffer2, 96);
	} else {
		// GMSK
		ret=c2gmskgbuff_add(sessid, outbuffer2, 96);
	}; // end else - if


	if (ret != C2GMSK_RET_OK) {
		return (ret);
	}; // end if

	sessid->m_v1400_framecounter++; // no need to do range-checking, is done above on next iteration

}; // end else - if

// point main application to chain
*out=sessid->m_chain;

// done
return(C2GMSK_RET_OK);

}; // end function c2gmsk modulate 1400 voice frame


int c2gmsk_mod_voice1400_end (struct c2gmsk_session *sessid, struct c2gmsk_msgchain **out) {
// MODULATE CHAIN. 1400 bps voice frame

unsigned char outbuffer[24];
int ret;
int loop;



// sanity checks
ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// check "disabled" flag
if (sessid->m_disabled == C2GMSK_DISABLED) {
	return(C2GMSK_RET_OPERATIONDISABLED);
}; // end if

if (!out) {
	return(C2GMSK_RET_NOVALIDRETPOINT);
}; // end if


// reinit chain ... this will also reinit the search parameters for this chain
ret=c2gmskchain_reinit(sessid->m_chain, C2GMSKCHAIN_DEFAULTSIZE_MOD);

if (ret != C2GMSK_RET_OK) {
	// something went wrong, exit
	return(ret);
}; // end if

// check status, "modulate voice end" should only be called in state = 1
if (sessid->m_state != 1) {
	return(C2GMSK_RET_BADSTATE);
}; // end if



// some error checking
assert((sessid->m_versrate == 0x0020) || (sessid->m_versrate == 0x001f));

// 4800 bps / version 0 code
if (sessid->m_versrate == 0x0020) {
	// version/rate = 0x20 : 4800 bps, version 0

	// we send up to 3 end-frames
	// common part for all 3 frames

	// first 3 chars (sync pattern) are "0111 1110  1000 0000  1100 0101" 
	outbuffer[0]=0x7e; outbuffer[1]=0x80; outbuffer[2]=0xc5;

	for (loop=0; loop<3; loop++) {

		// rest is 3 times codec2 frame @ 1400 bps, as we create a dummy record, all data will be zero
		// however, as we will need to "exor" it with the scrambling pattern, we can just copy
		// the scrambling pattern to the data

		// we have up to 8 known exor-patterns
		sessid->m_v1400_framecounter &= 0x7;

		memcpy(&outbuffer[3], &scramble_exor[sessid->m_v1400_framecounter][0],21);

		// modulate frame + add to queue
		// audio or gmskbits?
		if (sessid->outputformat == C2GMSK_OUTPUTFORMAT_AUDIO) {
			// audio
			// modulate frame + add to queue
			ret=c2gmskabuff48k_modulatebits(sessid, outbuffer, 192);
		} else {
			// GMSK
			ret=c2gmskgbuff_add(sessid, outbuffer, 192);
		}; // end else - if

		if (ret != C2GMSK_RET_OK) {
			return (ret);
		}; // end if

		sessid->m_v1400_framecounter++;
	}; // end for

} else {
	// version/rate = 0x1f : 2400 bps, version 15


	// rest is 3 times codec2 frame @ 1400 bps, as we create a dummy record, all data will be zero
	// however, as we will need to "exor" it with the scrambling pattern, we can just copy
	// the scrambling pattern to the data

	// set framecounter to "0" (sync is send)

	// copy scramble patterns 3 up to 11 
	// then also apply transmission interleaving

	// combine these two actions in one
	// interleave bits for transmission:
	interleaveTX_tx(&scramble_exor[0][0],outbuffer,1);

	// first 3 chars (sync pattern) are "0111 1110  1000 0000  1100 0101" 
	outbuffer[0]=0x7e; outbuffer[1]=0x80; outbuffer[2]=0xc5;


	// modulate frame + add to queue
	// do 3 times (we send end-frame 3 times)
	for (loop=0; loop<3; loop++) {
		// audio or gmskbits?
		if (sessid->outputformat == C2GMSK_OUTPUTFORMAT_AUDIO) {
			// audio
			// modulate frame + add to queue
			ret=c2gmskabuff48k_modulatebits(sessid, outbuffer, 96);
		} else {
			// GMSK
			ret=c2gmskgbuff_add(sessid, outbuffer, 96);
		}; // end else - if

		if (ret != C2GMSK_RET_OK) {
			return (ret);
		}; // end if
	}; // end for


}; // end else - if

// change state: send change state message
ret=queue_m_msg_2(sessid,C2GMSK_MSG_STATECHANGE,1,0); // move from state 1 to 0
if (ret != C2GMSK_RET_OK) {
	// reinit state, even in case of error
	sessid->m_state=0;
	return(ret);
}; // end if

// reinit state
sessid->m_state=0;


// point main application to chain
*out=sessid->m_chain;

// done
return(C2GMSK_RET_OK);


}; // end function c2gmsk modulate voice1400 END


/////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

int c2gmsk_mod_init (struct c2gmsk_session * sessid, struct c2gmsk_param * param) {
int ret;

// sanity checks
ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// check "disabled" flag
if (sessid->m_disabled == C2GMSK_DISABLED) {
	return(C2GMSK_RET_OPERATIONDISABLED);
}; // end if

ret=checksign_param (param);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if




// version and mode check:

// As c2gmsk_mod_init is called from sess_new, is it not said that modulation type
// is actually used.
// In this part of the code, the version/mode check is only done if either is enabled
// the check if the modulation is enabled is done in "mod_start"

if ((param->m_version < 0) || (param->m_version > 15)) {
	return(C2GMSK_RET_UNSUPPORTEDVERSIONID);
}; // end if

if ((param->m_bitrate != C2GMSK_MODEMBITRATE_2400) && (param->m_bitrate != C2GMSK_MODEMBITRATE_4800)) {
	return(C2GMSK_RET_UNSUPPORTEDMODEMBITRATE);
}; // end if


if ((param->d_disabled != C2GMSK_DISABLED) && ((param->d_bitrate != C2GMSK_MODEMBITRATE_2400) && (param->d_bitrate != C2GMSK_MODEMBITRATE_4800))) {
	return(C2GMSK_RET_UNSUPPORTEDMODEMBITRATE);
}; // end if


// check output format, should "0" (audio) or "1" (raw gmsk)
if ((param->outputformat != C2GMSK_OUTPUTFORMAT_AUDIO) && (param->outputformat != C2GMSK_OUTPUTFORMAT_GMSK)) {
	return(C2GMSK_RET_UNSUPPORTEDOUTPUTFORMAT);
}; // end if


// init audio-buffer or gmsk-buffer if it does not exist yet
if ((sessid->m_gbuff==NULL) && (sessid->m_abuff==NULL)) {
	// not yet created. Do now:

	if (sessid->outputformat == C2GMSK_OUTPUTFORMAT_GMSK) {
printf("creating m_gbuff! \n");
		// gmsk: 
		sessid->m_gbuff=malloc(sizeof(gmskbuff));

		// oeps. Failed
		if (!sessid->m_gbuff) {
			return(C2GMSK_RET_NOMEMORY);
		}; // end if

		// copy signature for gmsk stream
		//memcpy(&sessid->m_gbuff->msg.signature,GBUFF_SIGNATURE,4);
		memcpy(&sessid->m_gbuff->signature,GBUFF_SIGNATURE,4);

		// init vars
		if (param->m_bitrate == C2GMSK_MODEMBITRATE_2400) {
			sessid->m_gbuff->size=96;

			// initialise msg
			sessid->m_gbuff->msg.tod=C2GMSK_MSG_RAWGMSK_96;
			sessid->m_gbuff->msg.datasize=12;

		} else {
			sessid->m_gbuff->size=192;

			// initialise msg
			sessid->m_gbuff->msg.tod=C2GMSK_MSG_RAWGMSK_192;
			sessid->m_gbuff->msg.datasize=24;
		}; // end else - if (bitrate)

		// same for 2400 and 4800 bps
		sessid->m_gbuff->used=0; // counter is BITS
		sessid->m_gbuff->bitcount=0; // goes from 0 to 7
		sessid->m_gbuff->p_data=sessid->m_gbuff->msg.data;

		// signature for message
		memcpy(&sessid->m_gbuff->msg.signature,MSG_SIGNATURE,4);

		// clear all data
		memset(sessid->m_gbuff->p_data,0,24);

	} else {
		// audio
		sessid->m_abuff=malloc(sizeof(audiobuff40_48k));

		if (!sessid->m_abuff) {
			return(C2GMSK_RET_NOMEMORY);
		}; // end if

		// copy signature for 48kbps audio
		memcpy(sessid->m_abuff->signature,ABUFF48_SIGNATURE,4);

		// init vars
		sessid->m_abuff->used=0;
	}; // end else - if
}; // end if


// here using "ret" as tempory var for version/rate id
ret=(param->m_bitrate<<NUMVERSID_SHIFT)+param->m_version;

// current supported combinations: (bitrate = 4800, versionid = 0) and (bitrate = 2400, versionid = 15)
if ((ret != 0x001f) && (ret != 0x0020)) {
	return(C2GMSK_RET_UNSUPPORTEDVERSIONID);
}; // end if

// copy parameters from param to session vars
sessid->m_version=param->m_version;
sessid->m_bitrate=param->m_bitrate;
// combined version / rate 
sessid->m_versrate=ret;


// init all other modulator-related vars
sessid->m_state=0;
sessid->m_v1400_framecounter=0;
sessid->md_filt_init=1;

// done
return(C2GMSK_RET_OK);
}; // end function c2gmsk_mod_init


int c2gmsk_mod_start (struct c2gmsk_session *sessid, struct c2gmsk_msgchain **out) {
// MODULATE CHAIN. START stream

// local data
int ret;

unsigned char versionid[3];

// sanity checks
ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// check "disabled" flag
if (sessid->m_disabled == C2GMSK_DISABLED) {
	return(C2GMSK_RET_OPERATIONDISABLED);
}; // end if

if (!out) {
	return(C2GMSK_RET_NOVALIDRETPOINT);
}; // end if



// check status, "start modulate" should only be called in state = 0
if (sessid->m_state != 0) {
	return(C2GMSK_RET_BADSTATE);
}; // end if



// check if modulation is enabled
if ((sessid->m_version < 0) || (sessid->m_bitrate < 0)) {
	return(C2GMSK_RET_OPERATIONDISABLED);
}; // end if

// reinit chain ... this will also reinit the search parameters for this chain
ret=c2gmskchain_reinit(sessid->m_chain, C2GMSKCHAIN_DEFAULTSIZE_MOD);
if (ret != C2GMSK_RET_OK) {
	// something went wrong, exit
	return(ret);
}; // end if


// set status to 1 and queue "change of status" message
sessid->m_state=1;

ret=queue_m_msg_2(sessid,C2GMSK_MSG_STATECHANGE,0,1); // move from state 0 to 1
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// send sync pattern
assert ((sessid->m_bitrate == C2GMSK_MODEMBITRATE_2400) || (sessid->m_bitrate == C2GMSK_MODEMBITRATE_4800));
if (sessid->m_bitrate == C2GMSK_MODEMBITRATE_2400) {
	if (sessid->outputformat == C2GMSK_OUTPUTFORMAT_AUDIO) {
		// audio
		// modulate frame + add to queue
		ret=c2gmskabuff48k_modulatebits(sessid, codec2_startsync_pattern_2400, sizeof(codec2_startsync_pattern_2400)<<3);
	} else {
		// GMSK
		ret=c2gmskgbuff_add(sessid, codec2_startsync_pattern_2400, sizeof(codec2_startsync_pattern_2400)<<3);
	}; // end else - if

} else {
	if (sessid->outputformat == C2GMSK_OUTPUTFORMAT_AUDIO) {
		// audio
		// modulate frame + add to queue
		ret=c2gmskabuff48k_modulatebits(sessid, codec2_startsync_pattern_4800, sizeof(codec2_startsync_pattern_4800)<<3);
	} else {
		// GMSK
		ret=c2gmskgbuff_add(sessid, codec2_startsync_pattern_4800, sizeof(codec2_startsync_pattern_4800)<<3);
	}; // end else - if

}; // end else - if

if (ret != C2GMSK_RET_OK) {
	return (ret);
}; // end if


// send versionid NON inverted
// copy octet per octet to be endian-independant

assert((sessid->m_bitrate == C2GMSK_MODEMBITRATE_2400)  || (sessid->m_bitrate == C2GMSK_MODEMBITRATE_4800));

if (sessid->m_bitrate == C2GMSK_MODEMBITRATE_4800) {
	// 4800 bps modem
	versionid[0]=(char)(c2gmsk_idcode_4800[sessid->m_version] & 0xff);
	versionid[1]=(char)((c2gmsk_idcode_4800[sessid->m_version] & 0xff00) >> 8);
	versionid[2]=(char)((c2gmsk_idcode_4800[sessid->m_version] & 0xff0000) >> 16);
} else if (sessid->m_bitrate == C2GMSK_MODEMBITRATE_2400) {
	// 2400 bps modem
	versionid[0]=(char)(c2gmsk_idcode_2400[sessid->m_version] & 0xff);
	versionid[1]=(char)((c2gmsk_idcode_2400[sessid->m_version] & 0xff00) >> 8);
	versionid[2]=(char)((c2gmsk_idcode_2400[sessid->m_version] & 0xff0000) >> 16);
}; // end else - if

// send 24 bits
// audio or gmskbits?
if (sessid->outputformat == C2GMSK_OUTPUTFORMAT_AUDIO) {
	// audio
	// modulate frame + add to queue
	ret=c2gmskabuff48k_modulatebits(sessid, versionid, 24);
} else {
	// GMSK
	ret=c2gmskgbuff_add(sessid, versionid, 24);
}; // end else - if

if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if


// function c2gmsk_mod_start done.

// before we close, init all vars used by "modulate_voice_..."

// for mod_voice1400
sessid->m_v1400_framecounter=0;


// init auxiliary data (sender chain)
ret=auxdata_sessinit_s(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// queue nodata if nothing in queue

ret=queue_m_nodataifempty (sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if


// done. Return
*out=sessid->m_chain;
return(C2GMSK_RET_OK);
}; // end function c2gmsk_mod_start




///////////////////////////////////////////////
//// DEMODULATION CHAIN:
//// c2gmsk_demod_init
//// c2gmsk_demod (will be removed, to be replaced by c2gmsk_demod_pcm))

//// c2gmsk_demodpcm
//// c2gmsk_demodgmsk

//// internal functions:
//// c2gmsk_demod_process1bit

///////////////
// function c2gmsk_demod_process1bit
/// processes 1 bit of received data. Called by demod_pcm (after demodulation) or
/// from demod_gmsk
int c2gmsk_demod_process1bit (struct c2gmsk_session * sessid, int bit) {
int ret;

int bitmatch;
int loop;
int thisversrate;

static int init=1;
static char allzero[7];


if (init) {
	// create "all empty" (used for detecting end-frames)
	memset(allzero,0,7);

	init=0;
}; // end if

// no sanity checks done. (already done in demod_pcm or demod_gmsk)

// printall bit
ret=queue_debug_allbit (sessid, bit);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// "State" variable:
// state 0: waiting for sync "10101010 1010101" (bit sync) or
// 	"1110110 01010000" (frame sync)
// state 20: receiving codec2 speed/version id
// (state 21: receiving header ... to be added later)
// state 22: receiving codec2 main part of bitstream

///////////////////////////////////
// STATE 0
///////////////////////////////////

// state 0
if (sessid->d_state == 0) {
// some local vars
	int thisaverage;

	ret=queue_debug_bit(sessid,bit);

	if (ret != C2GMSK_RET_OK) {
		return(ret);
	}; // end if

	// keep track of last 16 bits
	sessid->d_last2octets <<= 1;

	if (bit) {
		sessid->d_last2octets |= 0x01; // set rightmost bit if input bit is set
	}; // end if


	// the syncronisation pattern is at least 64 times "01"
	if (((sessid->d_last2octets & 0xffff) == 0x5555) || ((sessid->d_last2octets & 0xffff) == 0xaaaa)) {
		sessid->d_syncreceived += 3;
	} else {
		if (sessid->d_syncreceived > 0) {
			sessid->d_syncreceived--;
		}; // end if
	}; // end if


	if (sessid->d_syncreceived <= 20) {
		// if not received enough sync pattern, get more data
		return(C2GMSK_RET_OK);
	}; // end if


	// we have received sufficient bit sync patterns, so start looking for frame sync
	// syncmask = 0x7FFF, 
	bitmatch=countdiff16_fromlsb(sessid->d_last2octets,0x7FFF,CODEC2_SYNCSIZE, CODEC2_SYNCPATTERN,BITERRORSSTARTSYN);

	// do we have a match?
	if (bitmatch) {
		// match in non-inverted 
		sessid->d_inaudioinvert=0;

	} else {
		// no match, try again with inverted bit pattern
			bitmatch=countdiff16_fromlsb(sessid->d_last2octets,0x7FFF,CODEC2_SYNCSIZE, (CODEC2_SYNCPATTERN ^ 0x7FFF), BITERRORSSTARTSYN);
			if (bitmatch) {
			sessid->d_inaudioinvert=1;
		};
	}; // end else - if


	// OK, do we not have a bitmatch, go to next audio sample
	if (!bitmatch) {
		return(C2GMSK_RET_OK);
	}; // end if


	// send debug message concerning audio-inversion
	ret=queue_d_msg_1(sessid, C2GMSK_MSG_INAUDIOINVERT, sessid->d_inaudioinvert);
	if (ret != C2GMSK_RET_OK) {
		return(ret);
	}; // end if


	// ok, we have a valid sync frame, go to state 20 (get codec2 version id)

	// Additional check: compair average audio level with average of 16 up to 31 samples
	// ago (when the signal should contain noise). The current audio-level should be at
	// least 18.75 % (13/16) below that "noise level"
	// only valid for AUDIO format
	if ((!sessid->d_disableaudiolevelcheck) && (sessid->outputformat == C2GMSK_OUTPUTFORMAT_AUDIO)) {
		int p;
		int averagevalid;
		int averagetotal;

		// init vars
		averagevalid=1;
		averagetotal=0;

		// the averagelevel table is a 32 wide ringbuffer
		// the index points (N) to the last value added in the buffer; so the value N+1
		// is actually the value N-31
		p=sessid->d_audiolevelindex;

		for (loop=0; loop<16;loop++) {
			p++;
			p &= 0x1f;

			thisaverage=sessid->d_audioleveltable[p];

			// average value = 0? -> sample data is not valid
			if (!thisaverage) {
				averagevalid=0;
			} else {
				averagetotal += thisaverage;
			}; // end if
		}; // end for


		if (averagevalid) {
			int maxlevel;
			// do check, compair average audio-level with reference value (average of samples -32 upto -16)

			maxlevel = (averagetotal >> 8) * 13; // >>8=/256: divide by 16 first (for 16 samples) and
															// then a 2nd time for get 13/16 of average total

			if (sessid->d_audioaverage < maxlevel) {
				ret=queue_d_msg_2(sessid, C2GMSK_MSG_AUDIOAVGLVLTEST, C2GMSK_AUDIOAVGLVLTEST_OK, maxlevel);

				if (ret != C2GMSK_RET_OK) {
					return(ret);
				}; // end if
			} else {
				ret=queue_m_msg_2(sessid, C2GMSK_MSG_AUDIOAVGLVLTEST, C2GMSK_AUDIOAVGLVLTEST_TOLOW, maxlevel);

				if (ret != C2GMSK_RET_OK) {
					return(ret);
					}; // end if

				return(C2GMSK_RET_OK);
			}; // end if
		} else {
			ret=queue_m_msg_2(sessid, C2GMSK_MSG_AUDIOAVGLVLTEST, C2GMSK_AUDIOAVGLVLTEST_CANCELED, 0);

			if (ret != C2GMSK_RET_OK) {
				return(ret);
			}; // end if
			return(C2GMSK_RET_OK);
		}; // end else - if

	}; // end if (not disable audio level)


	// go to new state: 20 (check codec2 version id)
	sessid->d_state = 20;

	ret=queue_d_msg_2(sessid, C2GMSK_MSG_STATECHANGE,0,20 ); // move from state 0 to 20
	if (ret != C2GMSK_RET_OK) {
		return(ret);
	}; // end if

	// init some vars
	sessid->d_bitcount=0;
	sessid->d_octetcount=0;

	memset(sessid->d_codec2versionid,0,3); // clear codec2 version id var


	// reset print-bit to beginning of stream
	ret=queue_debug_bit_init(sessid);

	if (ret != C2GMSK_RET_OK) {
		return(ret);
	}; // end if

	// no further checks, so go to next sample
	return(C2GMSK_RET_OK);
}; // end state = 0;


///////////////////////////////////
// STATE 20
///////////////////////////////////
if (sessid->d_state == 20) {
	// state 20: receive version id
	unsigned char thisversionid;
	unsigned int versionidreceived;
	int mindistance;

	ret=queue_debug_bit(sessid,bit);

	if (ret != C2GMSK_RET_OK) {
		return(ret);
	}; // end if

	// read up to 24 bits
	if (bit) {
		sessid->d_codec2versionid[sessid->d_octetcount] |= bit2octet[sessid->d_bitcount];
	}; // end if

	sessid->d_bitcount++;

	// just return if not (n*8)th bit
	if (sessid->d_bitcount < 8) {
		return(C2GMSK_RET_OK);
	}; // end if

	// increase octet
	sessid->d_bitcount=0;
	sessid->d_octetcount++;

	// return if not 24 bits (3*8th)
	if (sessid->d_octetcount < 3) {
		return(C2GMSK_RET_OK);
	}; // end if

	// all 24 bits received.
	// invert values if audio is inverted
	if (sessid->d_inaudioinvert) {
		sessid->d_codec2versionid[0] ^= 0xff; sessid->d_codec2versionid[1] ^= 0xff; sessid->d_codec2versionid[2] ^= 0xff;
	}; // end if

	// version id received, copy octet per octet to be architecture independant
	versionidreceived=sessid->d_codec2versionid[0]+ (sessid->d_codec2versionid[1]<<8)+(sessid->d_codec2versionid[2]<<16);

	// search for match of received versionid in table
	// we assume that a match with a distance of 2 or less is good enough and there is no reason to search further afterwards
	// look for 24 bits

	// min distance is send as pointer as it also returns the distance between the assumed-found id
	mindistance=3; // minimal distance requisted

	assert((sessid->d_bitrate == C2GMSK_MODEMBITRATE_2400)  || (sessid->d_bitrate == C2GMSK_MODEMBITRATE_4800));
	if (sessid->d_bitrate == C2GMSK_MODEMBITRATE_2400) {
		// 2400 bps modem
		thisversionid=findbestmatch(versionidreceived, c2gmsk_idcode_2400, 16, 0xffffff, &mindistance);
	} else {
		// 4800 bps modem
		thisversionid=findbestmatch(versionidreceived, c2gmsk_idcode_4800, 16, 0xffffff, &mindistance);
	}; // end else - if (bitrate)

	// mindistance now contains minimum distance found
		
	// send debug message: 4 elements: found versionid, receivedversionid, code of selected versionid, distance
	if (sessid->d_bitrate == C2GMSK_MODEMBITRATE_2400) {
		ret=queue_d_msg_4(sessid, C2GMSK_MSG_VERSIONID, thisversionid, versionidreceived, c2gmsk_idcode_2400[thisversionid],mindistance);
	} else {
		ret=queue_d_msg_4(sessid, C2GMSK_MSG_VERSIONID, thisversionid, versionidreceived, c2gmsk_idcode_4800[thisversionid],mindistance);
	}; // end else - if

	if (ret != C2GMSK_RET_OK) {
		return(ret);
	}; // end if

	// send debug message: 1 element: bitrate
	ret=queue_d_msg_1(sessid, C2GMSK_MSG_BITRATE, sessid->d_bitrate);
	if (ret != C2GMSK_RET_OK) {
		return(ret);
	}; // end if


	thisversrate=(sessid->d_bitrate<<NUMVERSID_SHIFT)+thisversionid;


	// check version
	// currently only versionid "0x20" (speed 4800, version "0") or "0x1f" (speed 2400, version "15") are accepted
	if ((thisversrate != 0x20) && (thisversrate != 0x1f)){
		// unknown version id
		ret=queue_d_msg_0(sessid, C2GMSK_MSG_UNKNOWNVERSIONID);

		if (ret != C2GMSK_RET_OK) {
			return(ret);
		}; // end if

		// re-init vars
		sessid->d_syncreceived=0;
		sessid->d_last2octets=0;

		sessid->d_state=0;

		ret=queue_d_msg_2(sessid,C2GMSK_MSG_STATECHANGE,20, 0 ); // move from state 20 to 0
		if (ret != C2GMSK_RET_OK) {
			return(ret);
		}; // end if

		// next bit
		return(C2GMSK_RET_OK);
	}; // end if (not a valid version id)

	// store versionid / bitrate information
	sessid->d_version=thisversionid;
	// demodulate bitrate: we be replaced by auto bitrate detection in later versions
//	sessid->d_bitrate=sessid->m_bitrate;

	// combined var
	sessid->d_versrate=(sessid->d_bitrate<<NUMVERSID_SHIFT)+thisversionid;


	// OK, we have a valid state. Go to new state: 22 (codec2 data frame)
	sessid->d_state=22;

	ret=queue_d_msg_2(sessid,C2GMSK_MSG_STATECHANGE,20, 22 ); // move from state 20 to 22
	if (ret != C2GMSK_RET_OK) {
		return(ret);
	}; // end if


	// init some vars
	sessid->d_last4octets=0;

	sessid->d_octetcount=0;
	sessid->d_bitcount=0;

	memset(sessid->d_codec2inframe,0,24); // clear codec2inframe

	// (re)init auxiliary data (receiver chain)
	ret=auxdata_sessinit_r(sessid);
	if (ret != C2GMSK_RET_OK) {
		return(ret);
	}; // end if


// audiolevel check desabled: d_framesnoise not used
//	sessid->d_framesnoise=0;
	sessid->d_framecount=0;

	sessid->d_missedsync=0;

	sessid->d_syncfound=0;

	// reset print-bit to beginning of stream
	ret=queue_debug_bit_flush(sessid);

	if (ret != C2GMSK_RET_OK) {
		return(ret);
	}; // end if


	// go to next bit
	return(C2GMSK_RET_OK);

}; // end state 20


///////////////////////////////////
// STATE 21
///////////////////////////////////
// state 21: receiving header
// not yet implemented

///////////////////////////////////
// STATE 22
///////////////////////////////////
if (sessid->d_state == 22) {
	// some tempory vars
	unsigned char codec2inframe2[12]; // temprory buffer to store codec2 input frame, used for scrambling using modem 2400/15
	int scramblestart=0; // used for modem 2400/15

	// state 22: main part of codec2 stream

	// reading the main part of the stream
	// the first 32 bits are read into the "last4octets" var
	// this is easier to process bitslips (after reading the 24-bit header) as these are mainly
	// "bit" operations
	// after reading/processing these 32 bits, they are copied into "codec2inframe" memory
	// the last 160 bits are read directly into the "codec2inframe" structure

	// keep track of last 32 bits (used to determine bitslips)
	sessid->d_last4octets>>=1;
	if (bit) {
		sessid->d_last4octets |= 0x80000000;
	} else {
		sessid->d_last4octets &= 0x7fffffff;
	}; // end if


	// print out bit
	ret=queue_debug_bit(sessid,bit);
	if (ret != C2GMSK_RET_OK) {
		return(ret);
	}; // end if


	// state 22, part 1: receiving bits 24 up to 31
	if (sessid->d_octetcount < 4) {
		sessid->d_bitcount++;

		// part 1a, bits 22 up to 26: bitslip detection
		// (only done once, so when "syncfound" not yet set))
		if ((sessid->d_bitcount >= 22) && (sessid->d_bitcount <= 26) && (!sessid->d_syncfound)) {
			// bitslip tests

			uint32_t framesync, endsync; 

			if (sessid->d_inaudioinvert == 0) {
					framesync=0x5c08e700;
					endsync=0xc5807e00;
			} else {
					// only left 24 bits are used
					framesync=0xa3f71800;
					endsync=0x3a7f8100;
			}; // end else - if

			if (sessid->d_bitcount == 24) {
				// check sync pattern (first 24 bits), allow up to 3 biterrors
				if (countdiff32_frommsb(sessid->d_last4octets, 0xffffff00, 24, framesync, 3)) {
					sessid->d_marker='S';
					sessid->d_missedsync=0;
					sessid->d_endfound=0;
					sessid->d_syncfound=1; // no further checks needed;
				} else if (countdiff32_frommsb(sessid->d_last4octets, 0xffffff00, 24, endsync, 3)) {
					sessid->d_marker='E';
					sessid->d_endfound=1;
					sessid->d_syncfound=1; // no further checks needed;
				}; // end elsif - if

			} else if ((sessid->d_bitcount == 23) || (sessid->d_bitcount == 25)) {
				// bitslip +1 or -1 bit
				// check sync pattern (first 24 bits), allow up to 1 biterror
				// if found, correct "bitcount" value

				if (countdiff32_frommsb(sessid->d_last4octets, 0xffffff00, 24, framesync, 1)) {
					sessid->d_marker='T';
					sessid->d_missedsync=0;
					sessid->d_endfound=0;
					sessid->d_syncfound=1; // no further checks needed
					sessid->d_bitcount=24; // correct bit position
				} else if (countdiff32_frommsb(sessid->d_last4octets, 0xffffff00, 24, endsync, 1)) {
					sessid->d_marker='F';
					sessid->d_endfound=1;
					sessid->d_syncfound=1; // no further checks needed
					sessid->d_bitcount=24; // correct bit position
				}; // end elsif - if
			} else if ((sessid->d_bitcount == 22) || (sessid->d_bitcount == 26)) {
				// bitslip +2 or -2 bit
				// check sync pattern (first 24 bits), allow no biterror
				// if found, correct "bitcount" value

				if (countdiff32_frommsb(sessid->d_last4octets, 0xffffff00, 24, framesync, 0)) {
					sessid->d_marker='U';
					sessid->d_missedsync=0;
					sessid->d_endfound=0;
					sessid->d_syncfound=1; // no further checks needed
					sessid->d_bitcount=24; // correct bit position
				} else if (countdiff32_frommsb(sessid->d_last4octets, 0xffffff00, 24, endsync, 0)) {
					sessid->d_marker='G';
					sessid->d_endfound=1;
					sessid->d_syncfound=1; // no further checks needed
					sessid->d_bitcount=24; // correct bit position
				}; // end elseif - if
			}; // end elsif - elsif - if

			// go to next sample
			return(C2GMSK_RET_OK);
		}; // end if (position 22 up to 26)


		if (sessid->d_bitcount < 32) {
		// if not yet 32 bits received, just continue
			return(C2GMSK_RET_OK);
		}; // end if

		// part 1b, 32 bits received. Copy them over to the codec2inframe structure
		// note that the octet order is reversed

		// copy octet per octet as we are not sure how a 32bit integer structure is stored in memory
		// (let's try to be platform independent) :-) 
		sessid->d_codec2inframe[0]=(unsigned char)sessid->d_last4octets & 0xff;
		sessid->d_codec2inframe[1]=(unsigned char)(sessid->d_last4octets >> 8) & 0xff;
		sessid->d_codec2inframe[2]=(unsigned char)(sessid->d_last4octets >> 16) & 0xff;
		sessid->d_codec2inframe[3]=(unsigned char)(sessid->d_last4octets >> 24) & 0xff;

		sessid->d_octetcount = 4;
		sessid->d_bitcount=0;

		// done, get next sample
		return(C2GMSK_RET_OK);
	}; // end if
			

	// state 22, part 2: receiving bits 32 up to the end
	// 24 octets (192 bits) for 4800 bps
	// 12 octets (96 bits) for 2400 bps

	// some sanity checking
	assert(sessid->d_octetcount <= sessid->framesize40ms);


	if (bit) {
		sessid->d_codec2inframe[sessid->d_octetcount] |= bit2octet[sessid->d_bitcount];
	}; // end if

	sessid->d_bitcount++;
	if (sessid->d_bitcount >= 8) {
		sessid->d_octetcount++;
		sessid->d_bitcount=0;
	}; // end if

	// go to next bit if not yet all bits received
	if (sessid->d_octetcount < sessid->framesize40ms) {
		return(C2GMSK_RET_OK);
	}; // end if

	// we have received all bits of the frame. Let's process
	// if no sync found
	if (!sessid->d_syncfound) {
		sessid->d_marker='M';
		sessid->d_missedsync++;
	}; // end if


	// flush debug bits
	ret=queue_debug_bit_flush(sessid);

	if (ret != C2GMSK_RET_OK) {
		return(ret);
	}; // end if

	// for 2400/15 modem
	if (sessid->d_versrate == 0x1f) {
		// if sync pattern is found or end marker is found
		// reset "framecount" value -> determines what sync pattern to use
		if ((sessid->d_syncfound) || (sessid->d_endfound)) {
			sessid->d_framecount=0;
		}; // end if
	}; // end if

	// some error checking
	// we have up to 8 lines of known exor-patterns
	sessid->d_framecount &= 0x7;


	// step 1: transmission interleaving: only for 2400/15 modem
	if  (sessid->d_versrate == 0x1f) {
		// STEP 1: DEINTERLEAVE BITS FOR TRANSMISSION

		if (sessid->d_framecount) {
			// framecount != 0 -> no sync
			interleaveTX_rx(sessid->d_codec2inframe,codec2inframe2,0);
		} else {
			// framecount == 0 -> sync
			interleaveTX_rx(sessid->d_codec2inframe,codec2inframe2,1);
		}; // end else - if

	}; // end if

	// step 2: scrambling

	if  (sessid->d_versrate == 0x1f) {
	// 2400/15 modem
		unsigned char *c1, *c2;
		
		// no sync: scrambling starts from the 1st char
		if (sessid->d_framecount) {
			scramblestart=0;
		} else {
		// if framecount is 0 (where sync is supposed to be present), start scrambling at 4th char, not first
			scramblestart=3;
		}; // end else - if

		// do actual scrambling
		c1=&scramble_exor[sessid->d_framecount][scramblestart];
		// descramblink chars (O or 3) up to 12 (starting at 0)
		c2=&codec2inframe2[scramblestart];

		// descramble 9 or 12 chars
		for (loop=scramblestart; loop < 12; loop++) {
			*c2 ^= *c1;
			c2++; c1++;
		}; // end for


	} else  {
	// 4800/0 modem
		unsigned char *c1, *c2;

		c1=&scramble_exor[sessid->d_framecount][0];
		// descramblink chars 3 up to 24 (starting at 0)
		c2=&sessid->d_codec2inframe[3];

		// descramble 21 chars
		for (loop=0; loop < 21; loop++) {
			*c2 ^= *c1;
			c2++; c1++;
		}; // end for

	}; 

	// step 3
	// invert bits if audio is inverted.
	// done now so FEC and FEC deinterleaving is not misled
	// only done for 2400/15 modem
	if (sessid->d_inaudioinvert) {
		unsigned char *p;
		int n;

		if (sessid->d_versrate == 0x1f) {
		// 2400/15 modem
			p=codec2inframe2;
			n=12;
		} else {
			p=sessid->d_codec2frame;
			n=24;
		}; // end else - if

		for (loop=0; loop<n; loop++) {
			*p ^= 0xff; p++;
		}; // end for
	}; // end if (audioinvert)
	

	// Step 4: apply FEC + inner interleaving

	// do not apply to "endmarker" data (all empty anyway)
	if (sessid->d_versrate == 0x20) {
		// 4800 bps modem
		unsigned char *p1, *d;

		int totalerror=0;
		int error_8bit=0;

		p1=&sessid->d_codec2inframe[3];
		// destination is codec2frame. Point to beginning of struct, then go up one position per loop
		d=sessid->d_codec2frame;

		for (loop=0; loop < 7; loop++) {
			error_8bit = fec13decode_8bit(*p1,sessid->d_codec2inframe[interl2[loop]],sessid->d_codec2inframe[interl3[loop]],d);
			if (error_8bit) {
				totalerror += count1s_8bit(error_8bit);
			}; // end if
			p1++; d++;
		}; // end for

		ret=queue_d_msg_1(sessid,C2GMSK_MSG_FECSTATS, totalerror ); // total error

		if (ret != C2GMSK_RET_OK) {
			return(ret);
		}; // end if
		
	} else {
		// 2400 bps modem

		// deinterleave + copy
		// use "interleave" function, but set "forward" to 0
		if (sessid->d_framecount) {
			int numerrors;

			// apply FEC:
			// frame without SYNC
			// bits 12 to 23 are golay code for bits  0 to 11
			// bits 36 to 47 are golay code for bits 24 to 35
			// bits 60 to 71 are golay code for bits 48 to 59


			numerrors=golay23_decode(codec2inframe2); // bits 0 to 23
			ret=queue_d_msg_1(sessid,C2GMSK_MSG_FECSTATS, numerrors ); // total error
			if (ret != C2GMSK_RET_OK) {
				return(ret);
			}; // end if

			numerrors=golay23_decode(&codec2inframe2[3]); // bits 24 to 47
			ret=queue_d_msg_1(sessid,C2GMSK_MSG_FECSTATS, numerrors ); // total error
			if (ret != C2GMSK_RET_OK) {
				return(ret);
			}; // end if

			numerrors=golay23_decode(&codec2inframe2[6]); // bits 48 to 71
			ret=queue_d_msg_1(sessid,C2GMSK_MSG_FECSTATS, numerrors ); // total error
			if (ret != C2GMSK_RET_OK) {
				return(ret);
			}; // end if


			// auxdata
			// not a sync frame, auxdata is at positions 92 to 95
			auxdata_receive1(sessid,(codec2inframe2[11]>>4));


			// framecount != 0 -> call deinterleaver with "sync" set to 0
			c2_interleave_2400_1400((uint8_t*)codec2inframe2,sessid->d_codec2frame,0,0);
		} else {
			int numerrors;

			// framecount == 0 -> sync
			// apply FEC
			// frame with SYNC
			// bits 36 to 47 are golay code for bits  0 to 11 (first data element after sync pattern)
			numerrors=golay23_decode(&codec2inframe2[3]); // bits 0 to 23
			ret=queue_d_msg_1(sessid,C2GMSK_MSG_FECSTATS, numerrors ); // total error
			if (ret != C2GMSK_RET_OK) {
				return(ret);
			}; // end if


			// auxdata
			// sync frame, auxdata is at positions 68 to 71
			auxdata_receive1(sessid,(codec2inframe2[8]>>4));

			// framecount != 0 -> call deinterleaver with "sync" set to 0
			// data starts at 3th char
			c2_interleave_2400_1400((uint8_t*)&codec2inframe2[3],sessid->d_codec2frame,1,0);
		}; // end if

	}; // end else - if (modem?)

		
	// increase framecounter; no need to do boundary check, happens above
	sessid->d_framecount++;


	// final part
	// 3 possible senario:
	// - to many missed sync: just stop
	// - endfound: send final frame and stop
	// other: send frame
	{
	int maxmissed;

		if (sessid->d_versrate == 0x20) {
			// modem 4800/0: break out after 20 missed
			maxmissed=20;
		} else {
			// modem 2400/15: break out after 50 missed (as sync is only send every 8 frames)
			maxmissed=50;
		}; // end else - if


		if (sessid->d_missedsync < maxmissed) {
			// send frame
			c2gmsk_msg_codec2 msg;

			// additional test: if end is found, and frame is all-zero, this is a empty end-of-stream frame. No need to send if
			if ((sessid->d_endfound==0) || (memcmp(allzero,sessid->d_codec2frame,7))) {
				memcpy(&msg.signature,MSG_SIGNATURE,4);
				msg.tod=C2GMSK_MSG_CODEC2;
				msg.datasize=sizeof(msg) - sizeof(struct c2gmsk_msg); // should be "7", however, due to work-alligment rules, it can be larger
																						// so use "size" functions to be platform independant
				msg.realsize=C2GMSK_CODEC2_FRAMESIZE_1400;
				msg.version=C2GMSK_CODEC2_1400;
				memcpy(msg.c2data,sessid->d_codec2frame,C2GMSK_CODEC2_FRAMESIZE_1400); // copy 7 octets

				ret=c2gmskchain_add (sessid->d_chain,&msg,sizeof(msg));

				if (ret != C2GMSK_RET_OK) {
					return(ret);
				}; // end if
			}; // end if: not (end and all-empty)
		}; // end if


		// send "end of stream" message if end found
		if (sessid->d_endfound)  {
			ret=queue_d_msg_0(sessid,C2GMSK_MSG_END_NORMAL); // normal end

			if (ret != C2GMSK_RET_OK) {
				return(ret);
			}; // end if
		}; // end if

		// break out of stream if to many missed sync
		if (sessid->d_missedsync >= maxmissed) {
			ret=queue_d_msg_0(sessid,C2GMSK_MSG_END_TOMANYMISSEDSYNC); // normal end

			if (ret != C2GMSK_RET_OK) {
				return(ret);
			}; // end if

			sessid->d_endfound=1;
		}; // end if


		// things to do at end of packet
		// re-init vars
		sessid->d_octetcount=0;
		sessid->d_bitcount=0;
		sessid->d_syncfound=0;

		// clear memory for new frame
		memset(sessid->d_codec2inframe,0,24);

		// reset counters of printbit function
		ret=queue_debug_bit_flush(sessid);
		if (ret != C2GMSK_RET_OK) {
			return(ret);
		}; // end if


		// if not end, go to next frame
		if (!sessid->d_endfound) {
			return(C2GMSK_RET_OK);
		}; // end if

		// END FOUND!!!
		// things to do at the end of the stream

		// flush auxiliary data
		ret=auxdata_receive_flush(sessid);
		if (ret != C2GMSK_RET_OK) {
			return(ret);
		}; // end if


		// reinit vars
		sessid->d_last2octets=0;
		sessid->d_syncreceived=0;
		sessid->d_inaudioinvert=0;
		sessid->d_endfound=0;
		sessid->d_state=0; // new state is 0, "waiting for sync"

		ret=queue_d_msg_2(sessid,C2GMSK_MSG_STATECHANGE,22 , 0 ); // move from state 22 to 0
		if (ret != C2GMSK_RET_OK) {
			return(ret);
		}; // end if

		// get next audio-frame to look for new stream
		return(C2GMSK_RET_OK);
	}; // end  final part

}; // end if (state 22)

// ALL DONE
return(C2GMSK_RET_OK);
}; // end function C2GMSK DEMODULATE PROCESS1BIT



///////////////
// function c2gmsk demodulation init
int c2gmsk_demod_init (struct c2gmsk_session * sessid, struct c2gmsk_param * param) {
int ret;
int loop;
	
// sanity checks
ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// check "disabled" flag
if (sessid->d_disabled == C2GMSK_DISABLED) {
	return(C2GMSK_RET_OPERATIONDISABLED);
}; // end if

ret=checksign_param(param);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// node "demod_init" is called from sess_new, but as nothing says that demodulation
// will be needed, the "disablelevelcheck" is only checked if demodulation is enabled
// (i.e. all needed parameters are filled in, i.e. values 0 or higher)

// copy parameters to session
sessid->d_disableaudiolevelcheck=param->d_disableaudiolevelcheck;
sessid->d_bitrate=param->d_bitrate;



// reinit vars
sessid->d_state=0;
sessid->d_audioinvert=0;

sessid->d_printbitcount=0;
sessid->d_printbitcount_v=0;

// audio level check
sessid->d_audiolevelindex=0;
for (loop=0; loop < 32; loop++) {
	sessid->d_audioleveltable[loop]=0;
}; // end for

// maxaudiolevel check disabled (see notes in gmskmodemapi.h/
// newsessid>d_maxaudiolevelvalid=0;
//	int d_maxaudiolevelvalid_total;
//	int d_maxaudiolevelvalid_numbersample;
//	int d_countnoiseframe;
//	int d_framesnoise;

// data for demod
sessid->d_last2octets=0;
sessid->d_last4octets=0;
sessid->d_syncreceived=0;
sessid->d_inaudioinvert=0;
sessid->d_bitcount=0;
sessid->d_octetcount=0;
sessid->d_framecount=0;
sessid->d_missedsync=0;
sessid->d_syncfound=0;
sessid->d_endfound=0;
sessid->d_marker=' ';

memset(sessid->d_codec2frame,0,7);
memset(sessid->d_codec2versionid,0,3);
memset(sessid->d_codec2inframe,0,24);

memset(sessid->d_printbit,' ',96);
sessid->d_printbitcount=0;

memset(sessid->d_printbit_v,' ',96);
sessid->d_printbitcount_v=0;


// vars for DSP filters
// for "demodulate"
sessid->dd_dem_init=1;

// for "firfilter_demodulate"
sessid->dd_filt_init=1;

// for predecimator filter
sessid->dd_predecfilt_init=1;


ret=queue_debug_bit_init(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// vars for auxiliary data
sessid->auxdata_r_data=NULL;
sessid->auxdata_r_buffersize=0;
sessid->auxdata_r_octetcount=0;
sessid->auxdata_r_nibble=0;

sessid->auxdata_s_data=NULL;
sessid->auxdata_s_size=0;
sessid->auxdata_s_octetcount=0;
sessid->auxdata_s_nibble=0;


return(C2GMSK_RET_OK);
}; // end function c2gmsk demodulation init


//////////////////
// function c2gmsk_demodulate PCM
// demodulate audio (1920 PCM samples)
int c2gmsk_demodpcm (struct c2gmsk_session *sessid, int16_t *in, struct c2gmsk_msgchain ** out) {
int ret, demod_retval;
int loop, sampleloop;
int16_t *samplepointer;
int16_t audiosample;
int bit;


// sanity checks
ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// check "disabled" flag
if (sessid->d_disabled == C2GMSK_DISABLED) {
	return(C2GMSK_RET_OPERATIONDISABLED);
}; // end if


// is the output format PCM? 
if (sessid->outputformat != C2GMSK_OUTPUTFORMAT_AUDIO) {
	return(C2GMSK_RET_UNSUPPORTEDOUTPUTFORMAT);
}; // end if


// does audio point somewhere?
if (!in) {
	return(C2GMSK_RET_NOVALIDAUDIO);
}; // end if


// check "out", must point somewhere
if (!out) {
	return(C2GMSK_RET_NOVALIDRETPOINT);
}; // end if


// reinit chain 
ret=c2gmskchain_reinit(sessid->d_chain, C2GMSKCHAIN_DEFAULTSIZE_MOD);
if (ret != C2GMSK_RET_OK) {
	// something went wrong, exit
	return(ret);
}; // end if

//////////////////
// part 1 of demodulation code
// this code runs once for every 40 ms audio-segment

// init some vars
ret=queue_debug_allbit_init(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if



// demodulation return value.
// init to OK
demod_retval=C2GMSK_RET_OK;


// calculate average value of sampleblock
sessid->d_audioaverage=0;

samplepointer=in;
// samples 0 up to 1918
for (loop=0; loop < 1919; loop++) {
	sessid->d_audioaverage += abs(*samplepointer);
	samplepointer++; // move up pointer
}; // end for

// samples 1919 (no need to move up pointers afterward)
sessid->d_audioaverage += abs(*samplepointer); // last audio sample

sessid->d_audioaverage >>= 11; // = divide by 2048, almost same as 1920 (actual number of
									 			 // samples) but much faster then divide, especially on CPUs
												 // without FPU

ret=queue_d_msg_1(sessid,C2GMSK_MSG_AUDIOAVGLEVEL, sessid->d_audioaverage);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if



// store audio average audio-level in ringbuffer, to be used later on to
// detect false-positive start-or-stream

sessid->d_audiolevelindex++;
sessid->d_audiolevelindex &= 0x1f; // wrap from 31 back to 0
sessid->d_audioleveltable[sessid->d_audiolevelindex]=sessid->d_audioaverage;


// do we process audio?
// if state is 0 (waiting for sync), and input audio-level is to high
// (i.e. received signal is noise), we skip the complete frame

// noisecheck is disabled as calculating the average audio-level is
// done in the header
// as the current implementation of the gmskmodem does not use a
// header, this check does not make sence
//if ((sessid->d_state == 0) && (sessid->d_maxaudiolevelvalid) && (sessid->d_audioaverage > sessid->d_maxaudiolevelvalid)) {
//	// audio is rejected (level to high, probably noise)
//	sessid->d_countnoiseframe++;


//	if (sessid->d_countnoiseframe > MAXNOISEREJECT) {
//		// disable noisecheck if to many frames rejected
//		sessid->d_maxaudiolevelvalid=0;
//	}; // end if

//	// done, return
//	demod_retval=C2GMSK_RET_OK;
//	goto l_demod_end;
//}; // end if


// OK, at this point, we have a valid audio frame
// reset count noise frame
// sessid->d_countnoiseframe=0;


// code only used in state 21 (receiving header), but as the current version
// of this modem does not have a header, ignore this for now

//	// when receiving header (state is 21), add up audio-level data to calculate
//	// maxaudiolevel when header completely received
//	if (state == 21) {
//		maxaudiolevelvalid_total += p_r_global->audioaverage[thisbuffer];
//		maxaudiolevelvalid_numbersample++;
//	}; // end if

// // when receiving data frame (state is 20), just count number of consequent audiolevels
// if (sessid->d_state == 20) {
// 	if ((sessid->d_maxaudiolevelvalid) && (sessid->d_audiolevel > sessid->d_maxaudiolevelvalid)) {
// 		sessid->d_framesnoise++;
// 	} else {
// 		sessid->d_framesnoise=0;
// 	}; // end else - if
// }; // end if

/////////////////////////
// part 2
// process individual samples in buffer

samplepointer=in;


assert((sessid->d_bitrate==C2GMSK_MODEMBITRATE_2400) || (sessid->d_bitrate==C2GMSK_MODEMBITRATE_4800));

for (sampleloop=0; sampleloop < 1920; sampleloop ++) {
	// get audio
	audiosample = *samplepointer;

	// move up pointer
	samplepointer++;


	// demodulate and get bit
	if (sessid->d_bitrate==C2GMSK_MODEMBITRATE_2400) {
		bit=demodulate_2400(sessid,audiosample);
	} else {
		bit=demodulate_4800(sessid,audiosample);
	}; // end else - if

	// the demodulate function returns three possible values:
	// -1: not a valid bit
	// 0 or 1, valid bit
	if (bit < 0) {
		// not a valid bit, go to next audio sample
		continue;
	}; // end if


	ret=c2gmsk_demod_process1bit(sessid,bit);	
	if (ret != C2GMSK_RET_OK) {
		return(ret);
	}; // end if

}; // end for (sampleloop)

// All samples done.

ret=queue_debug_allbit_flush(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if


// end of function

// label currently unused.
// commented-out to avoid warnings
// l_demod_end:

// create empty return msg if no data
// queue nodata if nothing in queue
ret=queue_d_nodataifempty (sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if


// done. Return
*out=sessid->d_chain;
return(demod_retval);
}; // end function c2gmsk_demodulate PCM


//////////////////
// function c2gmsk_demodulate
// function c2gmsk_demodulate is a copy of the c2gmsk_demodulatepcm function
// retained for compatibily. Will be removed in a later version of the API
int c2gmsk_demod (struct c2gmsk_session *sessid, int16_t *in, struct c2gmsk_msgchain ** out) {
	return(c2gmsk_demodpcm (sessid, in, out));
}; // end function c2gmsk_demod

//////////////////
// function c2gmsk_demodulate GMSK
// demodulate GMSK (96 or 192 bits)
int c2gmsk_demodgmsk (struct c2gmsk_session *sessid, unsigned char *in, struct c2gmsk_msgchain ** out) {
int ret;
int bitloop, numbits;
unsigned char *octetpointer;
int bitcounter;
int bit;


// sanity checks
ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// check "disabled" flag
if (sessid->d_disabled == C2GMSK_DISABLED) {
	return(C2GMSK_RET_OPERATIONDISABLED);
}; // end if


// is the output format PCM? 
if (sessid->outputformat != C2GMSK_OUTPUTFORMAT_GMSK) {
	return(C2GMSK_RET_UNSUPPORTEDOUTPUTFORMAT);
}; // end if


// does audio point somewhere?
if (!in) {
	return(C2GMSK_RET_NOVALIDAUDIO);
}; // end if


// check "out", must point somewhere
if (!out) {
	return(C2GMSK_RET_NOVALIDRETPOINT);
}; // end if


// reinit chain 
ret=c2gmskchain_reinit(sessid->d_chain, C2GMSKCHAIN_DEFAULTSIZE_MOD);
if (ret != C2GMSK_RET_OK) {
	// something went wrong, exit
	return(ret);
}; // end if

//////////////////
// part 1 of demodulation code
// this code runs once for every 40 ms gmsk-segment
// As input is a GMSK stream, no "audio"-based checks (noise, audio-level)
// are possible
// no other checks to be done


// init some vars
ret=queue_debug_allbit_init(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

/////////////////////////
// part 2
// process individual samples in buffer
if (sessid->d_bitrate == C2GMSK_MODEMBITRATE_2400) {
	numbits=96;
} else {
	numbits=192;
}; // end if

octetpointer=in;
bitcounter=0;

for (bitloop=0; bitloop < numbits; bitloop ++) {
	// get bit
	bit=*octetpointer & bit2octet[bitcounter];

	// move up
	bitcounter++;
	if (bitcounter >= 8) {
		octetpointer++;
		bitcounter=0;
	}; // end if

	ret=c2gmsk_demod_process1bit(sessid,bit);	
	if (ret != C2GMSK_RET_OK) {
		return(ret);
	}; // end if

}; // end for (sampleloop)

// All samples done.

ret=queue_debug_allbit_flush(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if


// end of function

// label currently unused.
// commented-out to avoid warnings
// l_demod_end:

// create empty return msg if no data
// queue nodata if nothing in queue
ret=queue_d_nodataifempty (sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if


// done. Return
*out=sessid->d_chain;
return(C2GMSK_RET_OK);
}; // end function c2gmsk_demodulate GMSK


