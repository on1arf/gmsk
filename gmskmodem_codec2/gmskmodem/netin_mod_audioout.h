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



// RECEIVE FROM NET, MODULATE via API and QUEUE to AUDIO-BUFFER

// version 20130326: initial release
// version 20130404: added half duplex support


void * funct_netin_modulate_audioout (void * globalin) {
struct globaldata * pglobal;
pglobal=(struct globaldata *) globalin;

// local vars
// network related
unsigned char *udpbuffer;
int udpsize;
int udpsock;
struct sockaddr_in localsa4;
struct sockaddr_in6 localsa6;

struct sockaddr * receivefromsa;
socklen_t receivefromsa_size=0;

// vars for c2gmsk API
struct c2gmsk_msgchain * c2gmsk_chain=NULL, ** c2gmsk_pchain;
struct c2gmsk_msg * c2gmsk_msg;


// C2ENCAP types
uint8_t *c2encap_type, *c2encap_begindata;
int tod;
int numsamples;

// audio
int16_t audio48kbuffer[480]; // 10 ms of audio @ 48ksamples/sec

// other vars
int ret;
int state;
int idfilecount;


// init some vars
idfilecount=0;

// init chain
c2gmsk_pchain=&c2gmsk_chain;

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
		fprintf(stderr,"Error creating socket in s_netin_mod_queue: %d (%s) \n",errno,strerror(errno));
		exit(-1);
	}; // end if

	localsa4.sin_family=AF_INET;
	localsa4.sin_port=htons(pglobal->udpin_port);
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
		fprintf(stderr,"Error creating socket in s_netin_mod_queue: %d (%s) \n",errno,strerror(errno));
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
	localsa6.sin6_port=htons(pglobal->udpin_port);
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

// sanity check. Just to be sure, check "channelCount" setting, should be "0" or "1"
if ((pglobal->inputParameters.channelCount != 1) && (pglobal->inputParameters.channelCount != 2)) {
	fprintf(stderr,"Internal error: stereo flag for playback in funct_net_receive not set correctly by main thread! Should not happen! Exiting. \n");
	exit(-1);
}; // end if


// init state
state=0; // state 0 = wait for start
pglobal->receivefromnet=0;


