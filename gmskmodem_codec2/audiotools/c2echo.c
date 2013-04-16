// C2ECHO: receives C2ENC (codec2 message in UDP), buffers and sends back
// designed as tool to correctly configure the "audiotool" audio levels


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


// version 20130414: initial release


#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

// c2encap structures
#include <c2encap.h>

// errno
#include <errno.h>

#define C2DATA_BUFFERSIZE 31500 // 180 sec @ 1400 bps
#define FOREVER 1



int main(int argc, char *argv[]) {

int sock=-1;
struct sockaddr_in6 localsa6;
struct sockaddr_in localsa4;
struct sockaddr * localsa;
socklen_t localsa_size;


struct sockaddr * receivedsa=NULL; // sockaddr of received UDP packet
socklen_t receivedsa_size;


struct sockaddr_in6 *remotesa6;
struct sockaddr_in *remotesa4;
struct sockaddr * remotesa=NULL; // sockaddr of remote side (as received by 1st frame)
socklen_t remotesa_size=0;


char buffer[1500]; // ethernet buffer size
int udpsize;
int receiveport,sendport;
int ret;
int state;

int ipv4_only=0;
int ipv6_only=0;
int ipversion=0;

// c2encap structures
c2encap c2_voice, c2_begin, c2_end;
uint8_t * c2encap_type;
unsigned char * c2encap_begindata;

unsigned char c2data_buffer[C2DATA_BUFFERSIZE];
unsigned char *p_c2data=NULL;
int framesreceived=0;


if (argc != 3) {
	fprintf(stderr, "USAGE: %s <receive-port> <sendport>\n", argv[0]);
	exit(1);
}

receiveport=atoi(argv[1]);
sendport=atoi(argv[2]); 

/////////////// main application

// init vars

// init c2encap structures
memcpy(c2_begin.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2_begin.header[3]=C2ENCAP_MARK_BEGIN;
memcpy(c2_begin.c2data.c2data_text3,"BEG",3);

memcpy(c2_end.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2_end.header[3]=C2ENCAP_MARK_END;
memcpy(c2_end.c2data.c2data_text3,"END",3);

memcpy(c2_voice.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2_voice.header[3]=C2ENCAP_DATA_VOICE1400;



// open network socket

// first try to create ipv6 socket. If we return a "protocol not supported" error, we set the
// "ipv4-only" flag and try again

if (!ipv4_only) {
	sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

	if (sock < 0) {
		sock=-1;

		// check errno
		if (errno == EAFNOSUPPORT) {
			if (ipv6_only) {
				fprintf(stderr,"Error creating socket: %d (%s) \n",errno,strerror(errno));
				exit(-1);
			} else {
				ipv4_only=1;
			}; // end else - if
		} else {
			/* Create the UDP socket */
			fprintf(stderr,"Error creating socket: %d (%s) \n",errno,strerror(errno));
			exit(-1);
		}; // end else - if
	} else {
		// success!

		// if ipv6 only, set option
		if (ipv6_only) {
			int yes=1;

			// make socket ipv6-only.
			ret=setsockopt(sock,IPPROTO_IPV6, IPV6_V6ONLY, (char *)&yes,sizeof(int));
			if (ret == -1) {
				fprintf(stderr,"Error: IPV6ONLY option set but could not make socket ipv6 only: %d (%s)! \n",errno,strerror(errno));
				exit(-1);
			}; // end if
		}; // end if

		localsa6.sin6_family=AF_INET6;
		localsa6.sin6_port=ntohs(receiveport);
		localsa6.sin6_flowinfo=0; // flows not used
		localsa6.sin6_scope_id=0; // we listen on all interfaces
		memset(&localsa6.sin6_addr,0,sizeof(struct in6_addr)); // address = "::" (all 0) -> we listen on all interfaces

		localsa=(struct sockaddr *)&localsa6;
		localsa_size=sizeof(struct sockaddr_in6);

		ipversion=6;

		ret=bind(sock, localsa, localsa_size);
	}; // end if

}; // end if


// if socket not ok in ipv6, try ipv4
if (sock < 0) {
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (sock < 0) {
		fprintf(stderr,"Error creating socket: %d (%s) \n",errno,strerror(errno));
		exit(-1);
	}; // end if

	localsa4.sin_family=AF_INET;
	localsa4.sin_port=ntohs(receiveport);
	memset(&localsa4.sin_addr,0,sizeof(struct in_addr)); // address = "0.0.0.0" ) -> we listen on all interfaces

	localsa=(struct sockaddr *) &localsa4;
	localsa_size=sizeof(struct sockaddr_in);

	ipversion=4;

	ret=bind(sock, localsa, localsa_size); 
}; // end if

// check result of "bind"
if (ret < 0) {
	fprintf(stderr,"Error: could not bind network-address to socket: %d (%s) \n",errno,strerror(errno));
	exit(-1);
}; // end if


// allocate memory for received socket address and remote socket address
// allocate size of ipv6 socket address structure). Is also large enough to
// hold ipv4 socket address
receivedsa = malloc(sizeof(struct sockaddr_in6));
if (!receivedsa) {
	fprintf(stderr,"Error: could not allocate memory for receivedsa! \n");
}; // end if
receivedsa_size=sizeof(struct sockaddr_in6);

remotesa = malloc(sizeof(struct sockaddr_in6));
if (!remotesa) {
	fprintf(stderr,"Error: could not allocate memory for remotesa! \n");
}; // end if


// init state
state=0;
fprintf(stderr,"LISTENING\n");

/* Run until cancelled */
while (FOREVER) {

	/* Receive a message from the client */
	// repeate until read or error, but ignore "EINTR" errors
	while (FOREVER) {
		udpsize = recvfrom(sock, buffer, 1500, 0, receivedsa, &receivedsa_size);

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


	// check remote socket address
	if (receivedsa_size > sizeof(struct sockaddr_in6)) {
		// something went wrong. Return socket address structure
		// is larger then one for ipv6. Should not happen
		fprintf(stderr,"Error: recvfrom returns to large socket address structure. Should not happen! \n");
		exit(-1);
	}; // end if

	// set pointers
	c2encap_type = (uint8_t*) &buffer[3];
	c2encap_begindata = (uint8_t*) &buffer[4]; 
	

	
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
			fprintf(stderr,"Error: RECEIVED C2ENCAP BEGIN MARKER WITH INCORRECT TEXT: 0X%02X 0X%02X 0X%02X. Ignoring frame!\n",buffer[4],buffer[5],buffer[6]);
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
			fprintf(stderr,"Error: RECEIVED C2ENCAP BEGIN MARKER WITH INCORRECT TEXT: 0X%02X 0X%02X 0X%02X. Ignoring frame!\n",buffer[4],buffer[5],buffer[6]);
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



	// state machine
	if (state == 0) {
		// state 0: wait for start

		if (*c2encap_type == C2ENCAP_MARK_BEGIN) {
			// received start. Init vars
			state=1;
			framesreceived=0;
			p_c2data=c2data_buffer;

			// copy sockaddress
			memcpy(remotesa,receivedsa,receivedsa_size);
			remotesa_size=receivedsa_size;

			fprintf(stderr,"RECEIVING\n");

			continue;
		}; 

		continue;
	}; // end state 0

	if (state == 1) {

		// compair sockaddr of currently received packet with that of first of stream
		if (remotesa_size != receivedsa_size) {
			continue;
		}; // end if

		if (memcmp(remotesa,receivedsa,remotesa_size) != 0) {
			continue;
		}; // end if


		if (*c2encap_type == C2ENCAP_DATA_VOICE1400) {
			if (framesreceived < 4500) {
				framesreceived++;
				memcpy(p_c2data,c2encap_begindata,7);
				p_c2data += 7;
			}; // end if

			continue;
		}; // end if

		if (*c2encap_type == C2ENCAP_MARK_END) {
			// end received. Go to state 2
			state=2;
			continue;
		}; 

		continue;
	}; // end state 1

	if (state == 2) {
		int loop;

		// state 2, echo data back to sender

		fprintf(stderr,"PAUSE\n");

		// first wait 1 second
		sleep(1);

		fprintf(stderr,"SENDING\n");

		// before sending back the packet, change destination udp
		if (remotesa->sa_family == AF_INET6) {
			remotesa6=(struct sockaddr_in6 *) remotesa;
			remotesa6->sin6_port = ntohs(sendport);
		} else if (remotesa->sa_family == AF_INET) {
			remotesa4=(struct sockaddr_in *) remotesa;
			remotesa4->sin_port = ntohs(sendport);
		} else {
			fprintf(stderr,"Internal error: incorrect value for ipversion %d!\n",ipversion);
			exit(-1);
		}; // end if


		// init vars
		p_c2data=c2data_buffer;

		// send START packet
		ret=sendto(sock, &c2_begin, C2ENCAP_SIZE_MARK, 0, (struct sockaddr *) remotesa, remotesa_size);
		if (ret < 0) {
			fprintf(stderr,"Error in sendto: reason %d (%s)\n",errno, strerror(errno));
		}; // end if

		// wait 40 ms
		usleep(40000);

		// send VOICE packets
		for (loop=0; loop < framesreceived; loop++) {
			// copy data from buffer
			memcpy(&c2_voice.c2data.c2data_data7,p_c2data,7);

			ret=sendto(sock, &c2_voice, C2ENCAP_SIZE_VOICE1400, 0, (struct sockaddr *) remotesa, remotesa_size);
			if (ret < 0) {
				fprintf(stderr,"Error in sendto: reason %d (%s)\n",errno, strerror(errno));
			}; // end if
			p_c2data += 7;

			// wait 40 ms
			usleep(40000);
		}; // end for

		// send END packet
		ret=sendto(sock, &c2_end, C2ENCAP_SIZE_MARK, 0, (struct sockaddr *) remotesa, remotesa_size);
		if (ret < 0) {
			fprintf(stderr,"Error in sendto: reason %d (%s)\n",errno, strerror(errno));
		}; // end if


		// go back to state 0
		// reinit vars
		state=0;
		fprintf(stderr,"LISTENING\n");

		sendport=atoi(argv[2]); 

		continue;

	}; // end state 2

	fprintf(stderr,"Error: unknown state: %d\n",state);
	exit(-1);
	
}; // end endless loop

fprintf(stderr,"Internal error: c2echo dropped out of endless loop. Should not happen!\n");

exit(0);

}; // end main application
          

