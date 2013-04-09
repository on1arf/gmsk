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


// c2gmsk "printbit" functions: return bitdump of received data

// queue_debug_bit_init
// queue_debug_bit
// queue_debug_bit_flush


//////////////////////
//////////////////////
// PART 1: PRINT MODULATED BIT (printbit)
///////////////

///////////////
int queue_debug_bit_init (struct c2gmsk_session * sessid) {
int ret;

// sanity check
ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
        return(ret);
}; // end if


sessid->d_printbitcount=0;

return(C2GMSK_RET_OK);
}; // end function QUEUE DEBUG ALLBIT : INIT


///////////////
int queue_debug_bit (struct c2gmsk_session * sessid, int bit) {
int ret;

// sanity checks
ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
        return(ret);
}; // end if


// add bit to queue, if there is place
// buffer is 200 octets (iun theory only 192, but add some headroom for PLL sync-issues)
if (sessid->d_printbitcount < 200) {
	if (bit) {
		sessid->d_printbit[sessid->d_printbitcount]='1';
	} else {
		sessid->d_printbit[sessid->d_printbitcount]='0';
	}; // end else - if

	sessid->d_printbitcount++;
}; // end if (is there place?)

return(C2GMSK_RET_OK);

}; // end function QUEUE DEBUG ALLBIT


//////////////////
int queue_debug_bit_flush (struct c2gmsk_session * sessid) {
int ret;

ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// add message to queue, if present
if (sessid->d_printbitcount > 0) {
	int memsize_data, memsize_total;

	// create message structure
	c2gmsk_msgrealwithmarker *msg;

	// allocate memory: size = header + marker + number of octets in buffer
	// datasize has to be correct to next word bounday
	memsize_data=(sessid->d_printbitcount + BOUNDARYADD) & BOUNDARYMASK;
	memsize_total=memsize_data+sizeof(c2gmsk_msgrealwithmarker);


	msg=malloc(memsize_total);

	if (!msg) {
		return(C2GMSK_RET_NOMEMORY);
	}; // end if

	msg->tod=C2GMSK_PRINTBIT_MOD;
	
	msg->datasize=memsize_data + (sizeof(int)<<1); // amount of data, corrected for word boundary + 2 * integer
	msg->realsize=sessid->d_printbitcount; // real amount of data
	msg->marker=(int)sessid->d_marker; // convert char to int 
	memcpy(&msg->data,&sessid->d_printbit,sessid->d_printbitcount);

	// add queue message
	ret=c2gmskchain_add(sessid->d_chain,msg,sizeof(c2gmsk_msgrealwithmarker)+sessid->d_printbitcount); 

	// free memory of chain
	free(msg);
}; // end if


// reinit buffer
sessid->d_printbitcount=0;
memset(&sessid->d_printbit,'X',192);

return(C2GMSK_RET_OK);

}; // end function QUEUE DEBUG ALLBIT : FLUSH



//////////////////////
//////////////////////
// PART 2: PRINT ALL BIT (verbose printbit)
///////////////
int queue_debug_allbit_init (struct c2gmsk_session * sessid) {
int ret;

// sanity check
ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if


sessid->d_printbitcount_v=0;

return(C2GMSK_RET_OK);
}; // end function QUEUE DEBUG ALLBIT : INIT


///////////////
int queue_debug_allbit (struct c2gmsk_session * sessid, int bit) {
int ret;

// sanity checks
ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if


// add bit to queue, if there is place
// buffer is 200 octets (in theory only 192, but add some headroom for PLL syncing)
if (sessid->d_printbitcount_v < 200) {
	if (bit) {
		sessid->d_printbit_v[sessid->d_printbitcount_v]='1';
	} else {
		sessid->d_printbit_v[sessid->d_printbitcount_v]='0';
	}; // end else - if

	sessid->d_printbitcount_v++;
}; // end if (is there place?)

return(C2GMSK_RET_OK);

}; // end function QUEUE DEBUG ALLBIT


//////////////////
int queue_debug_allbit_flush (struct c2gmsk_session * sessid) {
int ret;

ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// add message to queue, if present
if (sessid->d_printbitcount_v > 0) {
	int memsize_real, memsize_corrected, datasize_corrected;

	// create message structure
	c2gmsk_msgreal * msg;

	// allocate memory: size = header + marker + number of octets in buffer
	// datasize has to be correct to next word bounday
	memsize_real=sessid->d_printbitcount_v + sizeof(c2gmsk_msgreal);
	memsize_corrected=(memsize_real + BOUNDARYADD) & BOUNDARYMASK;

	datasize_corrected=(sessid->d_printbitcount_v + BOUNDARYADD) & BOUNDARYMASK;

	msg=malloc(memsize_corrected);

	if (!msg) {
		return(C2GMSK_RET_NOMEMORY);
	}; // end if

	msg->tod=C2GMSK_PRINTBIT_ALL;
	msg->datasize=datasize_corrected; // amount of data, corrected for word boundary
	msg->realsize=sessid->d_printbitcount_v; // real amount of data
	memcpy(&msg->data,&sessid->d_printbit_v,sessid->d_printbitcount_v);

	// add message to chain
	ret=c2gmskchain_add(sessid->d_chain,msg,sizeof(c2gmsk_msgreal)+sessid->d_printbitcount_v+sizeof(int));

	// free memory of chain
	free(msg);
}; // end if


// reinit buffer
sessid->d_printbitcount_v=0;
memset(&sessid->d_printbit_v,'X',192);

return(C2GMSK_RET_OK);

}; // end function QUEUE DEBUG ALLBIT : FLUSH


