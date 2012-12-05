/* audio playback */

// received C2encap frames from a UDP socket
// decodes and plays it out


//////////////////// CONFIGURATION: CHANGE BELOW 
// set portaudio device
// If set to -1, use "default device" value returned by portaudio
// Pa_GetDefaultInputDevice() function
#define PORTAUDIODEVICE -1

#define IPV4ONLY 0
#define IPV6ONLY 0

#define MINBUFF 5
#define NUMBUFF 8

#define FOREVER 1
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

// for usleep
#include <unistd.h>

// for networking, getaddrinfo and related
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

// defines and structures for codec2-encapsulation
#include <c2encap.h>


// global data structure//
typedef struct {
	int audioready; // are we transmitting ???
	int pa_devicenr; // only used to display information
	int ptr_audio_write; 
	int ptr_audio_read; 
	short int *audiobuffer[NUMBUFF]; // NUMBUFF audio-buffers
	int stereo;
} globaldatastr;

// global data itself //
globaldatastr global;

// functions playaudio (found below)
void * funct_audioout (void *) ;


int main (int argc, char ** argv) {

// vars for networking
int udpport;
struct sockaddr_in localsa4;
struct sockaddr_in6 localsa6;
struct sockaddr * receivefromsa = NULL; // structure used for sendto
socklen_t receivefromsa_size=0; // size of structed used for sendto
unsigned char *udpbuffer;
int udpsock;
int udpsize;

// vars for codec2
void *codec2;
int mode, nsam, nbit, nbyte;

// other vars
int ret;
int new_ptr_audio_write;
int state;

// structure for c2encap data
c2encap c2_voice;
c2encap c2_begin, c2_end;

uint8_t *c2encap_type;
uint8_t *c2encap_begindata;

// "audio out" posix thread
pthread_t thr_audioout;

// init data
global.stereo=-1;
global.ptr_audio_write=1;
global.ptr_audio_read=0;

// We need at least 1 arguments: udpport
if (argc < 2) {
	fprintf(stderr,"Error: at least 1 arguments needed. \n");
	fprintf(stderr,"Usage: %s <udp port> [ <audiodevice> ] \n",argv[0]);
	exit(-1);
}; // end if

udpport=atoi(argv[1]);

// if 1st argument exists, use it as capture device
if (argc >= 3) {
	global.pa_devicenr = atoi(argv[2]);
} else {
	// no argument given; use default
	global.pa_devicenr = PORTAUDIODEVICE;
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


// initialise UDP buffer
udpbuffer=malloc(1500); // we can receive up to 1500 octets

if (!udpbuffer) {
	fprintf(stderr,"Error: could not allocate memory for udpbuffer!\n");
	exit(-1);
}; // end if

// set pointers for c2encap type and c2encap begin-of-data
c2encap_type = (uint8_t*) &udpbuffer[3];
c2encap_begindata = (uint8_t*) &udpbuffer[4]; 

// open inbound UDP socket and bind to socket
if (IPV4ONLY) {
	udpsock=socket(AF_INET,SOCK_DGRAM,0);

	localsa4.sin_family=AF_INET;
	localsa4.sin_port=udpport;
	memset(&localsa4.sin_addr,0,sizeof(struct in_addr)); // address = "::" (all 0) -> we listen

	receivefromsa=(struct sockaddr *) &localsa4;

	ret=bind(udpsock, receivefromsa, sizeof(localsa4)); 

} else {
	// IPV6 socket can handle both ipv4 and ipv6
	udpsock=socket(AF_INET6,SOCK_DGRAM,0);

	// if ipv6 only, set option
	if (IPV6ONLY) {
		int yes=1;

		// make socket ipv6-only.
		ret=setsockopt(udpsock,IPPROTO_IPV6, IPV6_V6ONLY, (char *)&yes,sizeof(int));
		if (ret == -1) {
			fprintf(stderr,"Error: IPV6ONLY option set but could not make socket ipv6 only: %d (%s)! \n",errno,strerror(errno));
			return(-1);
		}; // end if
	}; // end if

	localsa6.sin6_family=AF_INET6;
	localsa6.sin6_port=htons(udpport);
	localsa6.sin6_flowinfo=0; // flows not used
	localsa6.sin6_scope_id=0; // we listen on all interfaces
	memset(&localsa6.sin6_addr,0,sizeof(struct in6_addr)); // address = "::" (all 0) -> we listen

	receivefromsa=(struct sockaddr *)&localsa6;

	ret=bind(udpsock, receivefromsa, sizeof(localsa6)); 

}; // end else - elsif - if

if (ret < 0) {
	fprintf(stderr,"Error: could not bind network-address to socket: %d (%s) \n",errno,strerror(errno));
	exit(-1);
}; // end if

// start audio out thread
pthread_create (&thr_audioout, NULL, funct_audioout, (void *) &global);


// init c2encap structures
memcpy(c2_begin.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2_begin.header[3]=C2ENCAP_MARK_BEGIN;
memcpy(c2_begin.c2data.c2data_text3,"BEG",3);

memcpy(c2_end.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2_end.header[3]=C2ENCAP_MARK_END;
memcpy(c2_end.c2data.c2data_text3,"END",3);

memcpy(c2_voice.header,C2ENCAP_HEAD,sizeof(C2ENCAP_HEAD));
c2_voice.header[3]=C2ENCAP_DATA_VOICE1400;

// in the mean time, do some other things while the audio-process initialises
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

// wait for thread audio to initialise 
while (!global.audioready) {
	// sleep 5 ms
	usleep(5000);
}; // end while

// done. Just to be sure, check "stereo" setting, should be "0" or "1"
if ((global.stereo != 0) && (global.stereo != 1)) {
	fprintf(stderr,"Internal error: stereo flag not set by audioout subthread! Should not happen! Exiting. \n");
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
		} else {
			fprintf(stderr,"Warning: received packet of type 0X%02X in state 0. Ignoring packet! \n",*c2encap_type);
			continue;
		}; // end if
	} else if (state == 1) {
		// state 1: receiving voice data, until we receive a "end" marker
		if (*c2encap_type == C2ENCAP_MARK_END) {
			// end received. Go back to state 0
			state=0;
			continue;
		} else if (*c2encap_type != C2ENCAP_DATA_VOICE1400) {
			fprintf(stderr,"Warning: received packet of type 0X%02X in state 1. Ignoring packet! \n",*c2encap_type);
			continue;
		} else {
			// voice 1400 data packet. Decode and play out

			// first check if there is place to store the result
			new_ptr_audio_write = global.ptr_audio_write+1;
			if (new_ptr_audio_write >= NUMBUFF) {
				// wrap around at NUMBUFF
				new_ptr_audio_write=0;
			}; // end if

			if (new_ptr_audio_write == global.ptr_audio_read) {
				// oeps. No buffers available to write data
//				fputc('B',stderr);
			} else {
//				fputc('Q',stderr);
				codec2_decode(codec2, global.audiobuffer[new_ptr_audio_write],c2encap_begindata);
				
				// make stereo (duplicate channels) if needed
				if (global.stereo) {
					int loop;
					int16_t *p1, *p2;

					// codec2_decode returns a buffer of 320 16-bit samples, MONO
					// so duplicate all samples, start with last sample, move down to sample "1" (not 0);
					p1=&global.audiobuffer[new_ptr_audio_write][619]; // last sample of buffer (320 samples stereo = 640 samples mono)
					p2=&global.audiobuffer[new_ptr_audio_write][319]; // last sample of buffer (mono)

					for (loop=0; loop < 319; loop++) {
						*p1 = *p2; p1--; // copy 1st octet, move down "write" pointer
						*p1 = *p2; p1--; p2--; // copy 2nd time, move down both pointers
					}; // end if
					// last sample, just copy (no need anymore to move pointers)
					*p1 = *p2;
				}; // end if

				// move up pointer in global vars
				global.ptr_audio_write=new_ptr_audio_write;
			}; // end if

		}; // end if
	} else {
		fprintf(stderr,"Internal Error: unknow state %d in audioplay main loop. Should not happen. Exiting!!!\n",state);
		exit(-1);
	}; // end if

}; // end while

fprintf(stderr,"Internal Error: audioplay main application drops out of endless loop. Should not happen! \n");
exit(-1);

}; // end main applicion


////////////////////////////////
// FUNCTIONS

// function "keypress"
// detects <enter> to switch "on" or "off" status
// started as a subthread
void * funct_audioout (void * globalin) {
globaldatastr * p_global;
p_global=(globaldatastr *) globalin;
int loop;
int numaudioframe;
int numBytes;
int16_t *silencebuffer;
int state;
int pawriteerror;
int new_ptr_audio_read;

const PaDeviceInfo *devinfo;

PaStreamParameters outputParameters;
PaStream * stream;
PaError pa_ret;



fprintf(stderr,"INITIALISING PORTAUDIO ... \n");
// open portaudio device
pa_ret=Pa_Initialize();
fprintf(stderr,".... DONE\n");

if (pa_ret != paNoError) {
	Pa_Terminate();
	fprintf(stderr,"Error: Could not initialise Portaudio: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
	exit(-1);
}; // end if

if (p_global->pa_devicenr == -1) {
	// portaudio device = -1 -> use portaudio "get default input device"
	outputParameters.device = Pa_GetDefaultInputDevice();

	if (outputParameters.device == paNoDevice) {
		fprintf(stderr,"Error: no portaudio default input device!\n");
		exit(-1);
	}; // end if
} else {

	if (p_global->pa_devicenr >= Pa_GetDeviceCount()) {
		fprintf(stderr,"Error: portaudio device %d does not exist. Total number of devices: %d \n,",p_global->pa_devicenr, (Pa_GetDeviceCount() -1));
		exit(-1);
	}; // end if

	outputParameters.device = p_global->pa_devicenr;
}; // end else - if

global.pa_devicenr=outputParameters.device;

devinfo = Pa_GetDeviceInfo (outputParameters.device);

fprintf(stderr,"Audio device = %d (%s %s)\n",p_global->pa_devicenr,Pa_GetHostApiInfo(devinfo->hostApi)->name,devinfo->name);

outputParameters.sampleFormat = paInt16;
outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowInputLatency;
outputParameters.hostApiSpecificStreamInfo = NULL;

// first check if samplerate is supported in mono
outputParameters.channelCount = 1;

pa_ret = Pa_IsFormatSupported(NULL,&outputParameters,(double) RATE);
if (pa_ret == paFormatIsSupported) {
	p_global->stereo=0;

	fprintf(stderr,"8 KHz sampling rate support in mono! \n");
} else {
	// try again in stereo
	outputParameters.channelCount = 2;

	pa_ret = Pa_IsFormatSupported(NULL,&outputParameters,(double) RATE);
	if (pa_ret == paFormatIsSupported) {
		p_global->stereo=1;
		fprintf(stderr,"8 KHz sampling rate support in stereo! \n");
	} else {
		fprintf(stderr,"Error: 8 Khz sampling not supported in mono or stereo! Exiting. \n");
		exit(-1);
	}; // end if
}; // end 


// configure portaudio global data
if (p_global->stereo) {
	numBytes = 1280;
} else {
	numBytes = 640; 
}; // end if

for (loop=0; loop < NUMBUFF; loop++) {
	// create memory for audiobuffer
	p_global->audiobuffer[loop] = malloc(numBytes); // allow memory for buffer 0
	if (!p_global->audiobuffer[loop]) {
		// memory could not be allocated
		fprintf(stderr,"Error: could not allocate memory for portaudio buffer %d!\n",loop);
		exit(-1);
	}; // end if

}; // end for

silencebuffer = malloc(numBytes); // allow memory for buffer 1
if (!silencebuffer) {
	// memory could not be allocated
	fprintf(stderr,"Error: could not allocate memory for silence buffer 1!\n");
	exit(-1);
}; // end if

// fill up silence with all 0
memset(silencebuffer,0,numBytes);

// open PortAudio stream
// do not start stream yet, will be done further down
pa_ret = Pa_OpenStream (
	&stream,
	NULL, // input Parameters, not used here 
	&outputParameters,
	RATE, // sample rate
	320, // frames per buffer: 40 ms @ 8000 samples/sec
	paClipOff, // we won't output out of range samples,
					// so don't bother clipping them
	NULL, // no callback function, syncronous read
	&global // parameters passed to callback function
);

if (pa_ret != paNoError) {
	Pa_Terminate();
	fprintf(stderr,"Error in Pa_OpenStream: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
	exit(-1);
}; // end if

// Start stream
pa_ret=Pa_StartStream(stream);

if (pa_ret != paNoError) {
	Pa_Terminate();
	fprintf(stderr,"Error in Pa_StartStream: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
	exit(-1);
}; // end if


// set audioready to on
p_global->audioready = 1;
state=0; // 0 = waiting for audio (at least 2 buffers filled), 1 = playing
pawriteerror=0; // number of Pa_WriteStream errors

while (( pa_ret = Pa_IsStreamActive (stream)) == 1) {
	// play audio

	// statemachine
	if (state == 0) {
		// state = 0: wait for audio, we need at least 2 buffers filled up before we start playing
		numaudioframe=p_global->ptr_audio_write - p_global->ptr_audio_read; // (how many audio frames are there in the buffer to play

		if (numaudioframe < 0) {
			// wrap around if needed
			numaudioframe += NUMBUFF;
		}; // end if

		if (numaudioframe < MINBUFF + 1) {
			// 0 or 1 audioframes is buffer. play silence and try again
			pa_ret = Pa_WriteStream(stream, silencebuffer, 160);
			//pa_ret = Pa_WriteStream(stream, silencebuffer, 320);
//			putc('W',stderr);
		} else {
//			putc('1',stderr);
			// 2 or more audioframes in buffer, set state to 1 and go back in loop
			state=1;
			continue;
		}; // end else - if

	} else if (state == 1) {
		// state = 1, play audio if still available, otherwize, go back to state 0
		new_ptr_audio_read = p_global->ptr_audio_read + 1;
		if (new_ptr_audio_read >= NUMBUFF) {
			new_ptr_audio_read=0 ; // wrap around at NUMBUFF
		}; // end if

		if (new_ptr_audio_read != p_global->ptr_audio_write) {
//			putc('R',stderr);
			// audio to play
			pa_ret = Pa_WriteStream(stream, p_global->audiobuffer[p_global->ptr_audio_read], 320);
			// move up marker
			p_global->ptr_audio_read=new_ptr_audio_read;
		} else {
//			putc('0',stderr);
			// no audio to play anymore: set state to 0 and go back to in loop
			state=0;
			continue;
		}; // end if
	} else {
		// unknown state
		fprintf(stderr,"Error: unknown state in audio playback loop: %d. Resetting! \n",state);

		// reset and go back
		state=0;
		continue;
	}; // end if

	// if we reach this, we have done a Pa_WriteStream

	if (pa_ret != paNoError) {
		Pa_Terminate();
		fprintf(stderr,"Error in Pa_WriteStream: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);

		pawriteerror++;

		if (pawriteerror > 50) {
			fprintf(stderr,"Error in more then 50 consequative write. Exiting!\n");
			exit(-1);
		}; // end if
	} else {
		// write was successfull, reset error counter
		pawriteerror=0;
	}; // end if

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

exit(-1);


}; // end function funct_audioin
