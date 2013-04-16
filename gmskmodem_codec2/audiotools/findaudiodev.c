/* find audio devices */

// find and display information about available audio devices
// decodes and plays it out


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


// version 20130415: initial release



//////////////////// CONFIGURATION: CHANGE BELOW 
// some fixed text
#define INPUT "INPUT"
#define OUTPUT "OUTPUT"

// size of return message
#define MSGSIZE 120


////  
// portaudio
#include <portaudio.h>

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

// for open and read
#include <sys/stat.h>
#include <fcntl.h>

// global data structure//
typedef struct {
	// devicename + "exact" are global for receive and transmit
	char * devicename; 
	int exact;

	// portaudio parameters for input and output
	PaStreamParameters outputParameters, inputParameters;

	int force_stereo_capt;
	int force_stereo_play;

	// other stuff
	int verbose;
} globaldatastr;

// global data itself //
globaldatastr global;


// functions defined below
int checkaudiodevice_one (PaStreamParameters * , int , char *, globaldatastr * );
void checkaudiodevice_find (globaldatastr *);

/////////////////////////////
// MAIN APPLICATION
//////////////
int main (int argc, char ** argv) {

// other vars
int loop;

PaError pa_ret;

// init data
global.inputParameters.channelCount=-1;
global.outputParameters.channelCount=-1;


// init some vars
global.force_stereo_capt=0;
global.force_stereo_play=0;
global.devicename=NULL;
global.exact=0;
global.verbose=0;

for (loop=1; loop < argc; loop++) {
	// part 1: options
	if (strcmp(argv[loop],"-v") == 0) {
		global.verbose++;
		continue; // go to next argument
	} else if (strcmp(argv[loop],"-forcestereo") == 0) {
		if (loop+1 < argc) {
			loop++;
			if ((strcmp(argv[loop],"c") == 0) || (strcmp(argv[loop],"C") == 0)) {
				global.force_stereo_play=0; global.force_stereo_capt=1;
			} else if ((strcmp(argv[loop],"p") == 0) || (strcmp(argv[loop],"P") == 0)) {
				global.force_stereo_play=1; global.force_stereo_capt=0;
			} else if ((strcmp(argv[loop],"b") == 0) || (strcmp(argv[loop],"B") == 0)) {
				global.force_stereo_play=1; global.force_stereo_capt=1;
			} else {
				fprintf(stderr,"Error: invalid parameter for forcestereo option: got \"%c\", expected \"c\", \"p\" or \"b\"!\n",argv[loop][0]);
				exit(-1);
			}; // end elsif - elsif - if
		}; // end if
		continue; // go to next argument
	} else if (strcmp(argv[loop],"-adevice") == 0) {
		if (loop+1 < argc) {
			loop++;
			global.devicename=argv[loop];
		}; // end if
		continue;
	} else if (strcmp(argv[loop],"-exact") == 0) {
		global.exact=1;
		continue;
	}; // end elsif - ... - if (options)
}; // end for

// We need at least 1 arguments: destination-ip destination-udpport listening-udpport
if (!global.devicename) {
	fprintf(stderr,"Error: audio devicename needed. \n");
	fprintf(stderr,"Usage: %s [ options ] -adevice <audiodevice> [-exact]\n",argv[0]);
	fprintf(stderr,"Note: use device \"\" to get list of devices.\n");
	fprintf(stderr," -forcestereo: Force stereo audio ('c' = capture, 'p' = playback, 'b' = both)\n");
	fprintf(stderr," -v          : increase verboselevel (repeat for more verbose)\n");
	fprintf(stderr,"\n");
	exit(-1);
}; // end if



/// INIT AUDIO
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

checkaudiodevice_find(&global);

exit(0);
}; // end main application

////////////////////////

/////// functions
// function checkaudiodevice_ont
// function checkaudiodevice_find


// check audio device 
int checkaudiodevice_one (PaStreamParameters * StreamParam, int inorout, char * retmsg, globaldatastr * pglobal) {

// local vars
const PaDeviceInfo *devinfo;
int maxnumchannel;
PaError pa_ret;

int forcestereo;
char *direction;



/// start of function

// sanity checks
if ((StreamParam->device < 0)  || (StreamParam->device >= Pa_GetDeviceCount())) {
	fprintf(stderr,"Internal Error: invalid devicenumber for checkaudiodevice_one: got %d, should be between 0 and %d\n", StreamParam->device, Pa_GetDeviceCount() -1 );
	exit(-1);
}; // end if

if ((inorout != 0) && (inorout != 1)) {
	fprintf(stderr,"Internal Error: invalid in-or-out for checkaudiodevice_one: got %d, should be 0 or 1\n",inorout);
	exit(-1);
}; // end if


// check if device supports samplerate:
StreamParam->sampleFormat = paInt16;
StreamParam->suggestedLatency = 0; // not used in Pa_IsFormatSupported
StreamParam->hostApiSpecificStreamInfo = NULL;

devinfo = Pa_GetDeviceInfo (StreamParam->device);

if (inorout) {
	direction=INPUT;
} else {
	direction=OUTPUT;
}; // end else - if

if (inorout) {
	maxnumchannel = devinfo->maxInputChannels;
} else {
	maxnumchannel = devinfo->maxOutputChannels;
}; // end else - if

if (maxnumchannel >= 1) {
	// first check if samplerate is supported in mono
	StreamParam->channelCount = 1;
	if (inorout) {
		pa_ret = Pa_IsFormatSupported(StreamParam,NULL,(double) 48000);
	} else {
		pa_ret = Pa_IsFormatSupported(NULL,StreamParam,(double) 48000);
	}; // end else - if


	forcestereo=0;
	if (inorout) {
	// input
		if (pglobal->force_stereo_capt) {
			forcestereo=1;
		}; // end if
	} else {
	// output
		if (pglobal->force_stereo_play) {
			forcestereo=1;
		}; // end if
	}; // end else - if

	if ((pa_ret == paFormatIsSupported) && (!forcestereo)){
		snprintf(retmsg,MSGSIZE,"%s: Samplerate 48000 samples/sec supported in mono.\n",direction);
		return(1);
	} else {
		// try again using stereo

		if (maxnumchannel >= 2) {
			StreamParam->channelCount = 2;
			if (inorout) {
				pa_ret = Pa_IsFormatSupported(StreamParam,NULL,(double) 48000);
			} else {
				pa_ret = Pa_IsFormatSupported(NULL,StreamParam,(double) 48000);
			}; // end else - if

			if (pa_ret == paFormatIsSupported) {
				snprintf(retmsg,MSGSIZE,"%s: Samplerate 48000 samples/sec supported in stereo.\n",direction);
				return(1);
			} else {
				snprintf(retmsg,MSGSIZE,"%s Error: Samplerate 48000 samples/sec not supported in mono or stereo!\n",direction);
				return(0);
			}; // end if
		} else {
			snprintf(retmsg,MSGSIZE,"%s Error: Samplerate 48000 samples/sec not supported in mono. Stereo not supported on this device!\n",direction);
			return(0);
		}; // end if
	}; // end else - if
} else {
	snprintf(retmsg,MSGSIZE,"Error: %s not supported on this device\n",direction);
	return(0);
}; // end else - if

// we should not come here. All clauses about should have returned
return(-1);
}; // end function "find_one"

