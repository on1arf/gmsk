// support functions for portaudio, especially concerning find correct device

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




// some fixed text
#define INPUT "INPUT"
#define OUTPUT "OUTPUT"

// check audio device 
int checkaudiodevice_one (PaStreamParameters * StreamParam, int inorout, globaldatastr * pglobal) {

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
		printf("%s: Samplerate 48000 samples/sec supported in mono.\n",direction);
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
				printf("%s: Samplerate 48000 samples/sec supported in stereo.\n",direction);
				return(1);
			} else {
				printf("%s Error: Samplerate 48000 samples/sec not supported in mono or stereo!\n",direction);
				return(0);
			}; // end if
		} else {
			printf("%s Error: Samplerate 48000 samples/sec not supported in mono. Stereo not supported on this device!\n",direction);
			return(0);
		}; // end if
	}; // end else - if
} else {
	printf("Error: %s not supported on this device\n",direction);
	return(0);
}; // end else - if

// we should not come here. All clauses about should have returned
return(-1);
}; // end if

//////////////////////////////////////
//////////////////////////////////////

// checkaudiodevice DEFAULT
// no devicename give, so check default input and output devices
void checkaudiodevice_default (globaldatastr * pglobal){

// sanity checks
// no device name give, find default input and output device
pglobal->outputParameters.device=Pa_GetDefaultOutputDevice();

if (pglobal->outputParameters.device == paNoDevice) {
	fprintf(stderr,"Error: no portaudio default output device!\n");
	exit(-1);
}; // end if

if (pglobal->outputParameters.device >= Pa_GetDeviceCount()) {
	fprintf(stderr,"Internal Error: portaudio \"GetDefaultOutputDevice\" returns device number %d while total number of devices is %d \n,",pglobal->outputParameters.device, Pa_GetDeviceCount() );
	exit(-1);
}; // end if


// same check for input
pglobal->inputParameters.device=Pa_GetDefaultInputDevice();

if (pglobal->inputParameters.device == paNoDevice) {
	fprintf(stderr,"Error: no portaudio default input device!\n");
	exit(-1);
}; // end if

if (pglobal->inputParameters.device >= Pa_GetDeviceCount()) {
	fprintf(stderr,"Internal Error: portaudio \"GetDefaultInputDevice\" returns device number %d while total number of devices is %d \n,",pglobal->inputParameters.device, Pa_GetDeviceCount() );
	exit(-1);
}; // end if



// check audio device for input and output
if ((!checkaudiodevice_one(&pglobal->outputParameters,0,pglobal)) || (!checkaudiodevice_one(&pglobal->inputParameters,1,pglobal))) {
	// input or output check failed, exiting
	fprintf(stderr,"\nError: Default audio device does not match all criteria. Exiting!\n");
	exit(-1);
}; // end if

// done.
printf("\n");
fflush(stdout);

return;
}; // end function 
//////////////////////////////////////
//////////////////////////////////////

void checkaudiodevice_find (globaldatastr * pglobal){

// local vars
const PaDeviceInfo *devinfo;

int loop;
int numdevice;

int numdevicefound=0;
int devicenr=0;

int channelcount_c=-1, channelcount_p=-1;

numdevice=Pa_GetDeviceCount();
fprintf(stderr,"Total number of devices found: %d\n\n",numdevice);

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

	if (devnamematch) {
		// devicename matches searchstring
		fprintf(stderr,"Audio device: %d (API:%s NAME:%s)\n",loop,Pa_GetHostApiInfo(devinfo->hostApi)->name,devinfo->name);

		pglobal->inputParameters.device=loop; pglobal->outputParameters.device=loop;

		// check audio device for input and output
		if ((checkaudiodevice_one(&pglobal->outputParameters,0,pglobal)) && (checkaudiodevice_one(&pglobal->inputParameters,1,pglobal))) {
			// input and output check ok
			numdevicefound++;
			devicenr=loop;
			channelcount_c=pglobal->inputParameters.channelCount;
			channelcount_p=pglobal->outputParameters.channelCount;

			fprintf(stderr,"Audio device matches all criteria\n");
		} else {
			fprintf(stderr,"Audio device does NOT match all criteria\n");
		}; // end if
		fprintf(stderr,"\n");
	}; // end if

}; // end for

// checked all devices. How match are OK?

// if we just searched for "", just provide list and exit
if (pglobal->devicename[0] == 0) {
	exit(0);
};

if (numdevicefound == 0) {
	// no device found
	fprintf(stderr,"Error: did not find any audio-device supporting that audio samplerate\n");
	fprintf(stderr,"       Try again with other samplerate of devicename \"\" to get list of all devices\n");
	exit(-1);
} else if (numdevicefound > 1) {
	// more then one device found
	fprintf(stderr,"Error: Found %d devices matching devicename \"%s\" supporting 48000 bps samplerate\n",numdevicefound,pglobal->devicename);
	fprintf(stderr,"       Try again with a more strict devicename or use the \"exact\" option!\n");
	exit(-1);
} else {
	// just one found.
	pglobal->inputParameters.channelCount=channelcount_c;
	pglobal->outputParameters.channelCount=channelcount_p;

	pglobal->inputParameters.device=devicenr;
	pglobal->outputParameters.device=devicenr;

}; // end else - elsif - if (numdevicefound)



// else print out selected audio device
devinfo = Pa_GetDeviceInfo (pglobal->outputParameters.device);
fprintf(stderr,"Selected Audio device = (API:%s NAME:%s)\n",Pa_GetHostApiInfo(devinfo->hostApi)->name,devinfo->name);

// done.
printf("\n");
fflush(stdout);

return;
}; // end function 
//////////////////////////////////////
//////////////////////////////////////