if (pglobal->verboselevel >= 1) {
	printf("MODULATE: listening.\n");
};

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


	///////////////////////////////////////
	// error check frame

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

	///////////////////////////////////////
	// process frame

	// processing from here on depends on state
	if (state == 0) {
		// state 0, waiting for start data

#if C2ENCAP_STRICT == 0
		int gotostate1=0;

		if (*c2encap_type == C2ENCAP_DATA_VOICE1400) {
			// we should have received a "start", but go to state 1 anyway
			// do NOT do a "continue" as we do want the packet to be
			// processed below

			
			if (pglobal->verboselevel >= 2) {
				printf("MODULATE: receiving voice without start. Going to start anyway!.\n");
			} else if (pglobal->verboselevel >= 1) {
				printf("MODULATE: receiving.\n");
			}; // end else - if

			gotostate1=1;
		}; // end if

		if ((*c2encap_type == C2ENCAP_MARK_BEGIN) || (gotostate1))
#else 
		if (*c2encap_type == C2ENCAP_MARK_BEGIN)
#endif
		{
			// received start

			// queue silence
			bufferfill_silence(pglobal->silencebegin,pglobal);

			// set "receivefromnet" marker, so that the audio callback
			// function can switch to "netreceive" mode
			pglobal->receivefromnet=1;



			// API call to initiate C2GMSK stream, return one or more audio blocks
			ret=c2gmsk_mod_start(pglobal->c2gmsksessid,c2gmsk_pchain);

			if (ret != C2GMSK_RET_OK) {
				fprintf(stderr,"c2gmsk mod start returned an error: %d \n",ret);
			}; // end if

			// check all messages, we are only interested in PCM48K messages
			tod=C2GMSK_MSG_PCM48K;

			while ((c2gmsk_msg = c2gmsk_msgchain_search_tod(C2GMSK_SEARCH_POSCURRENT, c2gmsk_chain, tod, &numsamples, NULL))) {
				bufferfill_audio_pcm48kmsg_p(c2gmsk_msg,pglobal);
			}; 

			// go to state 1
			state=1;
			if (pglobal->verboselevel >= 1) {
				printf("MODULATE: receiving.\n");
			}; // end else - if

#if C2ENCAP_STRICT == 0
			if (!gotostate1) {
				continue;
			}; // end if
#else
			continue;
#endif


#if C2ENCAP_STRICT == 0
		// less strict protocol checking
		} else if (*c2encap_type == C2ENCAP_MARK_END) {
			// do nothing, silently ignore and go to next packet
			continue;
#endif
		} else {
			fprintf(stderr,"Warning: received packet of type 0X%02X in state 0. Ignoring packet! \n",*c2encap_type);
			continue;
		}; // end if


	} else if (state == 1) {
		// state 1: receiving voice data, until we receive a "end" marker

		// set receive_from_net flag (not strickly necessairy, just
		//			to be sure)
		pglobal->receivefromnet=1;

		//////////////////////////
		// voice packet
		// some error-checking
		if (*c2encap_type == C2ENCAP_DATA_VOICE1400) {
			// API call to modulate 56 bits, get back one or more audio-blocks
			ret=c2gmsk_mod_voice1400(pglobal->c2gmsksessid,(unsigned char *)c2encap_begindata,c2gmsk_pchain);

			if (ret != C2GMSK_RET_OK) {
				fprintf(stderr,"c2gmsk mod voice1400 returned an error: %d \n",ret);
			}; // end if

			// check all messages, we are only interested in PCM48K messages
			tod=C2GMSK_MSG_PCM48K;
			while ((c2gmsk_msg = c2gmsk_msgchain_search_tod(C2GMSK_SEARCH_POSCURRENT, c2gmsk_chain, tod, &numsamples, NULL))) {
				bufferfill_audio_pcm48kmsg_p(c2gmsk_msg,pglobal);
			}; 

			// goto next packet
			continue;
		}; // end if


		//////////////////////////////
		// end of stream packet
		if (*c2encap_type == C2ENCAP_MARK_END) {
			// API call to end c2gmsk stream, get back one or more audio-blocks

			ret=c2gmsk_mod_voice1400_end(pglobal->c2gmsksessid,c2gmsk_pchain);

			if (ret != C2GMSK_RET_OK) {
				fprintf(stderr,"c2gmsk mod voice1400/end returned an error: %d \n",ret);
			}; // end if

			// check all messages, we are only interested in PCM48K messages
			tod=C2GMSK_MSG_PCM48K;
			while ((c2gmsk_msg = c2gmsk_msgchain_search_tod(C2GMSK_SEARCH_POSCURRENT, c2gmsk_chain, tod, &numsamples, NULL))) {
				bufferfill_audio_pcm48kmsg_p(c2gmsk_msg,pglobal);
			}; 

			if (pglobal->verboselevel >= 1) {
				printf("MODULATE: Transmission end.\n");
			}; // end else - if

			// is there an id-file to play?
			if (pglobal->idfile) {
				// only play out idfile every "idfreq" times
				if (idfilecount == 0) {
					if (pglobal->verboselevel >= 1) {
						printf("MODULATE: Transmitting idfile.\n");
					}; // end else - if

					int filesize=0;
					ssize_t sizeread;
					int idinfile;

					unsigned char filebuffer[960]; // 10 ms @ 48 Ksamples / sec @ 16 bit/sample

					// open file
					idinfile=open(pglobal->idfile,O_RDONLY);

					if (idinfile) {
						// open successfull

						filesize = 0;

						// maximum 3 seconds (300 times 10 ms)
						while (filesize < 300) {
						// read 10 ms (8000 Khz, 16 bit size, mono) = 160 octets
							sizeread=read(idinfile,filebuffer,160);

							if (sizeread < 160) {
							// stop if no audio anymore
								// close file
								close(idinfile);
								break;
							}; // end if

							// samplerate convertion from 8 Khz to 48 Khz
							convert_8kto48k((int16_t *)filebuffer,audio48kbuffer,0);

							// write 480 samples (10 ms @ 48 Ksamples/sec) 
							bufferfill_audio(audio48kbuffer,480,pglobal);
						}; // end while
					}; // end if
				}; // end if (idfile to play)


				idfilecount++;
				if (idfilecount >= pglobal->idfreq) {
					idfilecount=0;
				}; // end if
			}; // end if (idfile configured)

			// done  just play end-silence
			// queue silence
			bufferfill_silence(pglobal->silenceend,pglobal);
	
			// clear "receive from net" flag to signal change of
			// state to "portaudio callback" function
			pglobal->receivefromnet=0;

			if (pglobal->verboselevel >= 1) {
				printf("MODULATE: Listening\n");
			}; // end else - if

			state=0;

			// done
			continue;
		};  // end if - END_STREAM


#if C2ENCAP_STRICT == 0
		// silently ignore any additional "start" messages
		if (*c2encap_type == C2ENCAP_MARK_BEGIN) {
			// do nothing, ignore and go to next packet
			continue;
		}; // end if
#endif
		// some other packet
		// give error
		fprintf(stderr,"Warning: received packet of type 0X%02X in state 1. Ignoring packet! \n",*c2encap_type);
		continue;


	} else {
		fprintf(stderr,"Internal Error: unknow state %d in s_netin_queue main loop. Should not happen. Exiting!!!\n",state);
		exit(-1);
	}; // end if

}; // end while (endless loop)

fprintf(stderr,"Internal Error: audiotool funct_network_receive drops out of endless loop. Should not happen! \n");
exit(-1);

}; // end function funct_netin_modulate_audioout

