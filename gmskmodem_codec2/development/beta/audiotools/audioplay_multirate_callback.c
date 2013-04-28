/* audio playback */

// received C2encap frames from a UDP socket
// decodes and plays it out

// version 20121204: initial release
// version 20121209: multiple audio samplerate support


//////////////////// CONFIGURATION: CHANGE BELOW 
#define IPV4ONLY 0
#define IPV6ONLY 0

#define MINBUFF 5
#define NUMBUFF 8

#define FOREVER 1
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

// for usleep
#include <unistd.h>

// for networking, getaddrinfo and related
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

// for sample rate conversion
#include <samplerate.h>

// defines and structures for codec2-encapsulation
#include <c2encap.h>


// global data structure//
typedef struct {
	int audioready; // are we transmitting ???
	char * pa_device; // only used to display information
	int ptr_audio_write; 
	int ptr_audio_read; 
	int16_t *audiobuffer[NUMBUFF]; // NUMBUFF audio-buffers
	int stereo;
	int rate;
	int exact;
	int numBytes;
	int numSample;
} globaldatastr;

// global data itself //
globaldatastr global;

// audioplay callback function found below
static int funct_callbackwrite( const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData );

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


// vars for audio
int16_t *inaudiobuffer; // tempory audio buffer used when audio sampling is needed
float *inaudiobuffer_f = NULL; // used for audio samplerate conversion
float *outaudiobuffer_f = NULL; // used for audio samplerate conversion

// for SAMPLE RATE CONVERSION
SRC_STATE *src=NULL;
SRC_DATA src_data;
int src_error;

// vars for codec2
void *codec2;
int mode, nc2byte;

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
global.exact=0;

// We need at least 2 arguments: udpport and samplerate
if (argc < 3) {
	fprintf(stderr,"Error: at least 2 arguments needed. \n");
	fprintf(stderr,"Usage: %s <udp port> <samplerate> [ <audiodevice> [exact] ] \n",argv[0]);
	fprintf(stderr,"Note: allowed audio samplerate are 8000, 44100 or 48000 samples/second.\n");
	fprintf(stderr,"Note: use device \"\" to get list of devices.\n");
	exit(-1);
}; // end if

udpport=atoi(argv[1]);
global.rate=atoi(argv[2]);


// if 1st argument exists, use it as capture device
if (argc >= 4) {
	global.pa_device = argv[3];

	// is there the "exact" statement?
	if (argc >= 5) {
		if (!strcmp(argv[4],"exact")) {
			global.exact=1;
		} else {
			fprintf(stderr,"Error: parameter \"exact\" expected. Got %s. Ignoring! \n",argv[4]);
		}; // end else - if
	}; // end if
} else {
	// no argument given; use default
	global.pa_device = NULL;
}; // end else - if


// sample rates below 8Ksamples/sec or above 48Ksamples/sec do not make sence
if ((global.rate != 8000) &&  (global.rate != 44100) && (global.rate != 48000)) {
	fprintf(stderr,"Error: audio samplerate should be 8000, 44100 or 48000 samples/sec! \n");
	exit(-1);
}; // end if



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

nc2byte = (codec2_bits_per_frame(codec2) + 7) >> 3; // ">>3" is same as "/8"

if (nc2byte != 7) {
	fprintf(stderr,"Error: number of bytes for codec2 frames should be 7. We got %d \n",nc2byte);
}; // end if

if (codec2_samples_per_frame(codec2) != 320) {
	fprintf(stderr,"Error: number of samples for codec2 frames should be 320. We got %d \n",codec2_samples_per_frame(codec2));
}; // end if

// wait for thread audio to initialise 
while (!global.audioready) {
	// sleep 5 ms
	usleep(5000);
}; // end while

// done. Just to be sure, check "stereo" setting, should be "0" or "1"
if ((global.stereo != 0) && (global.stereo != 1)) {
	fprintf(stderr,"Internal error: stereo flag not set correctly by audioout subthread! Should not happen! Exiting. \n");
	exit(-1);
}; // end if


