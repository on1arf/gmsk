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


// c2gmsk API sessions

// functions:
// checksign_sess
// c2gmsk_sess_new
// c2gmsk_sess_destroy


// support function: check session signature
int checksign_sess (struct c2gmsk_session * sessid) {

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
struct c2gmsk_session * c2gmsk_sess_new (struct c2gmsk_param * param , int * ret, struct c2gmsk_msgchain ** out) {
struct c2gmsk_session *newsessid;
int ret2;

// check signature
ret2=checksign_param(param);
if (ret2 != C2GMSK_RET_OK) {
	*ret=ret2;
	return(NULL);
}; // end if

// sanity checking: "out" pointer, does it point somewhere?
if (!out) {
	*ret=C2GMSK_RET_NOVALIDRETPOINT;
	return(NULL);
}; // end if

// check exptected API version
// if higher then current, we have a problem
if (param->expected_apiversion > C2GMSK_APIVERSION) {
	*ret=C2GMSK_RET_UNSUPPAPIVERS;
	return(NULL);
}; // end if

// allocate memory for new structure
newsessid=malloc(sizeof(struct c2gmsk_session));

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



// init modulator
ret2=c2gmsk_mod_init(newsessid, param);
if (ret2 != C2GMSK_RET_OK) {
	*ret=ret2;
	return(NULL);
}; // end if


// init demodulator
// note: init demod will also call "queue_debug_bit_init" and "c2gmsk_printstr_init"
ret2=c2gmsk_demod_init(newsessid, param);
if (ret2 != C2GMSK_RET_OK) {
	*ret=ret2;
	return(NULL);
}; // end if


// return capabilities
// we are using the msgchain of the modulator for this
ret2=queue_m_msg_2(newsessid,C2GMSK_MSG_CAPABILITIES, 0, C2GMSK_MODEMBITRATE_4800); // we support versionid 0 / bitrate 4800
if (ret2 != C2GMSK_RET_OK) {
	*ret=ret2;
	return(NULL);
}; // end if


// return message queue
*out=newsessid->m_chain;

// done
*ret=C2GMSK_RET_OK;
return(newsessid);
}; // end function NEW

///////////////////////////

//////////////////////////
// FUNCTION C2GMSK_DESTROY
// free allocated memory

int c2gmsk_sess_destroy (struct c2gmsk_session * sessid) {

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





