// gmsk modem for codec2 - API version


// version 20130122: initial release
// Version 20130314: API c2gmsk version / bitrate control + versionid codes
// Version 20130324: convert into .so shared library



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


// defines
// version id
#define C2GMSK_APIVERSION 20130314


// return values of functions
#define C2GMSK_RET_OK 0
#define C2GMSK_RET_UNSUPPAPIVERS 1
#define C2GMSK_RET_NOTINIT 2
#define C2GMSK_RET_NOMEMORY 3
#define C2GMSK_RET_NOTIMPLEMENTED 4
#define C2GMSK_RET_SESSEXISTS 5
#define C2GMSK_RET_CHAINEXISTS 6
#define C2GMSK_RET_ABUFFEXISTS 7
#define C2GMSK_RET_TOLARGE 8
#define C2GMSK_RET_BADSTATE 9

#define C2GMSK_RET_NOVALIDSESS 10
#define C2GMSK_RET_NOVALIDCHAIN 11
#define C2GMSK_RET_NOVALIDABUFF 12
#define C2GMSK_RET_NOVALIDDATA 13
#define C2GMSK_RET_NOVALIDAUDIO 14
#define C2GMSK_RET_NOVALIDPARAMS 15
#define C2GMSK_RET_NOVALIDRETPOINT 16

#define C2GMSK_RET_UNSUPPORTEDVERSIONID 17
#define C2GMSK_RET_UNSUPPORTEDMODEMBITRATE 18
#define C2GMSK_RET_OPERATIONDISABLED 19



// Type Of Data
#define C2GMSK_MSG_UNDEF 0x00
#define C2GMSK_MSG_NODATA 0x01
#define C2GMSK_MSG_CAPABILITIES 0x02

#define C2GMSK_MSG_STATECHANGE 0x10
#define C2GMSK_MSG_CODEC2 0x20
#define C2GMSK_MSG_PCM8K 0x21
#define C2GMSK_MSG_PCM48K 0x22

// data types used for debug information
#define C2GMSK_MSG_AUDIOAVGLEVEL 0x30
#define C2GMSK_MSG_AUDIOAVGLVLTEST 0x31 // result of audio average audio-level test
#define C2GMSK_MSG_INAUDIOINVERT 0x32
#define C2GMSK_MSG_VERSIONID 0x33
#define C2GMSK_MSG_FECSTATS 0x34

// data types used for "printbit"
#define C2GMSK_PRINTBIT_MOD 0x40
#define C2GMSK_PRINTBIT_ALL 0x41

// debug messages
#define C2GMSK_MSG_UNKNOWNVERSIONID 0x50
#define C2GMSK_MSG_END_NORMAL 0x51
#define C2GMSK_MSG_END_TOMANYMISSEDSYNC 0x52


// results of AVERAGE AUDIO-LEVEL TEST
#define C2GMSK_AUDIOAVGLVLTEST_OK 0
#define C2GMSK_AUDIOAVGLVLTEST_TOLOW 1
#define C2GMSK_AUDIOAVGLVLTEST_CANCELED 2 

// STATUS values for receiver
#define C2GMSK_STATDEM_NOTSYNCED 0
#define C2GMSK_STATDEM_RECEIVING_SYNC 1
#define C2GMSK_STATDEM_RECEIVING_SOF 2
#define C2GMSK_STATDEM_RECEIVING_HEAD 3
#define C2GMSK_STATDEM_RECEIVING_DATA 4


#define C2GMSK_CODEC2_1200 1
#define C2GMSK_CODEC2_1400 2
#define C2GMSK_CODEC2_2400 3

#define C2GMSK_CODEC2_FRAMESIZE_1200 8
#define C2GMSK_CODEC2_FRAMESIZE_1400 7
#define C2GMSK_CODEC2_FRAMESIZE_2400 8

#define C2GMSK_MODEMBITRATE_2400 1
#define C2GMSK_MODEMBITRATE_4800 2


// capabilities of the API
#define C2GMSK_CAP_C2GMSKVER 1
#define C2GMSK_CAP_MODEMBITRATE 2

// defs for msgchain_search
#define C2GMSK_SEARCH_POSSTART 0
#define C2GMSK_SEARCH_POSCURRENT 1



// return message structures

// generic structure
struct c2gmsk_msg;

// memory chain structure
struct c2gmsk_msgchain;


// parameters structure
// GLOBAL parameters per c2gmsk SESSION
// used to init parameters when creating new session
struct c2gmsk_param {
	// the first two fields (signature) and expected API version must always be
	// be located in the beginning of the parameter function, no matter what
	// APIversion is used

	unsigned char signature[4]; // contains signature
	int expected_apiversion; // minimal API version

	// parameters for modulation
	int m_bitrate; // 2400 or 4800 bps ?
	int m_version; // version of c2gmsk

	// parameters for demodulation
	int d_disableaudiolevelcheck;
};


// session
struct c2gmsk_session;


// supportfunctions

// session support functions
int checksign_sess(struct c2gmsk_session * sessid);
struct c2gmsk_session * c2gmsk_sess_new (struct c2gmsk_param *param, int * ret, struct c2gmsk_msgchain ** out);
int c2gmsk_sess_destroy (struct c2gmsk_session * sessid);


// print string functions
char * c2gmsk_printstr_ret (int msg);
char * c2gmsk_printstr_msg (int msg);
char * c2gmsk_printstr_avglvltest (int msg);
char * c2gmsk_printstr_statdem (int msg);

// parameter support functions
int c2gmsk_param_init (struct c2gmsk_param * param);


// API calls itself
int c2gmsk_mod_init (struct c2gmsk_session * sessid, struct c2gmsk_param *param);
int c2gmsk_mod_start (struct c2gmsk_session * sessid,  struct c2gmsk_msgchain ** out);
int c2gmsk_mod_voice1400 (struct c2gmsk_session * sessid, unsigned char * c2dataframe, struct c2gmsk_msgchain ** out);
int c2gmsk_mod_voice1400_end (struct c2gmsk_session * sessid, struct c2gmsk_msgchain ** out);
int c2gmsk_mod_audioflush (struct c2gmsk_session * sessid, struct c2gmsk_msgchain ** out);

int c2gmsk_demod_init (struct c2gmsk_session * sessid, struct c2gmsk_param *param);
int c2gmsk_demod (struct c2gmsk_session * sessid, int16_t  * in, struct c2gmsk_msgchain ** out);



// function to search in msgchains
void * c2gmsk_msgchain_search (int where, struct c2gmsk_msgchain * chain, int * tod, int * datasize, int * numelem);
void * c2gmsk_msgchain_search_tod (int where, struct c2gmsk_msgchain * chain, int tod, int * datasize, int * numelem);

char * c2gmsk_msgdecode_printbit (struct c2gmsk_msg * msg, char * txtbuffer, int marker);
int c2gmsk_msgdecode_numeric (struct c2gmsk_msg * msg, int *data);
int c2gmsk_msgdecode_c2 (struct c2gmsk_msg * msg, unsigned char * c2buff);

// msgdecode pcm48k -> copy data, pcm48k_p -> only copy pointer
int c2gmsk_msgdecode_pcm48k (struct c2gmsk_msg * msg, int16_t  pcmbuff[]);
int c2gmsk_msgdecode_pcm48k_p (struct c2gmsk_msg * msg, int16_t *pcmbuff_ptr[]);


// get version
int c2gmsk_getapiversion ();
