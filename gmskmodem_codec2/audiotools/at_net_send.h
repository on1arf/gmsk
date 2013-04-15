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




void * funct_net_send (void * globalin) {
globaldatastr * pglobal;
pglobal=(globaldatastr *) globalin;


// local vars

// DNS related
char ipaddress_txt[INET6_ADDRSTRLEN];
struct addrinfo * hint=NULL;
struct addrinfo * info=NULL;

// network related
struct sockaddr_in * udp_aiaddr_in = NULL;
struct sockaddr_in6 * udp_aiaddr_in6 = NULL;
struct sockaddr * sendto_aiaddr = NULL; // structure used for sendto
int sendto_sizeaiaddr=0; // size of structed used for sendto
int udpsd;
int udp_family;


// vars for c2encap
c2encap c2_voice;
c2encap c2_begin, c2_end;
unsigned char *c2_buff;


// vars for codec2
void *codec2;
int mode, nc2byte;

// audio buffer + ringbuffer related
int16_t audiobuff8k[320]; // 40 ms of audio @ 8 KHz
int new_c_ptr_read;

// other vars
int ret;
int state, oldstate, changeofstate;


// function starts here
// do DNS lookup for ipaddress
hint=malloc(sizeof(struct addrinfo));

if (!hint) {
	fprintf(stderr,"Error: could not allocate memory for hint!\n");
	exit(-1);
}; // end if

// clear hint
memset(hint,0,sizeof(hint));

hint->ai_socktype = SOCK_DGRAM;
hint->ai_protocol = IPPROTO_UDP;

// resolve hostname, use function "getaddrinfo"
// set address family of hint if ipv4only or ipv6only
if (pglobal->ipv4only) {
	hint->ai_family = AF_INET;
} else if (pglobal->ipv6only) {
	hint->ai_family = AF_INET6;
} else {
	hint->ai_family = AF_UNSPEC;
}; // end else - elsif - if

// do DNS-query, use getaddrinfo for both ipv4 and ipv6 support
ret=getaddrinfo(pglobal->c_ipaddr, NULL, hint, &info);

// some linux kernels do not support the ai_socktype in getaddressinfo
// when we receive error "ai_socktype" not supported, try again without
// a sockettype
if (ret == EAI_SOCKTYPE) {
	// any type
	hint->ai_socktype = 0;

	// do DNS-query, use getaddrinfo for both ipv4 and ipv6 support
	ret=getaddrinfo(pglobal->c_ipaddr, NULL, hint, &info);
}; // end if

if (ret != 0) {
	fprintf(stderr,"Error: resolving hostname %s failed: %d (%s)\n",pglobal->c_ipaddr,ret,gai_strerror(ret));
	exit(-1);
}; // end if


// additional check. Make sure the info structure does point to a valid structure

if (!info) {
	fprintf(stderr,"Internal error: getaddrinfo returns NULL pointer without error indication!\n");
	exit(-1);
}; // end if
udp_family=info->ai_family;

// open UDP socket + set udp port
if (udp_family == AF_INET) {
	// create UDP socket (check will be done later on)
	udpsd=socket(AF_INET,SOCK_DGRAM,0);
        
	// getaddrinfo returns pointer to generic "struct sockaddr" structure.
	//              Cast to "struct sockaddr_in" to be able to fill in destination port
	udp_aiaddr_in=(struct sockaddr_in *)info->ai_addr;
	udp_aiaddr_in->sin_port=htons((unsigned short int) pglobal->c_udpport);

	// set pointer to be used for "sendto" ipv4 structure
	// sendto uses generic "struct sockaddr" just like the information
	//              returned from getaddrinfo, so no casting needed here
	sendto_aiaddr=info->ai_addr;
	sendto_sizeaiaddr=sizeof(struct sockaddr_in);

	inet_ntop(AF_INET,&udp_aiaddr_in->sin_addr,ipaddress_txt,INET6_ADDRSTRLEN);
        
} else if (udp_family == AF_INET6) {
	// create UDP socket (check will be done later on)
	udpsd=socket(AF_INET6,SOCK_DGRAM,0);

	// getaddrinfo returns pointer to generic "struct sockaddr" structure.
	//              Cast to "struct sockaddr_in6" to be able to fill in destination port
	udp_aiaddr_in6=(struct sockaddr_in6 *)info->ai_addr;
	udp_aiaddr_in6->sin6_port=htons((unsigned short int) pglobal->c_udpport);

	// set pointer to be used for "sendto" ipv6 structure
	// sendto uses generic "struct sockaddr" just like the information
	//              returned from getaddrinfo, so no casting needed here
	sendto_aiaddr=info->ai_addr;
	sendto_sizeaiaddr=sizeof(struct sockaddr_in6);


	// get textual version of returned ip-address
	inet_ntop(AF_INET6,&udp_aiaddr_in6->sin6_addr,ipaddress_txt,INET6_ADDRSTRLEN);
        
} else {
	fprintf(stderr,"Error: DNS query for %s returned an unknown network-family: %d \n",pglobal->c_ipaddr,udp_family);
	exit(-1);
}; // end if

// getaddrinfo can return multiple results, we only use the first one
// give warning is more then one result found.
// Data is returned in info as a linked list
// If the "next" pointer is not NULL, there is more then one
// element in the chain

if (info->ai_next != NULL) {
	fprintf(stderr,"Warning. getaddrinfo returned multiple entries. Using %s\n",ipaddress_txt);
}; // end if

if (udpsd < 0) {
	fprintf(stderr,"Error: could not create socket for UDP! \n");
	exit(-1);
}; // end if


// init c2encap structures
memcpy(c2_begin.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2_begin.header[3]=C2ENCAP_MARK_BEGIN;
memcpy(c2_begin.c2data.c2data_text3,"BEG",3);

memcpy(c2_end.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2_end.header[3]=C2ENCAP_MARK_END;
memcpy(c2_end.c2data.c2data_text3,"END",3);

memcpy(c2_voice.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2_voice.header[3]=C2ENCAP_DATA_VOICE1400;

c2_buff = (unsigned char *)&c2_voice.c2data.c2data_data7;


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


// network send initialisation is finished, set "network ready" marker so that main
// application can kick of portaudio loop
pglobal->networksendready=1;


// MAIN LOOP: wait for audio from ringbuffer, process it and start send out if needed
// init vars
oldstate=0;
// loop forever
while (FOREVER) {

	// reiinit "change of state"
	changeofstate=0;

	// loop until data received or change of state (during halfduplex)
	while (FOREVER) {
		// wait for data on data on ringbuffer
		new_c_ptr_read=pglobal->c_ptr_read+1;

		if (new_c_ptr_read >= NUMBUFF) {
			new_c_ptr_read=0;
		};

		// is there data to process?
		if (new_c_ptr_read == pglobal->c_ptr_write) {
			// no data to process
			// sleep 4 ms and return
			usleep(4000);
		} else {
			// we have data, break out of while
			break;
		}; // end if

		if ((pglobal->halfduplex) && (oldstate != pglobal->transmit)) {
			changeofstate=1;
			break;
		}; // end if

	}; // end endless loop until valid packet or change of state


	// OK, we've got data in ringbuffer

	// get state from "PTT" subthread
	state=pglobal->transmit;

	if (state) {
		// State = 1: write audio

		// first check old state, if we go from oldstate=0 to state=1, this is
		// the beginning of a new stream; so send start packe
		if (oldstate == 0) {
			// start "start" marker
			// fwrite((void *) &c2_begin,C2ENCAP_SIZE_MARK,1,stdout);
			// fflush(stdout);

			// send start 3 times, just to be sure
			ret=sendto(udpsd,&c2_begin,C2ENCAP_SIZE_MARK,0,sendto_aiaddr, sendto_sizeaiaddr);

			if (ret < 0) {
				fprintf(stderr,"Error in sendto: reason %d (%s)\n",errno, strerror(errno));
			} else {
				sendto(udpsd,&c2_begin,C2ENCAP_SIZE_MARK,0,sendto_aiaddr, sendto_sizeaiaddr);
				sendto(udpsd,&c2_begin,C2ENCAP_SIZE_MARK,0,sendto_aiaddr, sendto_sizeaiaddr);
			}; // end else - if

		}


		// if stereo, only use left channel
		if (pglobal->inputParameters.channelCount == 2) {
			int loop;

			int16_t *p1, *p2;

			// start at 2th element (in stereo format); which is 3th (in mono format)
			p1=&pglobal->c_ringbuffer[new_c_ptr_read][1];
			p2=&pglobal->c_ringbuffer[new_c_ptr_read][2];
        
			for (loop=1; loop < 1920; loop++) {
				*p1=*p2;
				p1++; p2 += 2;
			}; // end for
		}; // end if

		// do samplerate conversion from 48000 (capture) to 8000 (codec2 encoding)
		// store in tempory buffer
		convert_48kto8k(pglobal->c_ringbuffer[new_c_ptr_read],audiobuff8k,0);

		// do codec2 encoding
		codec2_encode(codec2, c2_buff, audiobuff8k);

		// send out
		ret=sendto(udpsd,&c2_voice,C2ENCAP_SIZE_VOICE1400,0,sendto_aiaddr, sendto_sizeaiaddr);
		if (ret < 0) {
			fprintf(stderr,"Error in sendto: reason %d (%s)\n",errno, strerror(errno));
		}; // end if

	} else {
		// state = 0, do not send
		// however, if we go from "oldstate = 1 - > state = 0", this is
		// the end of a stream

		if (oldstate) {
			// send end 3 times, just to be sure
			ret=sendto(udpsd,&c2_end,C2ENCAP_SIZE_MARK,0,sendto_aiaddr, sendto_sizeaiaddr);
			if (ret < 0) {
				fprintf(stderr,"Error in sendto: reason %d (%s)\n",errno, strerror(errno));
			} else {
				sendto(udpsd,&c2_end,C2ENCAP_SIZE_MARK,0,sendto_aiaddr, sendto_sizeaiaddr);
				sendto(udpsd,&c2_end,C2ENCAP_SIZE_MARK,0,sendto_aiaddr, sendto_sizeaiaddr);
			}; // end if

		}; // end if 
	}; // end else - if
	oldstate=state;

	// move up marker
	// only if action was NOT due to "change of state"
	if (!changeofstate) {
		pglobal->c_ptr_read=new_c_ptr_read;
	}; // end if


}; // end endless loop

fprintf(stderr,"Error: process audio thread has dropped out of endless loop. Should not happen! \n");
exit(-1);

}; // end function network sender
