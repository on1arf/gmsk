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

// version 20121222: initial release
// version 20130404: added halfduplex support




void * funct_net_receive (void * globalin) {
globaldatastr * pglobal;
pglobal=(globaldatastr *) globalin;

// local vars
// network related
unsigned char *udpbuffer;
int udpsize;
int udpsock;
struct sockaddr_in localsa4;
struct sockaddr_in6 localsa6;

struct sockaddr * receivefromsa;
socklen_t receivefromsa_size=0;

// C2ENCAP types
uint8_t *c2encap_type, *c2encap_begindata;
c2encap c2_voice, c2_begin, c2_end;

// codec2
int mode, nc2byte;
void * codec2;

// audio
int16_t audiobuff8k[320]; // 40 ms of audio @ 8k

// other vars
int ret;
int state;
int new_ptr_write;


// initialise UDP buffer
udpbuffer=malloc(1500); // we can receive up to 1500 octets

if (!udpbuffer) {
	fprintf(stderr,"Error: could not allocate memory for udpbuffer!\n");
	exit(-1);
}; // end if

// set pointers for c2encap type and c2encap begin-of-data
c2encap_type = (uint8_t*) &udpbuffer[3];
c2encap_begindata = (uint8_t*) &udpbuffer[4]; 


// INIT NETWORK

// open inbound UDP socket and bind to socket

// if neither ipv4 or ipv6 is forced, first try opening socket in ipv6
// if it fails with "Address family not supported", we can
// try again using ipv4

if ((!pglobal->ipv4only) && (!pglobal->ipv6only)) {
	udpsock=socket(AF_INET6,SOCK_DGRAM,0);

	if ((udpsock < 0) && (errno == EAFNOSUPPORT)) {
		pglobal->ipv4only=1;
	}; // end if
}; // end if


if (pglobal->ipv4only) {
	udpsock=socket(AF_INET,SOCK_DGRAM,0);

	if (udpsock < 0) {
		fprintf(stderr,"Error creating socket in at_net_received: %d (%s) \n",errno,strerror(errno));
		exit(-1);
	}; // end if

	localsa4.sin_family=AF_INET;
	localsa4.sin_port=htons(pglobal->p_udpport);
	memset(&localsa4.sin_addr,0,sizeof(struct in_addr)); // address = "0.0.0.0" ) -> we listen on all interfaces

	receivefromsa=(struct sockaddr *) &localsa4;

	ret=bind(udpsock, receivefromsa, sizeof(localsa4)); 

} else {
	// IPV6 socket can handle both ipv4 and ipv6

	// socket is already opened above if ipv4 and ipv6 are not forced

	// only redo "open" if ipv6 is forced
	if (pglobal->ipv6only) {
		udpsock=socket(AF_INET6,SOCK_DGRAM,0);
	}; // end if

	if (udpsock < 0) {
		fprintf(stderr,"Error creating socket in at_net_received: %d (%s) \n",errno,strerror(errno));
		exit(-1);
	}; // end if

	// if ipv6 only, set option
	if (pglobal->ipv6only) {
		int yes=1;

		// make socket ipv6-only.
		ret=setsockopt(udpsock,IPPROTO_IPV6, IPV6_V6ONLY, (char *)&yes,sizeof(int));
		if (ret == -1) {
			fprintf(stderr,"Error: IPV6ONLY option set but could not make socket ipv6 only: %d (%s)! \n",errno,strerror(errno));
			exit(-1);
		}; // end if
	}; // end if

	localsa6.sin6_family=AF_INET6;
	localsa6.sin6_port=htons(pglobal->p_udpport);
	localsa6.sin6_flowinfo=0; // flows not used
	localsa6.sin6_scope_id=0; // we listen on all interfaces
	memset(&localsa6.sin6_addr,0,sizeof(struct in6_addr)); // address = "::" (all 0) -> we listen on all interfaces

	receivefromsa=(struct sockaddr *)&localsa6;

	ret=bind(udpsock, receivefromsa, sizeof(localsa6)); 

}; // end else - elsif - if

if (ret < 0) {
	fprintf(stderr,"Error: could not bind network-address to socket: %d (%s) \n",errno,strerror(errno));
	exit(-1);
}; // end if


// INIT C2ENCAP

// init c2encap structures
memcpy(c2_begin.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2_begin.header[3]=C2ENCAP_MARK_BEGIN;
memcpy(c2_begin.c2data.c2data_text3,"BEG",3);

memcpy(c2_end.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2_end.header[3]=C2ENCAP_MARK_END;
memcpy(c2_end.c2data.c2data_text3,"END",3);

memcpy(c2_voice.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2_voice.header[3]=C2ENCAP_DATA_VOICE1400;


// INIT CODEC2 

// in the mean time, do some other things while the audio-process initialises
// init codec2
mode = CODEC2_MODE_1400;
codec2 = codec2_create (mode);

nc2byte = (codec2_bits_per_frame(codec2) + 7) >> 3; // ">>3" is same as "/8"

if (nc2byte != 7) {
	fprintf(stderr,"Error: number of bytes for codec2 frames should be 7. We got %d \n",nc2byte);
}; // end if

if (codec2_samples_per_frame(codec2) != 320) {
	fprintf(stderr,"Error: number of samples for codec2 frames should be 320. We got %d \n",codec2_samples_per_frame(codec2));
}; // end if


// sanity check. Just to be sure, check "channelCount" setting, should be "0" or "1"
if ((pglobal->inputParameters.channelCount != 1) && (pglobal->inputParameters.channelCount != 2)) {
	fprintf(stderr,"Internal error: stereo flag for playback in funct_net_receive not set correctly by main thread! Should not happen! Exiting. \n");
	exit(-1);
}; // end if


// init state
state=0; // state 0 = wait for start

while (FOREVER ) {
	// wait for UDP packets

	// read until read or error, but ignore "EINTR" errors
	while (FOREVER) {
		udpsize = recvfrom(udpsock, udpbuffer, 1500, 0, receivefromsa, &receivefromsa_size);

		if (udpsize > 0) {
			// break out if really packet received;
			break;
		}; // end if

		// break out when not error EINTR
		if (errno != EINTR) {
			break;
		}; // end if
	}; // end while (read valid UDP packet)

	if (udpsize < 0) {
		// error: print message, wait 1/4 of a second and retry
		fprintf(stderr,"Error receiving UDP packets: %d (%s) \n",errno, strerror(errno));
		usleep(250000);
		continue;
	}; // end if


	if (udpsize < 4) {
		// should be at least 4 octets: to small, ignore it
		fprintf(stderr,"Error: received UDP packet to small (size = %d). Ignoring! \n",udpsize);
		continue;
	}; // end if


	// check beginning of frame, it should contain the c2enc signature
	if (memcmp(udpbuffer,C2ENCAP_HEAD,3)) {
		// signature does not match, ignore packet
		continue;
	}; // end  if
	
	// check size + content
	// we know the udp packet is at least 4 octets, so check 4th char for type
	if (*c2encap_type == C2ENCAP_MARK_BEGIN) {
		if (udpsize < C2ENCAP_SIZE_MARK ) {
			fprintf(stderr,"Error: received C2ENCAP BEGIN MARKER with to small size: %d octets! Ignoring\n",udpsize);
			continue;
		} else if (udpsize > C2ENCAP_SIZE_MARK) {
			fprintf(stderr,"Warning: received C2ENCAP BEGIN MARKER with to large size: %d octets! Ignoring extra data\n",udpsize);
		};

		// check content
		if (memcmp(c2encap_begindata,"BEG",3)) {
			fprintf(stderr,"Error: RECEIVED C2ENCAP BEGIN MARKER WITH INCORRECT TEXT: 0X%02X 0X%02X 0X%02X. Ignoring frame!\n",udpbuffer[4],udpbuffer[5],udpbuffer[6]);
			continue;
		}; // end if
	} else if (*c2encap_type == C2ENCAP_MARK_END) {
		if (udpsize < C2ENCAP_SIZE_MARK ) {
			fprintf(stderr,"Error: received C2ENCAP END MARKER with to small size: %d octets! Ignoring\n",udpsize);
			continue;
		} else if (udpsize > C2ENCAP_SIZE_MARK) {
			fprintf(stderr,"Warning: received C2ENCAP END MARKER with to large size: %d octets! Ignoring extra data\n",udpsize);
		};

		// check content
		if (memcmp(c2encap_begindata,"END",3)) {
			fprintf(stderr,"Error: RECEIVED C2ENCAP BEGIN MARKER WITH INCORRECT TEXT: 0X%02X 0X%02X 0X%02X. Ignoring frame!\n",udpbuffer[4],udpbuffer[5],udpbuffer[6]);
			continue;
		}; // end if
	} else if (*c2encap_type == C2ENCAP_DATA_VOICE1200) {
		if (udpsize < C2ENCAP_SIZE_VOICE1200 ) {
			fprintf(stderr,"Warning: received C2ENCAP VOICE1200 with to small size: %d octets! Ignoring\n",udpsize);
			continue;
		} else if (udpsize > C2ENCAP_SIZE_VOICE1200) {
			fprintf(stderr,"Warning: received C2ENCAP VOICE1200 with to large size: %d octets! Ignoring extra data\n",udpsize);
		};

	} else if (*c2encap_type == C2ENCAP_DATA_VOICE1400) {
		if (udpsize < C2ENCAP_SIZE_VOICE1400 ) {
			fprintf(stderr,"Warning: received C2ENCAP VOICE1400 with to small size: %d octets! Ignoring\n",udpsize);
			continue;
		} else if (udpsize > C2ENCAP_SIZE_VOICE1400) {
			fprintf(stderr,"Warning: received C2ENCAP VOICE1400 with to large size: %d octets! Ignoring extra data\n",udpsize);
		};
	} else if (*c2encap_type == C2ENCAP_DATA_VOICE2400) {
		if (udpsize < C2ENCAP_SIZE_VOICE2400 ) {
			fprintf(stderr,"Warning: received C2ENCAP VOICE2400 with to small size: %d octets! Ignoring\n",udpsize);
			continue;
		} else if (udpsize > C2ENCAP_SIZE_VOICE2400) {
			fprintf(stderr,"Warning: received C2ENCAP VOICE2400 with to large size: %d octets! Ignoring extra data\n",udpsize);
		};
	} else {
		fprintf(stderr,"Warning: received packet with unknown type of C2ENCAP type: 0X%02X. Ignoring!\n",*c2encap_type);
		continue;
	}; // end if


	// processing from here on depends on state
	if (state == 0) {
		// state 0, waiting for start data

		if (*c2encap_type == C2ENCAP_MARK_BEGIN) {
			// received start, go to state 1
			state=1;
			continue;
#if C2ENCAP_STRICT == 0
		// less strict checking
		} else if (*c2encap_type == C2ENCAP_MARK_END) {
			// do nothing, silently ignore and go to next packet
			continue;
		} else if (*c2encap_type == C2ENCAP_DATA_VOICE1400) {
			// we should have received a "start", but go to state 1 anyway.
			// do not do a "continue" as we do want the current frame to be
			// processed below
			state=1;
#else 
		} else {
			fprintf(stderr,"Warning: received packet of type 0X%02X in state 0. Ignoring packet! \n",*c2encap_type);
			continue;
#endif
		}; // end if
		continue;
	};	// end if (state == 0)

	if (state == 1) {
		// state 1: receiving voice data, until we receive a "end" marker
		if (*c2encap_type == C2ENCAP_MARK_END) {
			// end received. Go back to state 0
			state=0;
			continue;
#if C2ENCAP_STRICT == 0
		// silently ignore any additional "start" messages
		} else if (*c2encap_type == C2ENCAP_MARK_BEGIN) {
			// do nothing, ignore and go to next packet
			continue;
#endif

		} else if (*c2encap_type != C2ENCAP_DATA_VOICE1400) {
			fprintf(stderr,"Warning: received packet of type 0X%02X in state 1. Ignoring packet! \n",*c2encap_type);
			continue;
		} else {
			// voice 1400 data packet. Decode and play out

			// queue data, not when in "halfduplex" mode and sending
			if (!((pglobal->halfduplex) && (pglobal->transmit))) {
				// first check if there is place to store the result
				new_ptr_write = pglobal->p_ptr_write+1;
				if (new_ptr_write >= NUMBUFF) {
					// wrap around at NUMBUFF
					new_ptr_write=0;
				}; // end if

				if (new_ptr_write == pglobal->p_ptr_read) {
					// oeps. No buffers available to write data
					fputc('B',stderr);
				} else {
//				fputc('Q',stderr);

					// decode codec2 frame
  					codec2_decode(codec2, audiobuff8k,c2encap_begindata);
				
					// convert to 48000k, store in ringbuffer
					convert_8kto48k(audiobuff8k,pglobal->p_ringbuffer[new_ptr_write],0);
				
					// make stereo (duplicate channels) if needed
					if (pglobal->outputParameters.channelCount == 2) {
						int loop;
						int16_t *p1, *p2;
	
						// codec2_decode returns a buffer of 16-bit samples, MONO
						// so duplicate all samples, start with last sample, move down to sample "1" (not 0);
						p1=&pglobal->p_ringbuffer[new_ptr_write][3839]; // last sample of buffer (1920 samples stereo = 3840 samples mono)
						p2=&pglobal->p_ringbuffer[new_ptr_write][1919]; // last sample of buffer (mono)

						for (loop=0; loop < 1919; loop++) {
							*p1 = *p2; p1--; // copy 1st octet, move down "write" pointer
							*p1 = *p2; p1--; p2--; // copy 2nd time, move down both pointers
						}; // end if

						// last sample, just copy (no need anymore to move pointers)
						*p1 = *p2;
					}; // end if

					// move up pointer in global vars
					pglobal->p_ptr_write=new_ptr_write;
				}; // end if

			}; // end if (not (halfduplex and sendinf))
		}; // end else - elsif - if
	} else {
		fprintf(stderr,"Internal Error: unknow state %d in audioplay main loop. Should not happen. Exiting!!!\n",state);
		exit(-1);
	}; // end if

}; // end while

fprintf(stderr,"Internal Error: audiotool funct_network_receive drops out of endless loop. Should not happen! \n");
exit(-1);

}; // end function funct_net_receive
