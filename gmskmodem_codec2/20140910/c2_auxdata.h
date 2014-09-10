// gmsk modem for codec2 - API

//////////////////////
// API version of the GMSK modem for 10m / VHF / UHF communication
// using codec2
// version 4800/0 (versionid 0x27f301): 4800 bps, 1/3 repetition code FEC
// version 2400/15: 2400 bps, interleave, golay FEC 


// Auxiliary data channel



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


// version 20130606: initial release


// some defines
// maximum size of auxiliary data structure
#define AUXDATA_RECEIVEBUFFERSIZE 1024 // 1KB

/////////////////////////////////////////
// PUBLIC function to set "set send" message
int c2gmsk_auxdata_sendmessage (struct c2gmsk_session * sessid, char * message, int size) {
int ret;

// some checks
ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// is there a message?
if (!message) {
	return(C2GMSK_RET_OK);
}; // end if

// "ret" used as tempory var for size of message
ret=strlen(message);

// free memory old message if present
if (sessid->auxdata_s_data) {
	free(sessid->auxdata_s_data);
}; // end if


// set maximum size (same as maxim size of receive buffer). Currently 1 K
if (ret > AUXDATA_RECEIVEBUFFERSIZE) {
	ret=AUXDATA_RECEIVEBUFFERSIZE;
}; // end if

// allocate memory for new message
sessid->auxdata_s_data=malloc(ret+1);
if (!sessid->auxdata_s_data) {
	return(C2GMSK_RET_NOMEMORY);
}; // end if

// copy message to buffer
strncpy(sessid->auxdata_s_data,message,ret);

// override NULL terminator, if string was truncated
sessid->auxdata_s_data[ret]=0;
sessid->auxdata_s_size=ret;

// special case, if we where already sending something, and we had just send the first
// nibble (i.e. s_nibble is set to "1"), we will need to insert a fake nibble so to keep
// the receiver nibble-counter in sync with the (new) sender
// set nibble to -1
if (sessid->auxdata_s_nibble) {
	sessid->auxdata_s_nibble=-1;
}; // end if


// done
return(C2GMSK_RET_OK);
}; // end function 



// PRIVATE functions

// auxdata_sessinit
// This function is called at the beginning of a stream, allowing
// session-dependent parameters to be initialised

// (re)init to-send text 
int auxdata_sessinit_s (struct c2gmsk_session * sessid) {

sessid->auxdata_s_data=NULL;
sessid->auxdata_s_size=0;

sessid->auxdata_s_octetcount=0;
sessid->auxdata_s_nibble=0;


return(C2GMSK_RET_OK);
}; // end function auxdata_init

// (re)init to-send text 
int auxdata_sessinit_r (struct c2gmsk_session * sessid) {

// allocate memory if needed
if (!sessid->auxdata_r_data) {
printf("auxdata_init for receive! \n");
	sessid->auxdata_r_data=malloc(AUXDATA_RECEIVEBUFFERSIZE+1); // add one for termination NULL

	if (!sessid->auxdata_r_data) {
	// could not allocate memory
		return(C2GMSK_RET_NOMEMORY);
	}; // end if
	sessid->auxdata_r_buffersize=AUXDATA_RECEIVEBUFFERSIZE;
}; // end if

// reinit receiving vars
sessid->auxdata_r_octetcount=0;
sessid->auxdata_r_nibble=0;

return(C2GMSK_RET_OK);
}; // end function auxdata_init_r


// auxdata_receive1
// This function is called when a valid modem-version 2400/0
// dataframe is received
// The left-most (lsb) 4 bits of the parameter "c" contain the
// received auxiliary data
int auxdata_receive1 (struct c2gmsk_session * sessid, unsigned char c) {

// is there place to store data?
if (!sessid->auxdata_r_data) {
	return(C2GMSK_RET_OK);
}; // end if

// store data
// limit checking
if (sessid->auxdata_r_octetcount > AUXDATA_RECEIVEBUFFERSIZE) {
	sessid->auxdata_r_octetcount=AUXDATA_RECEIVEBUFFERSIZE;
}; // end if

// store data
// first nibble of byte
if ((sessid->auxdata_r_nibble & 0x01) == 0) {
	sessid->auxdata_r_data[sessid->auxdata_r_octetcount]=c;
	sessid->auxdata_r_nibble++;

	// done
	return(C2GMSK_RET_OK);
}; // end if (first nibble of byte)

// 2nd nibble, store data in upper bits
sessid->auxdata_r_data[sessid->auxdata_r_octetcount] |= (c<<4);
sessid->auxdata_r_nibble++;

// was it a NULL termination?
if (sessid->auxdata_r_data[sessid->auxdata_r_octetcount] != 0) {
	// No, so get more data
	sessid->auxdata_r_octetcount++; // no need to do limit checking, is
												// done next itiration
	return(C2GMSK_RET_OK);
}; // end if

// received a NULL, return data to API 
// only send data if at least one char received
if (sessid->auxdata_r_octetcount==0) {
	// size = 0, stop and stat at beginning of buffer
	return(C2GMSK_RET_OK);
}; // end if

// complete is set to "1"
queue_d_auxdata (sessid, sessid->auxdata_r_data, sessid->auxdata_r_octetcount, 1);

// reset counters
sessid->auxdata_r_octetcount=0;
sessid->auxdata_r_nibble=0;

// currently not used. Just return
return(C2GMSK_RET_OK);
}; // end function auxdata_receive




