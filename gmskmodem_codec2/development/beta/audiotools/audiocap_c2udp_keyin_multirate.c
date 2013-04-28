/* audio capture */

// Reads audio from a portaudio audio device
// If the <enter> key is pushed, the audio is codec2-encoded and send as a
// UDP stream to the gmskmodem


//////////////////// CONFIGURATION: CHANGE BELOW 

#define IPV4ONLY 0
#define IPV6ONLY 0
//////////////////// END
//////////////////// DO NOT CHANGE ANYTHING BELOW UNLESS YOU KNOW WHAT
//////////////////// YOU ARE DOING

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

// for samplerate conversion
#include <samplerate.h>

// defines and structures for codec2-encapsulation
#include "c2encap.h"

// global data structure//
typedef struct {
	int transmit; // are we transmitting ???
} globaldatastr;

// global data itself //
globaldatastr global;

// functions defined below
void * funct_keypress (void *) ;


int main (int argc, char ** argv) {
int buffersize;
int stereo;
int samplerate;
int numBytes;
int numSample;
int maxnumchannel_input;

// state of keypress
int state, oldstate;

// vars for portaudio
char * portaudiodevice;
int exact=0;
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

// for SAMPLE RATE CONVERSION
SRC_STATE *src=NULL;
SRC_DATA src_data;
int src_error;

// vars for codec2
void *codec2;
unsigned char *c2_buff;
int mode, nc2byte;

// vars for audio
int16_t * audiobuffer;
float * inaudiobuffer_f = NULL;
float * outaudiobuffer_f = NULL;

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

// We need at least 3 arguments: IP-address, udpport and samplerate
if (argc < 4) {
	fprintf(stderr,"Error: at least 3 arguments needed. \n");
	fprintf(stderr,"Usage: %s <ip-addr> <udp port> <samplerate> [ <audiodevice> [exact] ] \n",argv[0]);
	fprintf(stderr,"Note: allowed audio samplerate are 8000, 44100 or 48000 samples/second.\n");
	fprintf(stderr,"Note: use device \"\" to get list of devices.\n");
	exit(-1);
}; // end if

ipaddrtxtin=argv[1];
udpport=atoi(argv[2]);
samplerate=atoi(argv[3]);


// if 1st argument exists, use it as capture device
if (argc >= 5) {
	portaudiodevice = argv[4];

	// is there the "exact" statement?
	if (argc >= 6) {
		if (!strcmp(argv[5],"exact")) {
			exact=1;
		} else {
			fprintf(stderr,"Error: parameter \"exact\" expected. Got %s. Ignoring! \n",argv[5]);
		}; // end else - if
	}; // end if
} else {
	// no argument given
	portaudiodevice = NULL;
}; // end else - if



// create network structure
if ((udpport < 0) || (udpport > 65535)) {
	fprintf(stderr,"Error: UDPport number must be between 0 and 65535! \n");
	exit(-1);
}; // end if


if ((IPV4ONLY) && (IPV6ONLY)) {
	fprintf(stderr,"Error: internal configuration error: ipv4only and ipv6only are mutually exclusive! \n");
	exit(-1);
}; // end if


// sample rates below 8Ksamples/sec or above 48Ksamples/sec do not make sence
if (samplerate == 8000) {
	numSample = 320;
} else if (samplerate == 44100) {
	numSample = 1764;
} else if (samplerate == 48000) {
	numSample = 1920;
} else {
	fprintf(stderr,"Error: audio samplerate should be 8000, 44100 or 48000 samples/sec! \n");
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
	inet_ntop(AF_INET6,&udp_aiaddr_in6->sin6_addr,ipaddrtxt,INET6_ADDRSTRLEN);
	
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



// PORTAUDIO STUFF

fprintf(stderr,"INITIALISING PORTAUDIO    (this can take some time, please ignore any errors below) .... \n");
// open portaudio device
pa_ret=Pa_Initialize();
fprintf(stderr,".... DONE\n");

if (pa_ret != paNoError) {
	Pa_Terminate();
	fprintf(stderr,"Error: Could not initialise Portaudio: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
	exit(-1);
}; // end if

if (portaudiodevice == NULL) {
	// portaudio device = NULL -> use portaudio "get default input device"
	inputParameters.device = Pa_GetDefaultInputDevice();

	if (inputParameters.device == paNoDevice) {
		fprintf(stderr,"Error: no portaudio default input device!\n");
		exit(-1);
	}; // end if

	if (inputParameters.device >= Pa_GetDeviceCount()) {
		fprintf(stderr,"Internal Error: portaudio \"GetDefaultInputDevice\" returns device number %d while possible devices go from 0 to %d \n,",inputParameters.device, (Pa_GetDeviceCount() -1) );
		exit(-1);
	}; // end if


	// check if device supports samplerate:
	inputParameters.sampleFormat = paInt16;
	inputParameters.suggestedLatency = 0; // not used in Pa_IsFormatSupported
	inputParameters.hostApiSpecificStreamInfo = NULL;

	devinfo = Pa_GetDeviceInfo (inputParameters.device);

   maxnumchannel_input = devinfo->maxInputChannels;
	printf("Audio device = %d (%s %s)\n",inputParameters.device,Pa_GetHostApiInfo(devinfo->hostApi)->name,devinfo->name);

	if (maxnumchannel_input >= 1) {
		// first check if samplerate is supported in mono
		inputParameters.channelCount = 1;
		pa_ret = Pa_IsFormatSupported(NULL,&inputParameters,(double) samplerate);

		if (pa_ret == paFormatIsSupported) {
			printf("Samplerate %d supported in mono.\n",samplerate);
			stereo=0;
		} else {
			// try again using stereo
			inputParameters.channelCount = 2;

			if (maxnumchannel_input >= 2) {
				pa_ret = Pa_IsFormatSupported(NULL,&inputParameters,(double) samplerate);

				if (pa_ret == paFormatIsSupported) {
					printf("Samplerate %d supported in stereo.\n",samplerate);
					stereo=1;
				} else {
					printf("Error: Samplerate %d not supported in mono or stereo!\n",samplerate);
					exit(-1);
				}; // end if
			} else {
				// stereo not supported on this device
				printf("Error: Samplerate %d not supported in mono. Stereo not supported on this device!\n",samplerate);
				exit(-1);
			}; // end if
		}; // end else - if
	} else {
		printf("Error: input not supported on this device!\n");
		exit(-1);
	}; // end if
	
	printf("\n");
	fflush(stdout);


} else {
	// CLI option "device" contains text, look throu list of all devices if there are
	// devices that match that name and support the particular requested samplingrate
	int loop;
	int numdevice;

	int numdevicefound=0;
	int devicenr=0;
	int devicestereo=0;



	// init some vars
	numdevice=Pa_GetDeviceCount();

	inputParameters.sampleFormat = paInt16;
	inputParameters.suggestedLatency = 0; // not used in Pa_IsFormatSupported
	inputParameters.hostApiSpecificStreamInfo = NULL;

	for (loop=0; loop<numdevice;loop++) {
		int devnamematch=0;

		// get name of device
		devinfo = Pa_GetDeviceInfo (loop);

		// only do check if searchstring is smaller or equal is size of device name
		if (strlen(devinfo->name) >= strlen(portaudiodevice)) {
			int numcheck;
			int devnamesize;
			int loop;
			char *p;

			// init pointer to beginning of string
			p=(char *)devinfo->name;
			devnamesize = strlen(portaudiodevice);

			if (exact) {
				// exact match, only check once: at the beginning
				numcheck=1;
			} else {
				numcheck=strlen(p) - strlen(portaudiodevice) +1;
			}; // end if

			// loop until text found or end-of-string
			for (loop=0; (loop<numcheck && devnamematch == 0); loop++) {
				if (strncmp(portaudiodevice,p,devnamesize) ==0) {
					devnamematch=1;
				}; // end if

				// move up pointer
				p++;
			};
		}; // end if

		if (devnamematch) {
			printf("Audio device: %d (API: %s ,NAME: %s)\n",loop,Pa_GetHostApiInfo(devinfo->hostApi)->name,devinfo->name);

   		maxnumchannel_input = devinfo->maxInputChannels;

			if (maxnumchannel_input >= 1) {
				// next step: check if this device supports the particular requested samplerate
				inputParameters.device = loop;

				inputParameters.channelCount = 1;
				pa_ret = Pa_IsFormatSupported(NULL,&inputParameters,(double) samplerate);

				if (pa_ret == paFormatIsSupported) {
					printf("Samplerate %d supported in mono.\n",samplerate);
					numdevicefound++;
					devicenr=loop;
					devicestereo=0;
				} else {
					if (maxnumchannel_input >= 2) {
						inputParameters.channelCount = 2;
						pa_ret = Pa_IsFormatSupported(NULL,&inputParameters,(double) samplerate);

						if (pa_ret == paFormatIsSupported) {
							printf("Samplerate %d supported in stereo.\n",samplerate);
							numdevicefound++;
							devicenr=loop;
							devicestereo=1;
						} else {
							printf("Error: Samplerate %d not supported in mono or stereo.\n",samplerate);
						}; // end else - if
					} else {
						// stereo not supported on this device
						printf("Error: Samplerate %d not supported in mono. Stereo not supported on this device!\n",samplerate);
					}; // end if
				}; // end else - if
			} else {
				printf("Error: Input not supported on device.\n");
			}; // end if

			printf("\n");
			fflush(stdout);
		};// end if
	}; // end for

	// did we find any device
	if (numdevicefound == 0) {
		fprintf(stderr,"Error: did not find any audio-device supporting that audio samplerate\n");
		fprintf(stderr,"       Try again with other samplerate of devicename \"\" to get list of all devices\n");
		exit(-1);
	} else if (numdevicefound > 1) {
		fprintf(stderr,"Error: Found multiple devices matching devicename supporting that audio samplerate\n");
		fprintf(stderr,"       Try again with a more strict devicename or use \"exact\" clause!\n");
		exit(-1);
	} else {
		// OK, we have exactly one device: copy its parameters
		inputParameters.device=devicenr;
		stereo=devicestereo;

		if (devicestereo) {
			inputParameters.channelCount = 2;
		} else {
			inputParameters.channelCount = 1;
		}; // end else - if

		// get name info from device
		devinfo = Pa_GetDeviceInfo (inputParameters.device);

		fprintf(stderr,"Selected Audio device = (API: %s ,NAME: %s)\n",Pa_GetHostApiInfo(devinfo->hostApi)->name,devinfo->name);
	};
}; // end else - if

// set other parameters of inputParameters structure
inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;

// configure portaudio global data
if (samplerate == 8000) {
	numSample = 320;
} else if (samplerate == 44100) {
	numSample = 1764;
} else if (samplerate == 48000) {
	numSample = 1920;
} else {
	fprintf(stderr,"Error: invalid value for samplerate in funct_audioout: %d !\n",samplerate);
	exit(-1);
}; // end if


// configure portaudio global data
if (stereo) {
	numBytes = (numSample << 2);
} else {
	numBytes = (numSample << 1); 
}; // end if

// create memory for audiobuffer
audiobuffer = malloc(numBytes); // allow memory for buffer 0
if (!audiobuffer) {
	// memory could not be allocated
	fprintf(stderr,"Error: could not allocate memory for portaudio buffer 0!\n");
	exit(-1);
}; // end if


// some network debug info
fprintf(stderr,"Sending CODEC2 DV stream to ip-address %s udp port %d\n",ipaddrtxt,udpport);


// open PortAudio stream
// do not start stream yet, will be done further down
pa_ret = Pa_OpenStream (
	&stream,
	&inputParameters,
	NULL, // output Parameters, not used here 
	samplerate, // sample rate
	numSample, // frames per buffer: 40 ms @ 8000 samples/sec
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

nc2byte = (codec2_bits_per_frame(codec2) + 7) >> 3; // ">>3" is same as "/8"

if (nc2byte != 7) {
	fprintf(stderr,"Error: number of bytes for codec2 frames should be 7. We got %d \n",nc2byte);
}; // end if

if (codec2_samples_per_frame(codec2) != 320) {
	fprintf(stderr,"Error: number of samples for codec2 frames should be 320. We got %d \n",codec2_samples_per_frame(codec2));
}; // end if

c2_buff = (unsigned char *)&c2_voice.c2data.c2data_data7;


// allocate audiobuffer
if (stereo) {
	buffersize= numSample << 2; // = number of samples  * 4 (stereo and 16 bit/sample)
} else {
	// mono
	buffersize= numSample << 1; // = number of samples * 2 (16 bit/sample)
}; // end else - if
audiobuffer=malloc(buffersize);

if (!audiobuffer) {
	fprintf(stderr,"Error: malloc audiobuffer: %s",strerror(errno));
	exit(-1);
}; // end if


// init samplerate conversion
if (samplerate != 8000) {

// allocate memory for audio sample buffers (only needed when audio rate conversion is used)
	inaudiobuffer_f=malloc(numSample * sizeof(float));
	if (!inaudiobuffer_f) {
		fprintf(stderr,"Error in malloc for inaudiobuffer_f! \n");
		exit(-1);
	}; // end if

	outaudiobuffer_f=malloc(320 * sizeof(float)); // output buffer is 320 samples (40 ms @ 8000 samples/sec)
	if (!outaudiobuffer_f) {
		fprintf(stderr,"Error in malloc for outaudiobuffer_f! \n");
		exit(-1);
	}; // end if

	src = src_new(SRC_SINC_FASTEST,1,&src_error);

	if (!src) {
		fprintf(stderr,"src_new failed! \n");
		exit(-1);
	}; // end if

	src_data.data_in = inaudiobuffer_f;
	src_data.data_out = outaudiobuffer_f;
	src_data.input_frames = numSample;
	src_data.output_frames = 320; // 40 ms @ 8000 samples / sec
	src_data.end_of_input = 0; // no further data, every 40 ms frame is concidered to be a seperate unit

	if (samplerate == 48000) {
		src_data.src_ratio = (float) 8000/48000;
	} else {
		src_data.src_ratio = (float) 8000/44100;
	}; // end else - if
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

	pa_ret = Pa_ReadStream(stream, audiobuffer, numSample);

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
	
			for (loop=1; loop < numSample; loop++) {
				*p1=*p2;
				p1++; p2 += 2;
			}; // end for
		}; // end if


		// if not 8000 samples / second: convert
		if (samplerate != 8000) {
fprintf(stderr,"2!!! \n");
			if (!inaudiobuffer_f) {
				fprintf(stderr,"Internal Error: inaudiobuffer_f not initialised \n");
				exit(-1);
			}; // end if

			if (!outaudiobuffer_f) {
				fprintf(stderr,"Internal Error: outaudiobuffer_f not initialised \n");
				exit(-1);
			}; // end if

			// convert int16 to float
			src_short_to_float_array(audiobuffer,inaudiobuffer_f,numSample);

			// convert
			ret=src_process(src,&src_data);

			if (ret) {
				fprintf(stderr,"Warning: samplerate conversion error %d (%s)\n",ret,src_strerror(ret));
			}; // end if

			// some error checking
			if (src_data.output_frames_gen != 320) {
				fprintf(stderr,"Warning: number of frames generated by samplerateconvert should be %d, got %ld. \n",numSample,src_data.output_frames_gen);
			}; // end if

			// convert back from float to int
			src_float_to_short_array(outaudiobuffer_f,audiobuffer,320); // 40 ms @ 8000 samples/sec = 320 samples

fprintf(stderr,"3!!! \n");
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


fprintf(stderr,"AUDIO CAPTURE, ENCODE AND STREAM OUT\n");
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
