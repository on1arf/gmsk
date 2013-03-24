// gmsk modem for codec2 - API version

// defines
// version id
#define C2GMSK_APIVERSION 20130321


// signatures of internal data structures
#define SESS_SIGNATURE "C2id"
#define CHAIN_SIGNATURE "C2ch"
#define ABUFF48_SIGNATURE "C2a4"
#define ABUFF8_SIGNATURE "C2a8"
#define PARAM_SIGNATURE "C2pa" // parameters 


// return values of functions
#define C2GMSK_RET_HIGHEST 19
char *c2gmsk_str_ret[C2GMSK_RET_HIGHEST+1];

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
#define C2GMSK_MSG_HIGHEST 0x52
char *c2gmsk_str_msg[C2GMSK_MSG_HIGHEST+1];

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
#define C2GMSK_AUDIOAVGLVLTEST_HIGHEST 2
char *c2gmsk_str_avglvltest[C2GMSK_AUDIOAVGLVLTEST_HIGHEST+1];

#define C2GMSK_AUDIOAVGLVLTEST_OK 0
#define C2GMSK_AUDIOAVGLVLTEST_TOLOW 1
#define C2GMSK_AUDIOAVGLVLTEST_CANCELED 2 

// STATUS values for receiver
#define C2GMSK_STATDEM_HIGHEST 4
char *c2gmsk_str_statdem[C2GMSK_STATDEM_HIGHEST+1];

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
typedef struct {
	int tod; // type of data
	int datasize; // whatever size it is
	unsigned char data[]; // unknown size, will not be included in sizeof
} c2gmsk_msg; 


// return messages concisting of 0, 1, 2, 3 or 4 data fields
// no datafields
typedef struct {
	int tod; // type of data
	int datasize; // 0
} c2gmsk_msg_0; // structure msg no datafields

typedef struct {
	int tod; // type of data
	int datasize; // 0
	int data0;
} c2gmsk_msg_1; // structure msg one datafield

typedef struct {
	int tod; // type of data
	int datasize; // 0
	int data0;
	int data1;
} c2gmsk_msg_2; // structure msg two datafields

typedef struct {
	int tod; // type of data
	int datasize; // 0
	int data0;
	int data1;
	int data2;
} c2gmsk_msg_3; // structure msg three datafields

typedef struct {
	int tod; // type of data
	int datasize; // 0
	int data0;
	int data1;
	int data2;
	int data3;
} c2gmsk_msg_4; // structure msg four datafields


// 40 ms of codec2 voice
typedef struct {
	int tod; // C2GMSK_MSG_CODEC2
	int datasize; // 7, 8 or corrected for 32 bit wordboundary
	int realsize; // 7 or 8
	int version; // 1 = 1200, 2 = 1400, 3 = 2400
	unsigned char c2data[8]; // 8 octets, large enough for 1200, 1400 and 2400 bps codec2
} c2gmsk_msg_codec2; // structure codec2 voice

// 40 ms of PCM voice (8k or 48 k)
typedef struct {
	int tod; // C2GMSK_MSG_PCM8K
	int datasize; // 640 (40 ms @ 8k = 320 samples: @ 16bit/sample = 640 octets)
	int16_t pcm[320];
} c2gmsk_msg_pcm8k; // structure 8k pcm audio

typedef struct {
	int tod; // C2GMSK_MSG_PCM48K
	int datasize; // 3840 (40 ms @ 8k = 1920 samples: @ 16bit/sample = 3840 octets)
	int16_t pcm[1920];
} c2gmsk_msg_pcm48k; // structure 48k pcm audio


// debug messages used by the demodulator
typedef struct {
	int tod; // C2GMSK_DEBUG_AUDIOAVG
	int datasize; // (sizeof int)
	int average;
} c2gmsk_debug_audioaverage; // average audio level

// structures used for printbit-verbose
// same as generic "msg" + real length
typedef struct {
	int tod; // type of data
	int datasize; // whatever size it is, stretched out to next word boundairy
	int realsize; // real amount of data, excluding padding data for word-boundairy
	unsigned char data[]; // unknown size, will no be included in "sizeof()"
} c2gmsk_msgreal; 

// structures used for printbit-verbose
// same as generic "msg" + real length + marker
typedef struct {
	int tod; // type of data
	int datasize; // whatever size it is, stretched out to next word boundairy
	int realsize; // real amount of data, excluding padding data for word-boundairy
	int marker; // is integer instead of char ... because else 'sizeof(c2gmsk_msgwithmarker' would give incorrect size))
	unsigned char data[]; // unknown size, will no be included in "sizeof()"
} c2gmsk_msgrealwithmarker; 



// memory chain structure
typedef struct {
	unsigned char signature[4]; // contains signature
	
	int size; // total size of chain
	int used; // amount of memory in use
	int numelem; // number of elements
	int s_pos; // search-position in octets
	int s_numelem; // search-position in number-of-element
	unsigned char * data; // pointer to datastructure
} msgchain; // structure for memory chain


// parameters structure

// GLOBAL parameters per c2gmsk SESSION
// used to init parameters when creating new session
typedef struct {
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
} c2gmsk_param;