//////////////////////////////////////
//////////////////////////////////////

void checkaudiodevice_find (globaldatastr * pglobal){

// local vars
const PaDeviceInfo *devinfo;

int loop;
int numdevice;

int numdevicefound=0;

char retmsg_in[MSGSIZE];
char retmsg_out[MSGSIZE];

numdevice=Pa_GetDeviceCount();
printf("Total number of devices found: %d\n\n",numdevice);

for (loop=0; loop<numdevice; loop++) {
	int devnamematch=0;

	devinfo = Pa_GetDeviceInfo(loop);

	// does devicename match searchstring ???
	if (strlen(devinfo->name) >= strlen(pglobal->devicename)) {
		// only check if searchstring is smaller or equal is size of device name
		int numcheck;
		int devnamesize;
		int loop;
		char *p;

		// init pointer to beginning of string
		p=(char *)devinfo->name;
		devnamesize = strlen(pglobal->devicename);

		if (pglobal->exact) {
			// exact match, only check once: at the beginning
			numcheck=1;
		} else {
			numcheck=strlen(p) - strlen(pglobal->devicename) +1;
		}; // end if

		// loop until text found or end-of-string
		for (loop=0; (loop<numcheck && devnamematch == 0); loop++) {
			if (strncmp(pglobal->devicename,p,devnamesize) ==0) {
				devnamematch=1;
			}; // end if

			// move up pointer
			p++;
		};
	}; // end if

	if (!devnamematch) {

		if ((pglobal->verbose >= 2)) {
			printf("Audio device %d (API:%s NAME:%s) ... devicename does NOT match! \n", loop,Pa_GetHostApiInfo(devinfo->hostApi)->name,devinfo->name);
		}; // end if

		continue;
		// go to next 
	}; // end if

	if (devnamematch) {
		int ok_in;
		int ok_out;

		// devicename matches searchstring
		printf("Audio device: %d (API:%s NAME:%s)\n",loop,Pa_GetHostApiInfo(devinfo->hostApi)->name,devinfo->name);

		pglobal->inputParameters.device=loop; pglobal->outputParameters.device=loop;

		// check audio device for input and output
		ok_in=checkaudiodevice_one(&pglobal->outputParameters,0,retmsg_in,pglobal);
		ok_out=checkaudiodevice_one(&pglobal->inputParameters,1,retmsg_out,pglobal);

		if ((ok_in) && (ok_out)) {
			// input and output check ok
			numdevicefound++;

			// print messages
			printf("%s",retmsg_in);
			printf("%s",retmsg_out);

			printf("Audio device matches all criteria\n");
		} else {
			// only print messages if verbose level 1 or higher
			if (pglobal->verbose >= 1) {
				printf("%s",retmsg_in);
				printf("%s",retmsg_out);

				printf("Audio device does NOT match all criteria\n");
			}; // end if

		}; // end if
		printf("\n");
	}; // end if

}; // end for

// checked all devices. How match are OK?

if (numdevicefound == 0) {
	// no device found
	printf("Warning: did not find any audio-device supporting that audio samplerate\n");
	printf("       Try again with other samplerate of devicename \"\" to get list of all devices\n");
	exit(-1);
} else if (numdevicefound > 1) {
	// more then one device found
	printf("Warning: Found %d devices matching devicename \"%s\" supporting 48000 bps samplerate\n",numdevicefound,pglobal->devicename);
	printf("       Try again with a more strict devicename or use the \"exact\" option!\n");
	exit(-1);
}; // end else - elsif - if (numdevicefound)


// else print out selected audio device
devinfo = Pa_GetDeviceInfo (pglobal->outputParameters.device);
printf("Selected Audio device = (API:%s NAME:%s)\n",Pa_GetHostApiInfo(devinfo->hostApi)->name,devinfo->name);

// done.
printf("\n");
fflush(stdout);

return;
}; // end function 
//////////////////////////////////////
//////////////////////////////////////
