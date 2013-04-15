/* audio in - demodulate - process - network send */

// High level functions to process audio, received from a audio device
// The actual audio is received from the ringbuffer, fed by portaudio callback
// functions
// gmskdemodulation is handled by the c2gmsk API

// version 20130326 initial release
// version 20130404: added half duplex support


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


void * funct_audioin_demod_proc_netwsend (void * globaldatain ) {


// vars
int nextbuffer;
int thisbuffer;

int16_t * audiobuffer;

unsigned char *codec2frame; // 7 octets (40 ms @ 1400 bps = 56 bits), placed in c2_voice.c2data.c2data_data7;

int ret, ret2;

// tempory data
int loop;

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


// C2ENCAP structures
c2encap c2_begin, c2_end, c2_voice;


// C2GMSK API data structures
struct c2gmsk_msgchain * c2gmsk_chain=NULL,  ** c2gmsk_pchain; // pointer to pointer of message chain
struct c2gmsk_msg * c2gmsk_msg;
int tod; // type of data
int datasize;
int c2gmsk_msg_data[4];

char txtline[203]; // text line for "printbit". Should normally by
			// up to 192, add 3 for "space + marker" and terminating null, add
			// 8 for headroom for PLL syncing-errors


////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

///////////////
// application-code starts here

// copy global data
// global data
struct globaldata * pglobal;
pglobal=(struct globaldata *) globaldatain;


// init c2encap structures
memcpy(c2_begin.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2_begin.header[3]=C2ENCAP_MARK_BEGIN;
memcpy(c2_begin.c2data.c2data_text3,"BEG",3);

memcpy(c2_end.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2_end.header[3]=C2ENCAP_MARK_END;
memcpy(c2_end.c2data.c2data_text3,"END",3);

memcpy(c2_voice.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2_voice.header[3]=C2ENCAP_DATA_VOICE1400;


// "codec2frame" points to 7 octet data structure in c2_voice
codec2frame = c2_voice.c2data.c2data_data7;


// init chain
c2gmsk_pchain=&c2gmsk_chain;


// INITIALISE NETWORK
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
ret=getaddrinfo(pglobal->udpout_host, NULL, hint, &info);

// some linux kernels do not support the ai_socktype in getaddressinfo
// when we receive error "ai_socktype" not supported, try again without
// a sockettype
if (ret == EAI_SOCKTYPE) {
	// any type
	hint->ai_socktype = 0;

	// do DNS-query, use getaddrinfo for both ipv4 and ipv6 support
	ret=getaddrinfo(pglobal->udpout_host, NULL, hint, &info);
}; // end if

if (ret != 0) {
	fprintf(stderr,"Error: resolving hostname %s failed: %d (%s)\n",pglobal->udpout_host,ret,gai_strerror(ret));
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
	udp_aiaddr_in->sin_port=htons((unsigned short int) pglobal->udpout_port);

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
	udp_aiaddr_in6->sin6_port=htons((unsigned short int) pglobal->udpout_port);

	// set pointer to be used for "sendto" ipv6 structure
	// sendto uses generic "struct sockaddr" just like the information
	//              returned from getaddrinfo, so no casting needed here
	sendto_aiaddr=info->ai_addr;
	sendto_sizeaiaddr=sizeof(struct sockaddr_in6);
	// get textual version of returned ip-address
	inet_ntop(AF_INET6,&udp_aiaddr_in6->sin6_addr,ipaddress_txt,INET6_ADDRSTRLEN);
        
} else {
	fprintf(stderr,"Error: DNS query for %s returned an unknown network-family: %d \n",pglobal->udpout_host,udp_family);
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


// NETWORK INIT DONE, set flag so that main application can continue
pglobal->networksendready=1;



// endless loop;
while (FOREVER) {

	// is there something to process?

	// check the next buffer slot. If the "capture" process is using it, we do not have any
	// audio available. -> wait and try again
	nextbuffer=pglobal->pntr_r_read + 1;
	if (nextbuffer >= NUMBUFF) {
		nextbuffer=0;
	}; // end if

	if (nextbuffer == pglobal->pntr_r_write) {
		// nothing to process: sleep for 1/4 of audioblock (=10 ms)
		usleep((useconds_t) 10000);

		// set "had to wait" marker
		continue;
	};


	// OK, we have a frame with audio-information, process it, sample
	// per sample
	thisbuffer=nextbuffer;
	audiobuffer= (int16_t *) pglobal->ringbuffer_r[thisbuffer];

	// do c2gmsk demodulation and decoding. Use API
	ret=c2gmsk_demod(pglobal->c2gmsksessid,audiobuffer,c2gmsk_pchain);
	c2gmsk_chain=*c2gmsk_pchain;

	if (ret != C2GMSK_RET_OK) {
		fprintf(stderr,"c2gmsk_mod failed: %d (%s) \n",ret,c2gmsk_printstr_ret(ret));
		exit(-1);
	}; // end if


	// process all messages, we are interested in two types: C2GMSK_MSG_STATECHANGE
	//					and C2GMSK_MSG_CODEC2

	while ((c2gmsk_msg = c2gmsk_msgchain_search(C2GMSK_SEARCH_POSCURRENT, c2gmsk_chain, &tod, &datasize, NULL))) {
		// change of state?
		if (tod == C2GMSK_MSG_STATECHANGE) {
			ret2=c2gmsk_msgdecode_numeric(c2gmsk_msg, c2gmsk_msg_data);

			// we should have two fields: old state and new state
			if (ret2 != 2)	{
				fprintf(stderr,"Error: MSG STATECHANGE should return two parameters. Got %d \n",ret2);
				exit(-1);
			}; // end if

			if (pglobal->verboselevel >= 1) {
				fprintf(stderr,"DEMOD: change of state: was %d now %d \n",c2gmsk_msg_data[0], c2gmsk_msg_data[1]);
			}; // end if

			// start of stream?
			if ((c2gmsk_msg_data[0] < 22) && (c2gmsk_msg_data[1] >= 22)) {


				// send start, 3 times
				for (loop=0; loop < 3; loop++) {
					ret=sendto(udpsd,&c2_begin,C2ENCAP_SIZE_MARK, 0, sendto_aiaddr, sendto_sizeaiaddr);

					if (ret < 0) {
						fprintf(stderr,"Error: audioin_demod_netout, sendto fails: %d (%s)\n",errno, strerror(errno));
						continue;
					};
				}; // end for

				continue;
			}; // end if

			// end of stream?
			if ((c2gmsk_msg_data[0] >= 22) && (c2gmsk_msg_data[1] < 22)) {

				// send start, 3 times
				for (loop=0; loop < 3; loop++) {
					ret=sendto(udpsd,&c2_end,C2ENCAP_SIZE_MARK, 0, sendto_aiaddr, sendto_sizeaiaddr);

					if (ret < 0) {
						fprintf(stderr,"Error: audioin_demod_netout, sendto fails: %d (%s)\n",errno, strerror(errno));
						continue;
					};
				}; // end for

				// done;
				continue;
			}; // end if

			// all other state-changes, just ignore
			continue;
		}; // end if

		// codec2 data?
		if (tod == C2GMSK_MSG_CODEC2) {

			// voice packet. Copy data
			ret2=c2gmsk_msgdecode_c2(c2gmsk_msg,codec2frame);

			if (ret2 != C2GMSK_CODEC2_1400) {
				fprintf(stderr,"Error: audioin_demod_netout, c2gmsk_msgdecode should return version 2 (C2GMSK_CODEC2_1400), got %d\n",ret2);
				continue;
			}; // end if

			if (pglobal->verboselevel >= 3) {
				printf("DEMOD: codec2 voice data: %02X%02X%02X%02X%02X%02X%02X\n",codec2frame[0],codec2frame[1],codec2frame[2],codec2frame[3],codec2frame[4],codec2frame[5],codec2frame[6]);
			}; // end if

			ret=sendto(udpsd,&c2_voice,C2ENCAP_SIZE_VOICE1400, 0, sendto_aiaddr, sendto_sizeaiaddr);

			if (ret < 0) {
				fprintf(stderr,"Error: audioin_demod_netout, sendto fails: %d (%s)\n",errno, strerror(errno));
				continue;
			};

			// done;
			continue;
		}; // end if


		// all other messages are just dumped without further processing

		// PRINTBIT messages
		if (tod == C2GMSK_PRINTBIT_MOD) {
			if (pglobal->dumpstream >= 1) {
				printf("DEMOD: bitdump (demodulated): %s\n",c2gmsk_msgdecode_printbit(c2gmsk_msg,txtline,1));
			}; // end if
			continue;
		}; // end if

		if (tod == C2GMSK_PRINTBIT_ALL) {
			if (pglobal->dumpstream >= 2) {
				printf("DEMOD: bitdump (all): %s\n",c2gmsk_msgdecode_printbit(c2gmsk_msg,txtline,1));
			}; // end if
			continue;
		}; // end if



		// no data
		if (tod == C2GMSK_MSG_NODATA) {
			if (pglobal->verboselevel >= 2) {
				printf("DEMOD: received NODATA message.\n");
			}; // end if
			continue;
		}; // end if

		// FEC stats?
		if (tod == C2GMSK_MSG_FECSTATS) {
			ret2=c2gmsk_msgdecode_numeric(c2gmsk_msg, c2gmsk_msg_data);

			// fecstats is one number:
			if (ret2 != 1)	{
				fprintf(stderr,"Error: MSG STATECHANGE should return one parameter. Got %d \n",ret2);
				exit(-1);
			}; // end if

			if (pglobal->verboselevel >= 2) {
				printf("DEMOD: FECSTATS: %04d\n", c2gmsk_msg_data[0]);
			}; // end if

			continue;
		}; // end if

		// average audio level
		if (tod == C2GMSK_MSG_AUDIOAVGLEVEL) {
			if (pglobal->dumpaverage) {
				ret2=c2gmsk_msgdecode_numeric(c2gmsk_msg, c2gmsk_msg_data);

				// average audio level is one value
				if (ret2 != 1)	{
					fprintf(stderr,"Error: MSG AUDIOAVGLEVEL should return one parameters. Got %d \n",ret2);
					exit(-1);
				}; // end if

				printf("DEMOD: Average audio level: %d\n",c2gmsk_msg_data[0]);
			}; // end if
			continue;
		}; // end if

		// average audio level test: result
		if (tod == C2GMSK_MSG_AUDIOAVGLVLTEST) {
			if (pglobal->verboselevel >= 2) {
				ret2=c2gmsk_msgdecode_numeric(c2gmsk_msg, c2gmsk_msg_data);

				// audio level test-result returns two values
				if (ret2 != 2)	{
					fprintf(stderr,"Error: MSG AUDIOAVGLEVEL should return one parameters. Got %d \n",ret2);
					exit(-1);
				}; // end if

				if (c2gmsk_msg_data[0] == C2GMSK_AUDIOAVGLVLTEST_OK) {
					printf("DEMOD: Audio average-level test returns OK. (max level %d)\n",c2gmsk_msg_data[1]);
				} else if (c2gmsk_msg_data[0] == C2GMSK_AUDIOAVGLVLTEST_TOLOW) {
					printf("DEMOD: Audio average-level test returns TOLOW. (max level %d)\n",c2gmsk_msg_data[1]);
				} else if (c2gmsk_msg_data[0] == C2GMSK_AUDIOAVGLVLTEST_CANCELED) {
					printf("DEMOD: Audio average-level test not done (not enough data)\n");
				} else {
					fprintf(stderr,"Error DEMOD: Audio average-level returns invalid data: %d %d\n",c2gmsk_msg_data[0],c2gmsk_msg_data[1]);
				}; // end if
			}; // end if
			continue;
		}; // end if

		// input audio invert
		if (tod == C2GMSK_MSG_INAUDIOINVERT) {
			if (pglobal->verboselevel >= 2) {
				ret2=c2gmsk_msgdecode_numeric(c2gmsk_msg, c2gmsk_msg_data);

				// input audio invert returns one value
				if (ret2 != 1)	{
					fprintf(stderr,"Error: MSG INAUDIOINVERT should return one parameters. Got %d \n",ret2);
					exit(-1);
				}; // end if

				printf("DEMOD: Input audio-invert flag: %d\n",c2gmsk_msg_data[0]);
			}; // end if
			continue;
		}; // end if

		// version id
		if (tod == C2GMSK_MSG_VERSIONID) {
			if (pglobal->verboselevel >= 2) {
				ret2=c2gmsk_msgdecode_numeric(c2gmsk_msg, c2gmsk_msg_data);

				// input audio invert returns one value
				if (ret2 != 4)	{
					fprintf(stderr,"Error: MSG VERSIONID should return four parameters. Got %d \n",ret2);
					exit(-1);
				}; // end if

				printf("DEMOD: c2gmsk version id: %d (received code: %04X, selected code: %04X, minimal distance: %d\n",c2gmsk_msg_data[0],c2gmsk_msg_data[1],c2gmsk_msg_data[2],c2gmsk_msg_data[3]);
			}; // end if
			continue;
		}; // end if


		// stream end messages
		if (tod == C2GMSK_MSG_UNKNOWNVERSIONID) {
			if (pglobal->verboselevel >= 2) {
				printf("DEMOD: Stream terminated due to UNKNOWN/UNSUPPORTED VERSIONID.\n");
			}; // end if
			continue;
		}; // end if

		if (tod == C2GMSK_MSG_END_NORMAL) {
			if (pglobal->verboselevel >= 2) {
				printf("DEMOD: Stream ended (end-of-stream received).\n");
			}; // end if
			continue;
		}; // end if

		if (tod == C2GMSK_MSG_END_TOMANYMISSEDSYNC) {
			if (pglobal->verboselevel >= 2) {
				printf("DEMOD: Stream terminated due to TO MANY MISSED SYNC.\n");
			}; // end if
			continue;
		}; // end if

		// message we should not receive here

		// is it a known message?
		if (c2gmsk_printstr_msg(tod)) {
			fprintf(stderr,"Error DEMOD: received unexpected message (should not happen): %s\n",c2gmsk_printstr_msg(tod));
			continue;
		}; // end if

		// unknown message
		fprintf(stderr,"Error DEMOD: received unknown message (should not happen): %d\n",tod);
		continue;
	}; // end while (message loop)

	pglobal->pntr_r_read = nextbuffer;

}; // end while (endless loop)


fprintf(stderr,"Error: AUDIOIN_DEMOD_PROC_NETSEND dropped out of main loop: should not happen! \n");

/// end program
exit(0);
}; // end thread audioin - demodulate - process - network send