// 40 ms audio-buffer
typedef struct {
	unsigned char signature[4]; // contains signature
	int used; // number of frames in use 
   c2gmsk_msg_pcm48k audio; // 48k audio, this contains a copy
										// of a c2gmsk_msg record which makes
										// it easier to copy to a chain 
} audiobuff40_48k;

// session
typedef struct {
	unsigned char signature[4]; // contains signature


	// parameters for the modulator
	int m_bitrate;
	int m_version;

	// data for modulator
	msgchain * m_chain;
	audiobuff40_48k m_abuff;
	
	int m_state;

	int m_v1400_framecounter;


	// data for DSP filters
	// for firfilter_modulate
	int md_filt_init;
	int md_filt_pointer;
	
#if _USEFLOAT == 1
	float *md_filt_buffer;
#else
	int16_t *md_filt_buffer;
#endif



	// parameters for demodulator
	int d_disableaudiolevelcheck;

	// data for demodulator
	msgchain * d_chain; 

	int d_state;
	int d_audioinvert;

	// maxaudiolevel check disabled (see note in gmskmodemapi.c)
	// int d_maxaudiolevelvalid;
	// int d_maxaudiolevelvalid_total;
	// int d_maxaudiolevelvalid_numbersample;
	// int d_countnoiseframe;
	// int d_framesnoise;

	int d_audioaverage;
	int d_audiolevelindex;
	int d_audioleveltable[32];
	int16_t d_last2octets;
	int32_t d_last4octets;
	int d_syncreceived;
	int d_inaudioinvert;
	int d_bitcount;
	int d_octetcount;
	int d_framecount;
	int d_missedsync;
	int d_syncfound;
	int d_endfound;
	char d_marker;
	unsigned char d_codec2frame[7];


	unsigned char d_codec2versionid[3];
	unsigned char d_codec2inframe[24]; 

	// textbuffers for "printbit"
	char d_printbit[200];
	int d_printbitcount;

	// textbuffer for "printbit verbose"
	char d_printbit_v[200];
	int d_printbitcount_v;

	// data for DSP filters
	// for "demodulate"
	int dd_dem_init;
	int dd_dem_last;
	int dd_dem_m_pll;

	// for firfilter_demodulate
	int dd_filt_init;
	int dd_filt_pointer;

	
#if _USEFLOAT == 1
	float *dd_filt_buffer;
#else
	int16_t *dd_filt_buffer;
#endif

} session;


// supportfunctions

// session support functions
int checksign_sess(session * sessid);
session * c2gmsk_sess_new (c2gmsk_param *param, int * ret, msgchain ** out);
int c2gmsk_sess_destroy (session * sessid);

// functions related to chains
msgchain * c2gmskchain_new (int size, int * ret);
int c2gmskchain_destroy (msgchain * chain);
int c2gmskchain_add (msgchain * chain, void * data, int size);
int c2gmskchain_reinit (msgchain * chain, int size);

// audio buffer functions for PCM 48k
int c2gmskabuff48_add (audiobuff40_48k * buffer, int16_t *in, int numsample, msgchain * chain);
int c2gmskabuff48_flush (audiobuff40_48k * buffer, msgchain * chain);
int c2gmskabuff48_modulatebits (session * sessid, unsigned char *in, int nbits, int orderinvert);

// printbit functions
int queue_debug_bit_init (session * sessid);
int queue_debug_bit (session * sessid, int bit);
int queue_debug_bit_flush (session * sessid);

int queue_debug_allbit_init (session * sessid);
int queue_debug_allbit (session * sessid, int bit);
int queue_debug_allbit_flush (session * sessid);


// print string functions
void c2gmsk_printstr_init();
char * c2gmsk_printstr_ret (int msg);
char * c2gmsk_printstr_msg (int msg);
char * c2gmsk_printstr_avglvltest (int msg);
char * c2gmsk_printstr_statdem (int msg);

// parameter support functions
int c2gmsk_param_init (c2gmsk_param *);
int checksign_param (c2gmsk_param * param);


// API calls itself
int c2gmsk_mod_init (session * sessid, c2gmsk_param *param);
int c2gmsk_mod_start (session * sessid,  msgchain ** out);
int c2gmsk_mod_voice1400 (session * sessid, unsigned char * c2dataframe, msgchain ** out);
int c2gmsk_mod_voice1400_end (session * sessid, msgchain ** out);
int c2gmsk_mod_audioflush (session * sessid, msgchain ** out);

int c2gmsk_demod (session * sessid, int16_t  * in, msgchain ** out);
int c2gmsk_demod_init (session * sessid, c2gmsk_param *param);



// function to search in msgchains
void * c2gmsk_msgchain_search (int where, msgchain * chain, int * tod, int * datasize, int * numelem);
void * c2gmsk_msgchain_search_tod (int where, msgchain * chain, int tod, int * datasize, int * numelem);

char * c2gmsk_msgdecode_printbit (c2gmsk_msg * msg, char * txtbuffer, int marker);
int c2gmsk_msgdecode_numeric (c2gmsk_msg * msg, int *data);
int c2gmsk_msgdecode_c2 (c2gmsk_msg * msg, unsigned char * c2buff);
int c2gmsk_msgdecode_pcm48k (c2gmsk_msg * msg, int16_t * c2buff);


// get version
int c2gmsk_getapiversion ();
