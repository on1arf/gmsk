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


// printstr: conversion of numeric messages and return values into text

// functions:
// c2gmsk_printstr_init

// c2gmsk_printstr_ret
// c2gmsk_printstr_msg
// c2gmsk_printstr_avglvltest
// c2gmsk_printstr_statdem



void c2gmsk_printstr_init () {
int loop;


// RETURN MESSAGES
for (loop=0; loop <= C2GMSK_RET_HIGHEST; loop++) {
	c2gmsk_str_ret[loop]=NULL;
}; // end for

c2gmsk_str_ret[C2GMSK_RET_OK]="OK";
c2gmsk_str_ret[C2GMSK_RET_NOTINIT]="Session not initialised";
c2gmsk_str_ret[C2GMSK_RET_NOMEMORY]="Out of memory";
c2gmsk_str_ret[C2GMSK_RET_NOTIMPLEMENTED]="Not yet implemented";
c2gmsk_str_ret[C2GMSK_RET_SESSEXISTS]="Session already exists";
c2gmsk_str_ret[C2GMSK_RET_CHAINEXISTS]="Messagechain already exists";
c2gmsk_str_ret[C2GMSK_RET_ABUFFEXISTS]="Audiobuffer already exists";
c2gmsk_str_ret[C2GMSK_RET_TOLARGE]="Messagebuffer has grown to large";
c2gmsk_str_ret[C2GMSK_RET_BADSTATE]="Modem is bad state";
c2gmsk_str_ret[C2GMSK_RET_NOVALIDSESS]="Not a valid session structure";
c2gmsk_str_ret[C2GMSK_RET_NOVALIDCHAIN]="Not a valid messagechain structure";
c2gmsk_str_ret[C2GMSK_RET_NOVALIDABUFF]="Not a valid audiobuffer structure";
c2gmsk_str_ret[C2GMSK_RET_NOVALIDDATA]="Not a valid data structure";
c2gmsk_str_ret[C2GMSK_RET_NOVALIDAUDIO]="Not a valid inputaudio structure";
c2gmsk_str_ret[C2GMSK_RET_NOVALIDPARAMS]="Not a valid parameter structure";
c2gmsk_str_ret[C2GMSK_RET_NOVALIDRETPOINT]="Not a valid messagechain return pointer";
c2gmsk_str_ret[C2GMSK_RET_UNSUPPORTEDVERSIONID]="Unsupported c2gmsk version id";
c2gmsk_str_ret[C2GMSK_RET_UNSUPPORTEDMODEMBITRATE]="Unsupported modem version id";
c2gmsk_str_ret[C2GMSK_RET_OPERATIONDISABLED]="Operation Disabled";


// type of data
for (loop=0; loop <= C2GMSK_MSG_HIGHEST; loop++) {
	c2gmsk_str_msg[loop]=NULL;
}; // end for

c2gmsk_str_msg[C2GMSK_MSG_UNDEF]="undefined";
c2gmsk_str_msg[C2GMSK_MSG_NODATA]="no data";
c2gmsk_str_msg[C2GMSK_MSG_CAPABILITIES]="capabilities";
c2gmsk_str_msg[C2GMSK_MSG_STATECHANGE]="change of state";
c2gmsk_str_msg[C2GMSK_MSG_CODEC2]="codec2 voice data";
c2gmsk_str_msg[C2GMSK_MSG_PCM8K]="pcm audio at 8khz";
c2gmsk_str_msg[C2GMSK_MSG_PCM48K]="pcm audio at 48 khz";
c2gmsk_str_msg[C2GMSK_MSG_AUDIOAVGLEVEL]="average audio level";
c2gmsk_str_msg[C2GMSK_MSG_AUDIOAVGLVLTEST]="average audio-level test result";
c2gmsk_str_msg[C2GMSK_MSG_INAUDIOINVERT]="input audio inversion";
c2gmsk_str_msg[C2GMSK_MSG_VERSIONID]="versionid";
c2gmsk_str_msg[C2GMSK_MSG_FECSTATS]="forward-error-correction statistics";
c2gmsk_str_msg[C2GMSK_PRINTBIT_MOD]="bitdump demodulated signals";
c2gmsk_str_msg[C2GMSK_PRINTBIT_ALL]="bitdump all signals";
c2gmsk_str_msg[C2GMSK_MSG_UNKNOWNVERSIONID]="unknown version id";
c2gmsk_str_msg[C2GMSK_MSG_END_NORMAL]="stream terminated due to end-of-stream received";
c2gmsk_str_msg[C2GMSK_MSG_END_TOMANYMISSEDSYNC]="stream terminated due to to many missed sync";




// STATUS DEMOD
for (loop=0; loop <= C2GMSK_STATDEM_HIGHEST; loop++) {
	c2gmsk_str_statdem[loop]=NULL;
}; // end for

c2gmsk_str_statdem[C2GMSK_STATDEM_NOTSYNCED]="Not synced";
c2gmsk_str_statdem[C2GMSK_STATDEM_RECEIVING_SYNC]="Receiving sync";
c2gmsk_str_statdem[C2GMSK_STATDEM_RECEIVING_SOF]="Receiving Start-of-frame";
c2gmsk_str_statdem[C2GMSK_STATDEM_RECEIVING_HEAD]="Receiving header";
c2gmsk_str_statdem[C2GMSK_STATDEM_RECEIVING_DATA]="Receiving Data";


// AVERAGE AUDIO-LEVEL TEST RESULT
for (loop=0; loop <= C2GMSK_AUDIOAVGLVLTEST_HIGHEST; loop++) {
	c2gmsk_str_avglvltest[loop]=NULL;
}; // end for


c2gmsk_str_avglvltest[C2GMSK_AUDIOAVGLVLTEST_OK]="ok";
c2gmsk_str_avglvltest[C2GMSK_AUDIOAVGLVLTEST_TOLOW]="audio level to low compaired to reference";
c2gmsk_str_avglvltest[C2GMSK_AUDIOAVGLVLTEST_CANCELED]="no sufficient previous data to do average-audio-level test";


}; // end of STR INIT function

// print string: return value
char * c2gmsk_printstr_ret(int msg) {
// valid value for msg?
if ((msg < 0) || (msg > C2GMSK_RET_HIGHEST)) {
	return(NULL);
}; // end if

// ok!
return(c2gmsk_str_ret[msg]);
}; // end function c2gmsk printstr return


// print string: status type of message
char * c2gmsk_printstr_msg(int msg) {
// valid value for msg?
if ((msg < 0) || (msg > C2GMSK_MSG_HIGHEST)) {
	return(NULL);
}; // end if

// ok!
return(c2gmsk_str_msg[msg]);
}; // end function c2gmsk printstr return

// print string: returnvalue average audio-level test
char * c2gmsk_printstr_avglvltest(int msg) {
// valid value for msg?
if ((msg < 0) || (msg > C2GMSK_AUDIOAVGLVLTEST_HIGHEST)) {
	return(NULL);
}; // end if

// ok!
return(c2gmsk_str_avglvltest[msg]);
}; // end function c2gmsk printstr return


// print string: status demodulation
char * c2gmsk_printstr_statdem(int msg) {
// valid value for msg?
if ((msg < 0) || (msg > C2GMSK_STATDEM_HIGHEST)) {
	return(NULL);
}; // end if

// ok!
return(c2gmsk_str_statdem[msg]);
}; // end function c2gmsk printstr return


