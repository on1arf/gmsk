/* gmskmodulate.h */
// API release

/*
 *      Copyright (C) 2011 - 2013 by Kristoff Bonne, ON1ARF
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


// Release information:
// version 20111213: initial release
// version 20130310: API release
// Version 20130314: API c2gmsk version / bitrate control + versionid codes
// Version 20130324: convert into .so shared library


int16_t process_return (int64_t filterret) {
// convert 48 bit result back to 16 bits
int bit30;
int16_t filterret16;

// remember 30th bit, is value just below unity
bit30=filterret & 0x40000000;

if (filterret >= 0) {
	// positive
	filterret16=((filterret & 0x3fff80000000LL)>>31);

	// add one for rounding if bit 30 was set
	if (bit30) {
		filterret16++;
	};
} else {
	// negative
	filterret16=-((-filterret & 0x3fff80000000LL)>>31);

	// subtract one for rounding if bit 30 was not set
	if (!(bit30)) {
		filterret16--;
	}; // end if
};


return(filterret16);

}; // end function process_return


int modulate1bit (struct c2gmsk_session * sessid, int bit, int16_t * audioret) {
// local vars
int loop;
int ret;

// filterret only used for int64
#if _USEFLOAT == 0
#if _INTMATH == 64
int64_t filterret;
#endif
#endif
int16_t ret16;

int16_t *p=NULL;

// some error checking
// check session
ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// check audioret
assert(audioret != NULL);

p=audioret;

// send 10 audio-samples to firfilter per bit
if (bit) {
// bit = 1 -> send -16384 to firfilter
	for (loop=0; loop<10; loop++) {
#if _USEFLOAT == 1
		ret16=(int)(firfilter_modulate(sessid, -16384));
#else
#if _INTMATH == 64
		// 16 bit audio
		filterret=firfilter_modulate(sessid, -16384);
		ret16=process_return(filterret);
#else
#if _INTMATH == 3212
		// 12 bit audio
		ret16=(firfilter_modulate(sessid, -1024)>> 16);
#else
		// mathint 32_10
		// 10 bit audio
		ret16=(firfilter_modulate(sessid, -256)>> 16);
#endif
#endif
#endif

		// store data
		*p=ret16;
		p++;
	}; // end for
} else {
// bit = 0 -> send +16384 to firfilter
	for (loop=0; loop<10; loop++) {
#if _USEFLOAT == 1
		ret16=(int)(firfilter_modulate(sessid, 16384));
#else
#if _INTMATH == 64
		// 16 bit audio
		filterret=firfilter_modulate(sessid, 16384);
		ret16=process_return(filterret);
#else 
#if _INTMATH == 3212
		// 12 bit audio
		ret16=(firfilter_modulate(sessid, 1024)>> 16);
#else
		// mathint 32_10
		// 10bit audio
		ret16=(firfilter_modulate(sessid, 256)>> 16);
#endif
#endif
#endif

		// store data
		*p=ret16;
		p++;
	}; // end for
}; // end if

return(C2GMSK_RET_OK);

}; // end function modulate1bit



