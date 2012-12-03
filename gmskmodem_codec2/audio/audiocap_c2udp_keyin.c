/* audio capture */

// Reads audio from a portaudio audio device
// If the <enter> key is pushed, the audio is codec2-encoded and send as a
// UDP stream to the gmskmodem


//////////////////// CONFIGURATION: CHANGE BELOW 
// set portaudio device
// If set to -1, use "default device" value returned by portaudio
// Pa_GetDefaultInputDevice() function
#define PORTAUDIODEVICE -1

#define IPV4ONLY 0
#define IPV6ONLY 0
//////////////////// END
//////////////////// DO NOT CHANGE ANYTHING BELOW UNLESS YOU KNOW WHAT
//////////////////// YOU ARE DOING



#define RATE 8000

// portaudio
#include <portaudio.h>

// posix threads and inter-process control
#include <pthread.h>
#include <signal.h>

// termio to disable echo on local console
#include <termios.h>

// codec2 libraries
#include <codec2.h>

// for memcpy
#include <string.h>

// for int16_t and others
#include <stdint.h>

// for malloc, exit
#include <stdlib.h> 

// for (f)printf
#include <stdio.h>

// for errno
#include <errno.h>

// for networking, getaddrinfo and related
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>


// global data structure//
typedef struct {
	int transmit; // are we transmitting ???
	int pa_devicenr; // only used to display information
} globaldatastr;

// global data itself //
globaldatastr global;

// defines and structures for codec2-encapsulation
#include "c2encap.h"

// functions defined below
void * funct_keypress (void *) ;


