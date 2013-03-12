//////////////////////
// API version of the GMSK modem for 10m / VHF / UHF communication
// using codec2
// version 0 (versionid 0x111111): 4800 bps, 1/3 repetition code FEC


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


// c2gmsk API sessions

// functions:
// checksign_sess
// c2gmsk_sess_new
// c2gmsk_sess_destroy


// support function: check session signature
int checksign_sess (session * sessid) {

// used for sanity checking
// does the sessid point somewhere?

if (!sessid) {
	return(C2GMSK_RET_NOVALIDSESS);
}; 

if (memcmp(sessid->signature,SESS_SIGNATURE,4)) {
	return(C2GMSK_RET_NOVALIDSESS);
}; // end if


// OK
return(C2GMSK_RET_OK);
}; // end function check sessign signature



/////////////////////////////////////
// support functions: new and destroy
session * c2gmsk_sess_new (c2gmsk_params *params, int * ret) {
session *newsessid;
int ret2;
int loop;

// check signature
if (memcmp(params->signature,PARAMS_SIGNATURE,4)) {
	*ret=C2GMSK_RET_NOVALIDPARAMS;
	return(NULL);
}; // end if

// allocate memory for new structure
newsessid=malloc(sizeof(session));

if (!newsessid) {
	// could not allocate memory
	*ret=C2GMSK_RET_NOMEMORY;
	return(NULL);
}; // end if

// set signature in data
memcpy(newsessid->signature,SESS_SIGNATURE,4);

// create return data chain for modulation
newsessid->m_chain=c2gmskchain_new(C2GMSKCHAIN_DEFAULTSIZE_MOD,&ret2);

if (ret2 != C2GMSK_RET_OK) {
	// oeps. Could not allocate memory, free everything already created and return
	free(newsessid);
	*ret=ret2;
	return(NULL);
}; // end if

// create return data chain for demodulation
newsessid->d_chain=c2gmskchain_new(C2GMSKCHAIN_DEFAULTSIZE_DEMOD,&ret2);
if (ret2 != C2GMSK_RET_OK) {
	// oeps. Could not allocate memory, free everything already created and return
	c2gmskchain_destroy(newsessid->m_chain);

	free(newsessid);
	*ret=ret2;
	return(NULL);
}; // end if


// init audiobuffer
// copy signature for 48kbps audio
memcpy(newsessid->m_abuff.signature,ABUFF48_SIGNATURE,4);

// other data
newsessid->d_state=0;
newsessid->d_audioinvert=0;

newsessid->d_printbitcount=0;
newsessid->d_printbitcount_v=0;


// audio level check:
newsessid->d_audiolevelindex=0;
for (loop=0; loop < 32; loop++) {
	newsessid->d_audioleveltable[loop]=0;
}; // end for
// disable audio level check, copy from paramlist
newsessid->d_disableaudiolevelcheck=params->d_disableaudiolevelcheck;

// maxaudiolevel check disabled (see notes in gmskmodemapi.h/
// newsessid>d_maxaudiolevelvalid=0;
//	int d_maxaudiolevelvalid_total;
//	int d_maxaudiolevelvalid_numbersample;
//	int d_countnoiseframe;
//	int d_framesnoise;

// data for demod
newsessid->d_last2octets=0;
newsessid->d_last4octets=0;
newsessid->d_syncreceived=0;
newsessid->d_inaudioinvert=0;
newsessid->d_bitcount=0;
newsessid->d_octetcount=0;
newsessid->d_framecount=0;
newsessid->d_missedsync=0;
newsessid->d_syncfound=0;
newsessid->d_endfound=0;
newsessid->d_marker=' ';

memset(newsessid->d_codec2frame,0,7);
memset(newsessid->d_codec2versionid,0,3);
memset(newsessid->d_codec2inframe,0,24);

memset(newsessid->d_printbit,' ',96);
newsessid->d_printbitcount=0;

memset(newsessid->d_printbit_v,' ',96);
newsessid->d_printbitcount_v=0;


// vars for DSP filters
// for "firfilter_modulate"
newsessid->md_filt_init=1;

// for "demodulate"
newsessid->dd_dem_init=1;

// for "firfilter_demodulate"
newsessid->dd_filt_init=1;


// init debug messages
c2gmsk_printstr_init();


// init demodulator
ret2=c2gmsk_demod_init(newsessid);
if (ret2 != C2GMSK_RET_OK) {
	*ret=ret2;
	return(NULL);
}; // end if


// done
*ret=C2GMSK_RET_OK;
return(newsessid);
}; // end function NEW

///////////////////////////

//////////////////////////
// FUNCTION C2GMSK_DESTROY
// free allocated memory

int c2gmsk_sess_destroy (session * sessid) {

int ret;

// sanity checks
ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// destroy return-data chains
c2gmskchain_destroy(sessid->m_chain);
c2gmskchain_destroy(sessid->d_chain);

// OK, free session and exit
free(sessid);

return(C2GMSK_RET_OK);

}; // end function GMSK DESTROY