if (global.rate != 8000) {
// allocate memory for audio sample buffers (only needed when audio rate conversion is used)
	inaudiobuffer=malloc(320 * sizeof(int16_t));
	if (!inaudiobuffer) {
		fprintf(stderr,"Error in malloc for inaudiobuffer! \n");
		exit(-1);
	}; // end if

	inaudiobuffer_f=malloc(320 * sizeof(float));
	if (!inaudiobuffer_f) {
		fprintf(stderr,"Error in malloc for inaudiobuffer_f! \n");
		exit(-1);
	}; // end if

	outaudiobuffer_f=malloc(global.numSample * sizeof(float));
	if (!outaudiobuffer_f) {
		fprintf(stderr,"Error in malloc for outaudiobuffer_f! \n");
		exit(-1);
	}; // end if

	// init samplerate conversion
	src = src_new(SRC_SINC_FASTEST, 1, &src_error);

	if (!src) {
		fprintf(stderr,"src_new failed! \n");
		exit(-1);
	}; // end if

	src_data.data_in = inaudiobuffer_f;
	src_data.data_out = outaudiobuffer_f;
	src_data.input_frames = 320; // 40ms @ 8000 samples/sec
	src_data.output_frames = global.numSample;
	src_data.end_of_input = 0; // no further data, every 40 ms is processed on itself

	if (global.rate == 48000) {
		src_data.src_ratio = (float) 48000/8000;
	} else {
		src_data.src_ratio = (float) 44100/8000;
	}; // end if
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
				fputc('B',stderr);
			} else {
//				fputc('Q',stderr);


				// decode codec2 frame
  				codec2_decode(codec2, global.audiobuffer[new_ptr_audio_write],c2encap_begindata);
				

				// if not samplerate 8000, do rate conversion
				if (global.rate != 8000) {
					// convert int16 to float
					if (!inaudiobuffer_f) {
						fprintf(stderr,"Internal error: inaudiobuffer_f not initialised! \n");
						exit(-1);
					}; // "end if

					if (!outaudiobuffer_f) {
						fprintf(stderr,"Internal error: outaudiobuffer_f not initialised! \n");
						exit(-1);
					}; // "end if


					src_short_to_float_array(global.audiobuffer[new_ptr_audio_write],inaudiobuffer_f,320);

					// do conversion
					ret=src_process(src,&src_data);

					if (ret) {
						fprintf(stderr,"Warning: samplerate conversion error %d (%s)\n",ret,src_strerror(ret));
					}; // end if

					// some error checking
					if (src_data.output_frames_gen != global.numSample) {
						fprintf(stderr,"Warning: number of frames generated by samplerateconvert should be %d, got %ld. \n",global.numSample,src_data.output_frames_gen);
					}; // end if

					// convert back from float to int, and store immediatly in ringbuffer
					src_float_to_short_array(outaudiobuffer_f,global.audiobuffer[new_ptr_audio_write],global.numSample );
				}; // end if (samplerate != 8000)


				// make stereo (duplicate channels) if needed
				if (global.stereo) {
					int loop;
					int16_t *p1, *p2;

					int lastsample_m, lastsample_s;

					lastsample_m = global.numSample - 1;
					lastsample_s = global.numSample*2 - 1;

					// codec2_decode returns a buffer of 16-bit samples, MONO
					// so duplicate all samples, start with last sample, move down to sample "1" (not 0);
					p1=&global.audiobuffer[new_ptr_audio_write][lastsample_s]; // last sample of buffer (320 samples stereo = 640 samples mono)
					p2=&global.audiobuffer[new_ptr_audio_write][lastsample_m]; // last sample of buffer (mono)

					for (loop=0; loop < lastsample_m; loop++) {
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

}; // end main application


////////////////////////////////
// FUNCTIONS

// function "audio out"
// started as a subthread
void * funct_audioout (void * globalin) {
globaldatastr * p_global;
p_global=(globaldatastr *) globalin;
int loop;
int16_t *silencebuffer;
int maxnumchannel_output;

const PaDeviceInfo *devinfo;

PaStreamParameters outputParameters;
PaStream * stream;
PaError pa_ret;



printf("INITIALISING PORTAUDIO (this can take some time, please ignore any errors below) ... \n");
// open portaudio device
pa_ret=Pa_Initialize();
printf(".... DONE\n");
fflush(stdout);

if (pa_ret != paNoError) {
	Pa_Terminate();
	fprintf(stderr,"Error: Could not initialise Portaudio: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
	exit(-1);
}; // end if

if (p_global->pa_device == NULL) {
	// portaudio device = NULL -> use portaudio "get default output device"
	outputParameters.device = Pa_GetDefaultOutputDevice();

	if (outputParameters.device == paNoDevice) {
		fprintf(stderr,"Error: no portaudio default output device!\n");
		exit(-1);
	}; // end if

	if (outputParameters.device >= Pa_GetDeviceCount()) {
		fprintf(stderr,"Internal Error: portaudio \"GetDefaultOutputDevice\" returns device number %d while total number of devices is %d \n,",outputParameters.device, Pa_GetDeviceCount() );
		exit(-1);
	}; // end if

	// check if device supports samplerate:
	outputParameters.sampleFormat = paInt16;
	outputParameters.suggestedLatency = 0; // not used in Pa_IsFormatSupported
	outputParameters.hostApiSpecificStreamInfo = NULL;

	devinfo = Pa_GetDeviceInfo (outputParameters.device);

   maxnumchannel_output = devinfo->maxOutputChannels;
	printf("Audio device = %d (API: %s ,NAME: %s)\n",outputParameters.device,Pa_GetHostApiInfo(devinfo->hostApi)->name,devinfo->name);


	if (maxnumchannel_output >= 1) {
		// first check if samplerate is supported in mono
		outputParameters.channelCount = 1;
		pa_ret = Pa_IsFormatSupported(NULL,&outputParameters,(double) p_global->rate);

		if (pa_ret == paFormatIsSupported) {
			printf("Samplerate %d supported in mono.\n",p_global->rate);
			p_global->stereo=0;
		} else {
			// try again using stereo

			if (maxnumchannel_output >= 2) {
				outputParameters.channelCount = 2;
				pa_ret = Pa_IsFormatSupported(NULL,&outputParameters,(double) p_global->rate);

				if (pa_ret == paFormatIsSupported) {
					printf("Samplerate %d supported in stereo.\n",p_global->rate);
					p_global->stereo=1;
				} else {
					printf("Error: Samplerate %d not supported in mono or stereo!\n",p_global->rate);
					exit(-1);
				}; // end if
			} else {
				printf("Error: Samplerate %d not supported in mono. Stereo not supported on this device!\n",p_global->rate);
				exit(-1);
			}; // end if
		}; // end else - if
	} else {
		printf("Error: Output not supported on this device\n");
		exit(-1);
	}; // end else - if
	
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

	outputParameters.sampleFormat = paInt16;
	outputParameters.suggestedLatency = 0; // not used in Pa_IsFormatSupported
	outputParameters.hostApiSpecificStreamInfo = NULL;

	for (loop=0; loop<numdevice;loop++) {
		int devnamematch=0;

		// get name of device
		devinfo = Pa_GetDeviceInfo (loop);

		if (strlen(devinfo->name) >= strlen(p_global->pa_device)) {
			// only check if searchstring is smaller or equal is size of device name
			int numcheck;
			int devnamesize;
			int loop;
			char *p;

			// init pointer to beginning of string
			p=(char *)devinfo->name;
			devnamesize = strlen(p_global->pa_device);

			if (p_global->exact) {
				// exact match, only check once: at the beginning
				numcheck=1;
			} else {
				numcheck=strlen(p) - strlen(p_global->pa_device) +1;
			}; // end if

			// loop until text found or end-of-string
			for (loop=0; (loop<numcheck && devnamematch == 0); loop++) {
				if (strncmp(p_global->pa_device,p,devnamesize) ==0) {
					devnamematch=1;
				}; // end if

				// move up pointer
				p++;
			};
		}; // end if

		if (devnamematch) {
			maxnumchannel_output = devinfo->maxOutputChannels;
			
			if (maxnumchannel_output >= 1) {
				// next step: check if this device supports the particular requested samplerate
				outputParameters.device = loop;

				outputParameters.channelCount = 1;
				pa_ret = Pa_IsFormatSupported(NULL,&outputParameters,(double) p_global->rate);

				if (pa_ret == paFormatIsSupported) {
					printf("Audio device: %d (API: %s ,NAME: %s)\n",loop,Pa_GetHostApiInfo(devinfo->hostApi)->name,devinfo->name);
					printf("Samplerate %d supported in mono.\n",p_global->rate);
					numdevicefound++;
					devicenr=loop;
					devicestereo=0;
				} else {
					if (maxnumchannel_output >= 2) {
						// try again using stereo
						outputParameters.channelCount = 2;
						pa_ret = Pa_IsFormatSupported(NULL,&outputParameters,(double) p_global->rate);

						if (pa_ret == paFormatIsSupported) {
							printf("Audio device: %d (API: %s ,NAME: %s)\n",loop,Pa_GetHostApiInfo(devinfo->hostApi)->name,devinfo->name);
							printf("Samplerate %d supported in stereo.\n",p_global->rate);
							numdevicefound++;
							devicenr=loop;
							devicestereo=1;
//						} else {
//							printf("Error: Samplerate %d not supported in mono or stereo.\n",p_global->rate);
						}; // end else - if
//					} else {
//						printf("Error: Samplerate %d not supported in mono. Stereo not supported on this device!\n",p_global->rate);
					}; // end else - if
				}; // end else - if
//			} else {
//				printf("Error: Output not supported on device.\n");
			}; // end if

			printf("\n");
			fflush(stdout);
		};// end if
	}; // end for

	// did we find any device?
	if (numdevicefound == 0) {
		fprintf(stderr,"Error: did not find any audio-device supporting that audio samplerate\n");
		fprintf(stderr,"       Try again with other samplerate of devicename \"\" to get list of all devices\n");
		exit(-1);
	} else if (numdevicefound > 1) {
		fprintf(stderr,"Error: Found multiple devices matching devicename supporting that audio samplerate\n");
		fprintf(stderr,"       Try again with a more strict devicename or use the \"exact\" clause!\n");
		exit(-1);
	} else {
		// OK, we have exactly one device: copy its parameters
		outputParameters.device=devicenr;
		p_global->stereo=devicestereo;

		if (devicestereo) {
			outputParameters.channelCount = 2;
		} else {
			outputParameters.channelCount = 1;
		}; // end else - if

		// get name info from device
		devinfo = Pa_GetDeviceInfo (outputParameters.device);

		fprintf(stderr,"Selected Audio device = (API: %s ,NAME: %s)\n",Pa_GetHostApiInfo(devinfo->hostApi)->name,devinfo->name);

	};

}; // end else - if

// set other parameters of outputParameters structure
outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;

// configure portaudio global data
if (p_global->rate == 8000) {
	global.numSample = 320;
} else if (p_global->rate == 44100) {
	global.numSample = 1764;
} else if (p_global->rate == 48000) {
	global.numSample = 1920;
} else {
	fprintf(stderr,"Error: invalid value for samplerate in funct_audioout: %d !\n",p_global->rate);
	exit(-1);
}; // end if

if (p_global->stereo) {
	// int16 = 2 octets / sample * 2 for stereo
	global.numBytes=(global.numSample << 2);
} else {
	// int16 = 2 octets / sample
	global.numBytes=(global.numSample << 1);
}; // end if

for (loop=0; loop < NUMBUFF; loop++) {
	// create memory for audiobuffer
	p_global->audiobuffer[loop] = malloc(global.numBytes); // allow memory for buffer 0
	if (!p_global->audiobuffer[loop]) {
		// memory could not be allocated
		fprintf(stderr,"Error: could not allocate memory for portaudio buffer %d!\n",loop);
		exit(-1);
	}; // end if

}; // end for

silencebuffer = malloc(global.numBytes); // allow memory for silence (10 ms, so numSample@40ms / 4)
if (!silencebuffer) {
	// memory could not be allocated
	fprintf(stderr,"Error: could not allocate memory for silence buffer 1!\n");
	exit(-1);
}; // end if

// fill up silence with all 0
memset(silencebuffer,0,global.numBytes);

// open PortAudio stream
// do not start stream yet, will be done further down
pa_ret = Pa_OpenStream (
	&stream,
	NULL, // input Parameters, not used here 
	&outputParameters,
	p_global->rate, // sample rate
	global.numSample, // frames per buffer: 320, 1764 or 1920 (= 8000, 44100 or 48000 samples / sec @ 40 ms)
	paClipOff, // we won't output out of range samples,
					// so don't bother clipping them
	funct_callbackwrite, // callback function
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

while (( pa_ret = Pa_IsStreamActive (stream)) == 1) {
	// endless sleep, wake up and check status every 2 seconds
	Pa_Sleep(1000);
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


//////////////////////////////
// audioplay callback function 
static int funct_callbackwrite( const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *globalin ) {

globaldatastr * p_global;
p_global=(globaldatastr *) globalin;
static int init=1;
static int state;
int numaudioframe, new_ptr_audio_read;

// init vars first time function is called
if (init) {
	state=0;
	init=0;
}; // 

if (framesPerBuffer != p_global->numSample) {
	fprintf(stderr,"FramePerBuffer is %ld, expected %d \n",framesPerBuffer,p_global->numSample);
}; // end if

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
		memset(outputBuffer,0,global.numBytes);

		return paContinue;
//	putc('W',stderr);
	} else {
//	putc('1',stderr);
		// 2 or more audioframes in buffer, set state to 1 and continue below
		state=1;
	}; // end else - if
};

// state = 1, play audio if still available, otherwize, go back to state 0
new_ptr_audio_read = p_global->ptr_audio_read + 1;
if (new_ptr_audio_read >= NUMBUFF) {
	new_ptr_audio_read=0 ; // wrap around at NUMBUFF
}; // end if

// STATE 1, senario 1: still more data
if (new_ptr_audio_read != p_global->ptr_audio_write) {
//	putc('R',stderr);
	// audio to play
	memcpy(outputBuffer,p_global->audiobuffer[p_global->ptr_audio_read], global.numBytes);
	// move up marker
	p_global->ptr_audio_read=new_ptr_audio_read;
	return paContinue;
};

// STATE 1, senario 2: no more data -> set state to 0 and play silence
memset(outputBuffer,0,global.numBytes);
state=0;

return paContinue;

}; // end callback function portaudio write
