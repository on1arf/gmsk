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
// version 20130310 initial release
// Version 20130314: API c2gmsk version / bitrate control + versionid codes
// Version 20130324: convert into .so shared library


// C2GMSK AUDIO BUFFERS
// check signature for ABUFF 48k
int checksign_abuff_48k (audiobuff40_48k * buff) {
// does it point somewhere?
if (!buff) {
	return(C2GMSK_RET_NOVALIDABUFF);
}; // end if


if (memcmp(buff->signature,ABUFF48_SIGNATURE,4)) {
	return(C2GMSK_RET_NOVALIDABUFF);
}; // end if

// ok
return(C2GMSK_RET_OK);

}; // end function




/////////////////////////////
///////////////////////

// ADD TO BUFFER
int c2gmskabuff48_add (audiobuff40_48k * buffer, int16_t *in, int numsample, struct c2gmsk_msgchain * chain) {
int samplestodo;
int samplesnow;
int16_t * p_in;
int ret;

// if number of samples is 0 or less, just return
if (numsample <= 0) {
	return(C2GMSK_RET_OK);
}; // end if

// sanity checks
// check audio buffers
ret=checksign_abuff_48k(buffer);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if


// check audio in
if (!in) {
	return(C2GMSK_RET_NOVALIDAUDIO);
}; // end if

// check memory chain 
ret=checksign_chain(chain);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if


samplestodo=numsample;

p_in=in; // set pointer to beginning of in-buffer


while (samplestodo > 0) {
	// how many samples to process now?
	if (samplestodo < (1920 - (buffer->used))) {
		// we can store all data in this buffer
		samplesnow=samplestodo;
	} else {
		// more data then place available. Fill up corrent buffer
		samplesnow=1920-(buffer->used);
	}; // end if

	// copy data
	if (samplesnow > 0) {
		memcpy(&(buffer->audio.pcm[buffer->used]),p_in,(samplesnow << 1)); // 1 sample = 2 octets, so multiply by 2
		buffer->used += samplesnow;
		p_in += samplesnow;
		samplestodo -=  samplesnow;
	}; // 


	// if buffer is full, write to chain
	if (buffer->used >= 1920) {
		// reinit vars of msg-structure, just to be sure
		memcpy(&buffer->audio.signature,MSG_SIGNATURE,4);
		buffer->audio.tod=C2GMSK_MSG_PCM48K;
		buffer->audio.datasize=3840;
		ret=c2gmskchain_add(chain,&buffer->audio,sizeof(buffer->audio));

		// break out if error
		if (ret != C2GMSK_RET_OK) {
			return(ret);
		}; // end if

		// reinit vars
		buffer->used=0;
	}; // end if
}; // end while

// done: success
return(C2GMSK_RET_OK);
}; // end function abuffer add

// FLUSH BUFFER
int c2gmskabuff48_flush (audiobuff40_48k * buffer, struct c2gmsk_msgchain * chain) {
int ret;

// sanity checks
// check audio buffers
ret=checksign_abuff_48k(buffer);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// check memory chain 
if (!chain) {
	// if no chain found, just reset the audio-buffer but do not
	// store to chain

	// reinit vars
	buffer->used=0;

	// done: success
	return(C2GMSK_RET_OK);
}; // end if


// check if memory chain really c2gmsk memory chain, check signature
ret=checksign_chain(chain);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if


// if nothing in buffer, just return
if (buffer->used == 0) {
	return(C2GMSK_RET_OK);
}; // end if

// if buffer is less then 1920 frames (which is supposed to have as
// it would have been flushed earlier), fill rest with silence

if (buffer->used < 1920) {
	memset(&buffer->audio.pcm[buffer->used],0,((1920-(buffer->used))<<1)); // multiply by 2 as
															// memset uses octets and buffer->user uses frames
}; // end if

// add buffer to chain

// reinit vars of msg-structure, just to be sure
buffer->audio.tod=C2GMSK_MSG_PCM48K;
buffer->audio.datasize=3840;

ret=c2gmskchain_add(chain,&buffer->audio,sizeof(buffer->audio));
// break out if error
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// reinit vars
buffer->used=0;

// done: success
return(C2GMSK_RET_OK);
}; // end function abuffer flush

