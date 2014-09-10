// c2gmskgbuff.h

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
// version 20130614 initial release


// C2GMSK GMSK BUFFERS
// check signature for ABUFF 48k
int checksign_gbuff (gmskbuff * buff) {
// does it point somewhere?
if (!buff) {
	return(C2GMSK_RET_NOVALIDGBUFF);
}; // end if


if (memcmp(buff->signature,GBUFF_SIGNATURE,4)) {
	return(C2GMSK_RET_NOVALIDGBUFF);
}; // end if

// ok
return(C2GMSK_RET_OK);

}; // end function




/////////////////////////////
///////////////////////

// ADD TO BUFFER
int c2gmskgbuff_add (struct c2gmsk_session * sessid, unsigned char *in, int numbits) {
int bitstodo;
unsigned char * p_in;
int inbitcounter;
int ret;

gmskbuff * gbuffer;

// if number of samples is 0 or less, just return
if (numbits <= 0) {
	return(C2GMSK_RET_OK);
}; // end if

// sanity checks
// check sessiod
ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if


// check gmsk buffers (temp buffer)
ret=checksign_gbuff(sessid->m_gbuff);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

gbuffer=sessid->m_gbuff;


// check gbuff in
if (!in) {
	return(C2GMSK_RET_NOVALIDGBUFF);
}; // end if

// check memory chain 
ret=checksign_chain(sessid->m_chain);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// intt some vars
bitstodo=numbits;
inbitcounter=0;

p_in=in; // set pointer to beginning of in-buffer


// loop as long as we have data or the tempory buffer is full
while ((bitstodo > 0) || (gbuffer->used >= gbuffer->size)) {

	// if there data to queue?
	if (gbuffer->used >= gbuffer->size) {
		// queue bits
		// size to copy depends on message (bitrate) = payload + msg header
		ret=c2gmskchain_add(sessid->m_chain,&gbuffer->msg,gbuffer->msg.datasize+sizeof(struct c2gmsk_msg));

		// reinit vars
		gbuffer->used=0;
		gbuffer->bitcount=0;
		gbuffer->p_data=gbuffer->msg.data;
		memset(gbuffer->p_data,0,24); // clear all 24 ocets
	}; // end if


	// process bit if there are to process
	if (bitstodo > 0) {
		// set bit if input != 0
		if ((*p_in & bit2octet[inbitcounter])) {
			*gbuffer->p_data |= bit2octet[gbuffer->bitcount];
		}; // end if

		// move up pointers
		gbuffer->used++;
		bitstodo--;

		// move up ouput bit counter
		gbuffer->bitcount++;
		if (gbuffer->bitcount >= 8) {
			// move up pointer
			gbuffer->p_data++;

			// reinit vars
			gbuffer->bitcount=0;
		}; // end if

		// move up input bitcounter
		inbitcounter++;
		if (inbitcounter >= 8) {
			// move up pointer
			p_in++;

			// reinit vars
			inbitcounter=0;
		}; // end if

	}; // end if
}; // end while

// done: success
return(C2GMSK_RET_OK);
}; // end function gbuffer add



//////////////////////////////
/////////////////
// FLUSH BUFFER
int c2gmskgbuff_flush (gmskbuff * gbuffer, struct c2gmsk_msgchain * chain) {
int ret;

// sanity checks
// check gmsk buffers
ret=checksign_gbuff(gbuffer);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// check memory chain 
ret=checksign_chain(chain);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// if nothing in buffer, just return here
if ((gbuffer->used == 0) && (gbuffer->bitcount == 0)) {
	return(C2GMSK_RET_OK);
}; // end if

// as the complete buffer has been reset with all zero during creation or reinitialising
// there is no need do anything with the remaining bits


// send bits to queue
// size to copy depends on message (bitrate) = payload + msg header
ret=c2gmskchain_add(chain,&gbuffer->msg,gbuffer->msg.datasize+sizeof(struct c2gmsk_msg));

// reinit vars
gbuffer->used=0;
gbuffer->bitcount=0;
gbuffer->p_data=gbuffer->msg.data;
memset(gbuffer->p_data,0,24); // clear all 24 ocets


// done: success
return(C2GMSK_RET_OK);
}; // end function gmsk buffer flush

