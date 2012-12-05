/* find Port Audio Devices */

// Provides a list of all port-audio devices that can be used for 8K
// samples/sec capture or playback. This application is largly based
// on the "pa_devs.c" application provided in the "examples" directory
// of the portaudio distribution (author: Phil Burk http://www.softsynth.com)
// Original Copyright (c) 1999-2000 Ross Bencina and Phil Burk

// Rewritten by Kristoff Bonne to find valid audio-devices for
// the "audio capture / playback" tools part of the c2_gmskmodem project

// 2012-dec-05 Kristoff Bonne - ON1ARF

// for (f)printf
#include <stdio.h>

// for exit
#include <stdlib.h>

// for portaudio
#include <portaudio.h>

int main (int argc, char ** argv) {
int loop;
int numdevice;
int defaultinputdevice, defaultoutputdevice;
int samplerate;

int capabilities, fullduplex, maxnumchannel_input, maxnumchannel_output;

const PaDeviceInfo *devinfo;
PaStreamParameters PortaudioParameters;
PaError pa_ret;

char *capatext_in[]={ "INPUT NOT SUPPORTED" , "SAMPLERATE NOT SUPPORTED", "SUPPORTED: MONO ONLY", "SUPPORTED: STEREO ONLY", "SUPPORTED: MONO AND STEREO" };
char *capatext_out[]={ "OUTPUT NOT SUPPORTED" , "SAMPLERATE NOT SUPPORTED", "SUPPORTED: MONO ONLY", "SUPPORTED: STEREO ONLY", "SUPPORTED: MONO AND STEREO" };
char *capatext[]={ "NOT SUPPORTED", "SUPPORTED: MONO ONLY", "SUPPORTED: STEREO ONLY", "SUPPORTED: MONO AND STEREO" };


// read CLI parameters
if (argc < 2) {
	printf("Usage: %s samplerate\n\nSearches for audio-devices supporting that samplerate.\nOften supported samplerates are 48000 and 8000 samples/sec.\n\n",argv[0]);
	exit(0);
}; // end if

samplerate = atoi(argv[1]);

printf("INITIALISING PORTAUDIO ... \n");
// open portaudio device
pa_ret=Pa_Initialize();
printf(".... DONE\n");

if (pa_ret != paNoError) {
	Pa_Terminate();
	fprintf(stderr,"Error: Could not initialise Portaudio: %s(%d) \n",Pa_GetErrorText(pa_ret),pa_ret);
	exit(-1);
}; // end if

numdevice=Pa_GetDeviceCount();

if (numdevice < 0) {
	Pa_Terminate();
	fprintf(stderr,"Error: error in GetDeviceCount %s(%d) \n",Pa_GetErrorText(numdevice),numdevice);
	exit(-1);
} // # end if


if (numdevice == 0) {
	Pa_Terminate();
	printf("No portaudio devices found.\n");
	exit(-1);
}; // end if

printf("\nDevices portaudio devices supporting %d samples/sec rate:\n\n",samplerate);

// init vars
PortaudioParameters.sampleFormat = paInt16;
PortaudioParameters.hostApiSpecificStreamInfo = NULL;

defaultinputdevice=Pa_GetDefaultInputDevice();
defaultoutputdevice=Pa_GetDefaultOutputDevice();


for (loop=0; loop<numdevice; loop++) {
	capabilities=0;
	fullduplex=1;

	PortaudioParameters.device = loop;
	PortaudioParameters.suggestedLatency = 0; // ignored by Pa_IsFormatSupported anyway
	
	devinfo = Pa_GetDeviceInfo (loop);
	printf("Audio device %d: %s %s\n",loop,Pa_GetHostApiInfo(devinfo->hostApi)->name,devinfo->name);
	fflush(stdout);


// PART 1: INPUT
	maxnumchannel_input = devinfo->maxInputChannels;

	if (maxnumchannel_input >= 1) {
		// first try mono
		PortaudioParameters.channelCount = 1;

		pa_ret = Pa_IsFormatSupported(&PortaudioParameters,NULL,(double) samplerate);
		if (pa_ret == paFormatIsSupported) {
			capabilities += 1;
		}; // end if
	} else {
		// special clause: input not supported
		capabilities = -1;
		fullduplex=0;
	}; // end if


	if (maxnumchannel_input >= 2) {	
		// try again in stereo
		PortaudioParameters.channelCount = 2;

		pa_ret = Pa_IsFormatSupported(&PortaudioParameters,NULL,(double) samplerate);
		if (pa_ret == paFormatIsSupported) {
			capabilities += 2;
		}; // end if
	}; // end if
	
	if (loop == defaultinputdevice) {
		printf("*HALF DUPLEX INPUT : %s\n",capatext_in[capabilities+1]);
	} else {
		printf(" HALF DUPLEX INPUT : %s\n",capatext_in[capabilities+1]);
	}; // end else - if
	fflush(stdout);

	// no need to check full duplex if input not supported
	if (capabilities == 0) {
		fullduplex=0;
	}

// PART 2: OUTPUT
	// re-init
	capabilities=0;

	maxnumchannel_output = devinfo->maxOutputChannels;

	if (maxnumchannel_output >= 1) {
		// first try mono
		PortaudioParameters.channelCount = 1;

		pa_ret = Pa_IsFormatSupported(NULL,&PortaudioParameters,(double) samplerate);
		if (pa_ret == paFormatIsSupported) {
			capabilities += 1;
		}; // end if
	} else {
		// special case: output not supported
		capabilities = -1;
		fullduplex=0;
	}; // end if
	
	if (maxnumchannel_output >= 2) {
		// try again in stereo
		PortaudioParameters.channelCount = 2;

		pa_ret = Pa_IsFormatSupported(NULL,&PortaudioParameters,(double) samplerate);
		if (pa_ret == paFormatIsSupported) {
			capabilities += 2;
		}; // end if
	}; // end if
	
	if (loop == defaultoutputdevice) {
		printf("*HALF DUPLEX OUTPUT: %s\n",capatext_out[capabilities+1]);
	} else {
		printf(" HALF DUPLEX OUTPUT: %s\n",capatext_out[capabilities+1]);
	}; // end else - if

	// no need to check full duplex if input not supported
	if (capabilities == 0) {
		fullduplex=0;
	}

// PART 3: FULL DEPLUX: INPUT AND OUTPUT
	if (fullduplex) {
		// re-init
		capabilities=0;

		if ((maxnumchannel_output >= 1) && (maxnumchannel_input >= 1)) {
			// first try mono
			PortaudioParameters.channelCount = 1;

			pa_ret = Pa_IsFormatSupported(&PortaudioParameters,&PortaudioParameters,(double) samplerate);
			if (pa_ret == paFormatIsSupported) {
				capabilities += 1;
			}; // end if
		}; // end if
	
		if ((maxnumchannel_output >= 2) && (maxnumchannel_input >= 2)) {
			// try again in stereo
			PortaudioParameters.channelCount = 2;

			pa_ret = Pa_IsFormatSupported(&PortaudioParameters,&PortaudioParameters,(double) samplerate);
			if (pa_ret == paFormatIsSupported) {
				capabilities += 2;
			}; // end if
		}; // end if
	
		printf(" FULL DUPLEX       : %s\n\n",capatext[capabilities]);
	} else {
		//  full duplex not supported. Just add 1 line to output
		printf("\n");
	}; // end if (full duplex)

}; // end for

printf("(*) DEFAULT AUDIO DEVICE\n\n");

exit(0);

}; // end if