//// function auxdata receive flush queue.
// called when stream is terminated to send any auxiliary data that
// might have been send
int auxdata_receive_flush (struct c2gmsk_session * sessid) {
int ret;

// some error checking
assert(sessid != NULL);

// is there data in the receive queue?
if (sessid->auxdata_r_octetcount==0) {
	return(C2GMSK_RET_OK);
}; // end if

// queue data, set "complete" to 0
ret=queue_d_auxdata (sessid, sessid->auxdata_r_data, sessid->auxdata_r_octetcount, 0);

if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// reset counters
sessid->auxdata_r_octetcount=0;
sessid->auxdata_r_nibble=0;

// done
return(C2GMSK_RET_OK);
}; // end function receive _ flush





// auxdata_send1
// this function is called just before a 2400/0 or 2400/15 frame is
// send.
// The left-most (lsb) 4 bits of the parameter "c" is filled in
// to be send as auxdata
int auxdata_send1 (struct c2gmsk_session * sessid, unsigned char * c) {

// some error checking
assert(c != NULL);
assert(sessid != NULL);

// return 0 is no valid data
if (!sessid->auxdata_s_data) {
	*c=0;
	return(C2GMSK_RET_OK);
}; // end if

// nothing to send?
// special case, if nibble is -1 (see c2gmsk_sendmessage) send it anyway, even if size if 0;
if ((sessid->auxdata_s_size <= 0) && (sessid->auxdata_s_nibble != -1)) {
	*c=0;
	return(C2GMSK_RET_OK);
}; // end if


// some error checking on size
if (sessid->auxdata_s_octetcount > sessid->auxdata_s_size) {
	sessid->auxdata_s_octetcount=0;
	sessid->auxdata_s_nibble=0;

	// restart from beginning of message, so continue;
}; // end if


// copy correct octet / correct nibble and move to next octet if needed
if (sessid->auxdata_s_nibble == 0) {
	// bottom nibble (bits 0 to 3)
	*c=(sessid->auxdata_s_data[sessid->auxdata_s_octetcount] & 0x0f);
} else {
	// bit is 1 or  -1 (special case in c2gmsk_auxdata_sendmessage);
	// copy top nibble (bits 4 to 7) of data to lowest bits of "c"
	*c=(sessid->auxdata_s_data[sessid->auxdata_s_octetcount] & 0xf0) >> 4;

	// move up octet-counter
	sessid->auxdata_s_octetcount++;
}; 

// move up nibble-counter
sessid->auxdata_s_nibble++;
sessid->auxdata_s_nibble &= 0x01; // reset nibble, only use lsb bit 

// are we done?
if (sessid->auxdata_s_octetcount > sessid->auxdata_s_size) {
	int ret;
	// queue "auxdata send done" message
	ret=queue_m_msg_0(sessid,C2GMSK_MSG_AUXDATA_DONE);
	if (ret != C2GMSK_RET_OK) {
		return(ret);
	}; // end if

	// reset all vars
	sessid->auxdata_s_size=0;
	sessid->auxdata_s_octetcount=0;
	sessid->auxdata_s_nibble=0;

	// done
	return(C2GMSK_RET_OK);
}; // end if


// done
return(C2GMSK_RET_OK);
}; // end function





////////////////////////////////
// queue received auxiliary data
int queue_d_auxdata (struct c2gmsk_session * sessid, char * txt, int size, int complete) {
c2gmsk_msgauxdata_txt *msg;
int ret;
int memsize_data;
int memsize_total;

// some sanity checks
ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

memsize_data=(size + 1 +  BOUNDARYADD) & BOUNDARYMASK; // add 1 for terminating NULL char
memsize_total=memsize_data + sizeof(c2gmsk_msgauxdata_txt);

msg=malloc(memsize_total);

if (!msg) {
	return(C2GMSK_RET_NOMEMORY);
}; // end if

memcpy(msg->signature,MSG_SIGNATURE,4);
msg->tod=C2GMSK_MSG_AUXDATA ;
msg->datasize=memsize_data + (sizeof(int)<<1); // amount of data (incl. terminating null) , corrected for word boundairy + 2 * integer
msg->realsize=size + 1; // real size (incl. terminating null)
msg->complete=complete;
memcpy(&msg->data,txt,size);
// add terminating null, just to be sure
msg->data[size]=0;

//ret=c2gmskchain_add(sessid->d_chain,msg,sizeof(c2gmsk_msgauxdata_txt)+memsize_data);
ret=c2gmskchain_add(sessid->d_chain,msg,memsize_total);

// free memory of message
free(msg);

// done
return(ret);
}; // end function