int main (int argc, char ** argv) {
int buffersize;
int stereo;
int numBytes;

// state of keypress
int state, oldstate;

int portaudiodevice;
PaStreamParameters inputParameters;
PaStream * stream;
PaError pa_ret;

const PaDeviceInfo *devinfo;

// vars for networking
char * ipaddrtxtin;
int udpport;
struct sockaddr_in * udp_aiaddr_in = NULL;
struct sockaddr_in6 * udp_aiaddr_in6 = NULL;
struct sockaddr * sendto_aiaddr = NULL; // structure used for sendto
int sendto_sizeaiaddr=0; // size of structed used for sendto
int udpsd;
int udp_family;

char ipaddrtxt[INET6_ADDRSTRLEN];

// vars for getaddrinfo
struct addrinfo * hint;
struct addrinfo * info;

// vars for codec2
void *codec2;
unsigned char *c2_buff;
int mode, nsam, nbit, nbyte;
int16_t * audiobuffer;

// other vars
int ret;

// structure for c2encap data
c2encap c2_voice;
c2encap c2_begin, c2_end;

// "audio in" posix thread
pthread_t thr_keypress;

// init data
stereo=-1;
global.transmit=0;

// We need at least 2 arguments: IP-address and udpport
if (argc < 3) {
	fprintf(stderr,"Error: at least 2 arguments needed. \n");
	fprintf(stderr,"Usage: %s <ip-addr> <udp port> [ <audiodevice> ] \n",argv[0]);
	exit(-1);
}; // end if

ipaddrtxtin=argv[1];
udpport=atoi(argv[2]);


// if 1st argument exists, use it as capture device
if (argc >= 4) {
	portaudiodevice = atoi(argv[3]);
} else {
	// no argument given; use default
	portaudiodevice = PORTAUDIODEVICE;
}; //


// create network structure
if ((udpport < 0) || (udpport > 65535)) {
	fprintf(stderr,"Error: UDPport number must be between 0 and 65535! \n");
	exit(-1);
}; // end if


if ((IPV4ONLY) && (IPV6ONLY)) {
	fprintf(stderr,"Error: internal configuration error: ipv4only and ipv6only are mutually exclusive! \n");
	exit(-1);
}; // end if


// DO DNS query for ipaddress
hint=malloc(sizeof(struct addrinfo));

if (!hint) {
	fprintf(stderr,"Error: could not allocate memory for hint!\n");
	exit(-1);
}; // end if

// clear hint
memset(hint,0,sizeof(hint));

hint->ai_socktype = SOCK_DGRAM;

// resolve hostname, use function "getaddrinfo"
// set address family of hint if ipv4only or ipv6only
if (IPV4ONLY) {
	hint->ai_family = AF_INET;
} else if (IPV6ONLY) {
	hint->ai_family = AF_INET6;
} else {
	hint->ai_family = AF_UNSPEC;
}; // end else - elsif - if

// do DNS-query, use getaddrinfo for both ipv4 and ipv6 support
ret=getaddrinfo(ipaddrtxtin, NULL, hint, &info);

if (ret < 0) {
	fprintf(stderr,"Error: resolving hostname %s failed: (%s)\n",ipaddrtxtin,gai_strerror(ret));
	exit(-1);
}; // end if


udp_family=info->ai_family;

// open UDP socket + set udp port
if (udp_family == AF_INET) {
	udpsd=socket(AF_INET,SOCK_DGRAM,0);
	
	// getaddrinfo returns pointer to generic "struct sockaddr" structure.
	// 		Cast to "struct sockaddr_in" to be able to fill in destination port
	udp_aiaddr_in=(struct sockaddr_in *)info->ai_addr;
	udp_aiaddr_in->sin_port=htons((unsigned short int) udpport);

	// set pointer to be used for "sendto" ipv4 structure
	// sendto uses generic "struct sockaddr" just like the information
	// 		returned from getaddrinfo, so no casting needed here
	sendto_aiaddr=info->ai_addr;
	sendto_sizeaiaddr=sizeof(struct sockaddr);

	// get textual version of returned ip-address
	inet_ntop(AF_INET,&udp_aiaddr_in->sin_addr,ipaddrtxt,INET6_ADDRSTRLEN);
	
} else if (udp_family == AF_INET6) {
	udpsd=socket(AF_INET6,SOCK_DGRAM,0);

	// getaddrinfo returns pointer to generic "struct sockaddr" structure.
	// 		Cast to "struct sockaddr_in6" to be able to fill in destination port
	udp_aiaddr_in6=(struct sockaddr_in6 *)info->ai_addr;
	udp_aiaddr_in6->sin6_port=htons((unsigned short int) udpport);

	// set pointer to be used for "sendto" ipv4 structure
	// sendto uses generic "struct sockaddr" just like the information
	// 		returned from getaddrinfo, so no casting needed here
	sendto_aiaddr=info->ai_addr;
	sendto_sizeaiaddr=sizeof(struct sockaddr_in6);

	// get textual version of returned ip-address
	inet_ntop(AF_INET,&udp_aiaddr_in6->sin6_addr,ipaddrtxt,INET6_ADDRSTRLEN);
	
} else {
	fprintf(stderr,"Error: DNS query for %s returned an unknown network-family: %d \n",ipaddrtxtin,udp_family);
	exit(-1);
}; // end if



// getaddrinfo can return multiple results, we only use the first one
// give warning is more then one result found.
// Data is returned in info as a linked list
// If the "next" pointer is not NULL, there is more then one
// element in the chain

if (info->ai_next != NULL) {
	fprintf(stderr,"Warning. getaddrinfo returned multiple entries. Using %s\n",ipaddrtxt);
} else {
	fprintf(stderr,"Sending CODEC2 DV stream to ip-address %s\n",ipaddrtxt);
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





fprintf(stderr,"INITIALISING PORTAUDIO ... \n");
// open portaudio device
pa_ret=Pa_Initialize();
fprintf(stderr,".... DONE\n");

if (pa_ret != paNoError) {
	Pa_Terminate();
	fprintf(stderr,"Error: Could not initialise Portaudio: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
	exit(-1);
}; // end if

if (portaudiodevice == -1) {
	// portaudio device = -1 -> use portaudio "get default input device"
	inputParameters.device = Pa_GetDefaultInputDevice();

	if (inputParameters.device == paNoDevice) {
		fprintf(stderr,"Error: no portaudio default input device!\n");
		exit(-1);
	}; // end if
} else {
	inputParameters.device = portaudiodevice;
}; // end else - if

global.pa_devicenr=inputParameters.device;

devinfo = Pa_GetDeviceInfo (inputParameters.device);

fprintf(stderr,"Audio device = %d (%s %s)\n",global.pa_devicenr,Pa_GetHostApiInfo(devinfo->hostApi)->name,devinfo->name);

inputParameters.sampleFormat = paInt16;
inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
inputParameters.hostApiSpecificStreamInfo = NULL;


// first try mono
inputParameters.channelCount = 1;

pa_ret = Pa_IsFormatSupported(&inputParameters,NULL, (double) RATE);
if (pa_ret == paFormatIsSupported) {
	stereo=0;

	fprintf(stderr,"8 KHz sampling rate support in mono! \n");
} else {
	// try again in stereo
	inputParameters.channelCount = 2;

	pa_ret = Pa_IsFormatSupported(&inputParameters, NULL, (double) RATE);
	if (pa_ret == paFormatIsSupported) {
		stereo=1;
		fprintf(stderr,"8 KHz sampling rate support in stereo! \n");
	} else {
		fprintf(stderr,"Error: 8 Khz sampling not supported in mono or stereo! Exiting. \n");
		exit(-1);
	}; // end if
}; // end 


// configure portaudio global data
if (stereo) {
	numBytes = 1280;
} else {
	numBytes = 640; 
}; // end if

// create memory for audiobuffer
audiobuffer = malloc(numBytes); // allow memory for buffer 0
if (!audiobuffer) {
	// memory could not be allocated
	fprintf(stderr,"Error: could not allocate memory for portaudio buffer 0!\n");
	exit(-1);
}; // end if


// open PortAudio stream
// do not start stream yet, will be done further down
pa_ret = Pa_OpenStream (
	&stream,
	&inputParameters,
	NULL, // output Parameters, not used here 
	RATE, // sample rate
	320, // frames per buffer: 40 ms @ 8000 samples/sec
	paClipOff, // we won't output out of range samples,
					// so don't bother clipping them
	NULL, // no callback function, syncronous read
	&global // parameters passed to callback function (not used here)
);

if (pa_ret != paNoError) {
	Pa_Terminate();
	fprintf(stderr,"Error in Pa_OpenStream: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
	exit(-1);
}; // end if

// init codec2
mode = CODEC2_MODE_1400;
codec2 = codec2_create (mode);
nsam = codec2_samples_per_frame(codec2);
nbit = codec2_bits_per_frame(codec2);

nbyte = (nbit + 7) >> 3; // ">>3" is same as "/8"

if (nbyte != 7) {
	fprintf(stderr,"Error: number of bytes for codec2 frames should be 7. We got %d \n",nbyte);
}; // end if

if (nsam != 320) {
	fprintf(stderr,"Error: number of samples for codec2 frames should be 320. We got %d \n",nsam);
}; // end if

c2_buff = (unsigned char *)&c2_voice.c2data.c2data_data7;


// allocate audiobuffer
if (stereo) {
	buffersize=nsam<<2; // = * 4 (stereo and 16 bit/sample)
} else {
	// mono
	buffersize=nsam<<1; // = * 2 (16 bit/sample)
}; // end else - if
audiobuffer=malloc(buffersize);

if (!audiobuffer) {
	fprintf(stderr,"Error: malloc audiobuffer: %s",strerror(errno));
	exit(-1);
}; // end if


// start thread to read detect keypress (used to switch transmitting)
pthread_create (&thr_keypress, NULL, funct_keypress, (void *) &global);


// Start stream
pa_ret=Pa_StartStream(stream);

if (pa_ret != paNoError) {
	Pa_Terminate();
	fprintf(stderr,"Error in Pa_StartStream: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
	exit(-1);
}; // end if


// init some vars;
oldstate=0;

while (( pa_ret = Pa_IsStreamActive (stream)) == 1) {
// get audio

	pa_ret = Pa_ReadStream(stream, audiobuffer, 320);

	if (pa_ret != paNoError) {
		Pa_Terminate();
		fprintf(stderr,"Error in Pa_ReadStream: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
		exit(-1);
	}; // end if

	// get state from subthread
	state=global.transmit;

	if (state) {
		// State = 1: write audio

		// first check old state, if we go from oldstate=0 to state=1, this is
		// the beginning of a new stream; so send start packe
		if (oldstate == 0) {
			// start "start" marker
			// fwrite((void *) &c2_begin,C2ENCAP_SIZE_MARK,1,stdout);
			// fflush(stdout);

			// send start 3 times, just to be sure
			sendto(udpsd,&c2_begin,C2ENCAP_SIZE_MARK,0,sendto_aiaddr, sendto_sizeaiaddr);
			sendto(udpsd,&c2_begin,C2ENCAP_SIZE_MARK,0,sendto_aiaddr, sendto_sizeaiaddr);
			sendto(udpsd,&c2_begin,C2ENCAP_SIZE_MARK,0,sendto_aiaddr, sendto_sizeaiaddr);

//			putc('B',stderr);
		}

		// if stereo, only use left channel
		if (stereo) {
			int loop;

			int16_t *p1, *p2;

			// start at 2th element (in stereo format); which is 3th (in mono format)
			p1=&audiobuffer[1];
			p2=&audiobuffer[2];
	
			for (loop=1; loop < nsam; loop++) {
				*p1=*p2;
				p1++; p2 += 2;
			}; // end for
		}; // end if

		// do codec2 encoding
		codec2_encode(codec2, c2_buff, audiobuffer);

		//fwrite((void *)&c2_voice,C2ENCAP_SIZE_VOICE1400,1,stdout);
		//fflush(stdout);

		sendto(udpsd,&c2_voice,C2ENCAP_SIZE_VOICE1400,0,sendto_aiaddr, sendto_sizeaiaddr);

//		putc('T',stderr);
	} else {
		// state = 0, do not send
		// however, if we go from "oldstate = 1 - > state = 0", this is
		// the end of a stream

		if (oldstate) {
			// send "end" marker
			//fwrite((void *)&c2_end,C2ENCAP_SIZE_MARK,1,stdout);
			//fflush(stdout);

			// send end 3 times, just to be sure
			sendto(udpsd,&c2_end,C2ENCAP_SIZE_MARK,0,sendto_aiaddr, sendto_sizeaiaddr);
			sendto(udpsd,&c2_end,C2ENCAP_SIZE_MARK,0,sendto_aiaddr, sendto_sizeaiaddr);
			sendto(udpsd,&c2_end,C2ENCAP_SIZE_MARK,0,sendto_aiaddr, sendto_sizeaiaddr);

//			putc('E',stderr);
		}; // end if 
	}; // end else - if

	oldstate=state;

	
}; // end while
// dropped out of endless loop. Should not happen

if (pa_ret < 0) {
	Pa_Terminate();
	fprintf(stderr,"Error in Pa_isStreamActive: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
	exit(-1);
}; // end if

fprintf(stderr,"Error: audiocap dropped out of audiocapturing loop. Should not happen!\n");

pa_ret=Pa_CloseStream(stream);

if (pa_ret != paNoError) {
	Pa_Terminate();
	fprintf(stderr,"Error in Pa_CloseStream: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
	exit(-1);
}; // end if

// Done!!!

Pa_Terminate();

exit(0);


}; // end main applicion

// FUNCTIONS

// function "keypress"
// detects <enter> to switch "on" or "off" status
// started as a subthread
void * funct_keypress (void * globalin) {
globaldatastr * p_global;
p_global=(globaldatastr *) globalin;

// local vars
struct termios tio;
int stop;


// toggle "transmit" flag based on enter begin pushed

// first disable echo on local console
// get termiod struct for stdin
tcgetattr(0,&tio);
// disable echo
tio.c_lflag &= (~ECHO);
// write change to console
tcsetattr(0,TCSANOW,&tio);


fprintf(stderr,"AUDIO CAPTURE, ENCODE AND OUTPUT\n");
fprintf(stderr,"audio device = %d\n",p_global->pa_devicenr);
fprintf(stderr,"press <enter> to switch on or off\n");
fprintf(stderr,"press \"x\" to stop\n");
fprintf(stderr,"\n");

fprintf(stderr,"audio is OFF! \n");

stop=0;
while (!stop) {
	int c;

	c=getchar();

	// is it lf?
	if (c == 0x0a) {
		if (p_global->transmit) {
			// ON to OFF

			fprintf(stderr,"audio is OFF! \n");
			p_global->transmit=0;

		} else {
			// OFF to ON
			fprintf(stderr,"audio is ON! \n");
			p_global->transmit=1;
		}; // end if

	} else if ((c == 'x') || (c == 'X')) {
		fprintf(stderr,"STOP! \n");
		stop=1;
	}; // end else - if

}; // end while

// reenable ECHO

// reenable echo on local console
// get termiod struct for stdin
tcgetattr(0,&tio);
// disable echo
tio.c_lflag |= (ECHO);
// write change to console
tcsetattr(0,TCSANOW,&tio);

// exit application
global.transmit=-1;
exit(-1);

}; // end function funct_audioin
