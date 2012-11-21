/* audio in */



// Reads audio from the ALSA audio device
// If the GPIO pin is pushed, the audio is outputted as a raw PCM stream to stdout

// This application uses the /sys/class/gpio kernel-drivers to do GPIO
// The application assumes that the driver has been correctly initialised
// and configured

// E.g. on a raspberry pi, the initialion to read data from GPIO pin 18
// (i.e. the 12th (!!!) pin of the GPIO pins of the RPi) requires root
// priviledges and can be does as follows:

// # echo "18" > /sys/class/gpio/export
// (This will create a directory /sys/class/gpio/gpio18)
// # echo "in" > /sys/class/gpio/gpio18/direction
// # chmod 666 /sys/class/gpio/gpio18/value

// The status of the GPIO pin can be read as follows:
// $ cat /sys/class/gpio/gpio18/value
// Note, if the proper file-priviledges are set on
// /sys/class/gpio/gpioXX/value, reading or writing gpio data
// does not require root access


#define RATE 8000
#define PERIOD 160

// ALSA
#include <alsa/asoundlib.h>

// posix threads + inter-process control
#include <pthread.h>


#define FOREVER 1

//////////////////// CONFIGURATION: CHANGE BELOW 
// use pin 12 (input) to read button
// On the RPi, pin 12 on the GPIO connectors match /sys/class/gpio/gpio18 (18!!!, not 12)
//#define GPIOINFILE 18
//#define INVERT 0

// data for pandaboard, using pushbutton on board (note: info is inverted)
#define GPIOINFILE 121
#define INVERT 1

//////////////////// END
//////////////////// DO NOT CHANGE ANYTHING BELOW UNLESS YOU KNOW WHAT
//////////////////// YOU ARE DOING


// global data structure
typedef struct {
	int in;
} globaldatastr;

// global data itself
globaldatastr global;



// functions defined below
void * funct_readgpio (void *);


int main (int argc, char ** argv) {
int ret;
snd_pcm_hw_params_t *params;
snd_pcm_uframes_t frames;

char defaultcapturedevice[]="hw:CARD=system,DEV=0";
char *capturedevice;
int numchannels;
unsigned int val;
int dir;

snd_pcm_t *alsahandle;
int stereo;
int thisframes;
int16_t * audiobuffer;
int buffersize;
int inpbit;

// vars used for capturing audio and playout
int framesleft;
int16_t *p;
int framesreceived;
int thisbuffersize;
int state;
int gpioin;

pthread_t thr_readgpio;

// init data
stereo=1;

// if 1st argument exists, use it as capture device
if (argc >= 2) {
	capturedevice = argv[1];
} else {
	// no argument given; use default
	capturedevice = defaultcapturedevice;
}; //


// create subthread.
// this thread will read the GPIO status of the pins.
// this runs independantly from the main application so not to
// interfere with the strick timing of the ALSA process
pthread_create (&thr_readgpio, NULL, funct_readgpio, (void *) &global);


// continue main application

// open PCM device for capture 
ret=snd_pcm_open(&alsahandle, capturedevice, SND_PCM_STREAM_CAPTURE, 0);

if (ret < 0) {
	fprintf(stderr,"Error: unable to open alsa capture device: %s\n",snd_strerror(ret));
	exit(-1);
}; // end if


// Allocate a hardware parameters object
snd_pcm_hw_params_alloca(&params);

// fill in default values
ret=snd_pcm_hw_params_any(alsahandle, params);
if (ret < 0) {
	fprintf(stderr,"ALSA capture: Error in setting default values: %s\n",snd_strerror(ret));
	exit(-1);
}; // end if

// set the desired parameters
ret=snd_pcm_hw_params_set_access(alsahandle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
if (ret < 0) {
	fprintf(stderr,"ALSA capture: Error in setting access interleaved: %s \n",snd_strerror(ret));
	exit(-1);
}; // end if

// signed 16-bit little endian
ret=snd_pcm_hw_params_set_format(alsahandle, params, SND_PCM_FORMAT_S16_LE);

if (ret < 0) {
	fprintf(stderr,"ALSA capture: Error in setting format 16 bit signed: %s\n",snd_strerror(ret));
	exit(-1);
}; // end if

// number of channels 

if (stereo) {
	numchannels=2;
} else {
	numchannels=1;
}; // end if

ret=snd_pcm_hw_params_set_channels(alsahandle, params, numchannels);

if (ret < 0) {
	if (stereo) {
		fprintf(stderr,"ALSA capture: Error setting number of channels to %d: %s\n",numchannels, snd_strerror(ret));
		exit(-1);
	} else {
		// mono failed, try again in stereo;
		ret=snd_pcm_hw_params_set_channels(alsahandle, params, 2);

		if (ret < 0) {
			fprintf(stderr,"ALSA capture: Error setting number of channels. Tried both mono and sterero %s\n",snd_strerror(ret));
			exit(-1);
		} else {
			fprintf(stderr,"ALSA capture: setting mono capture failed. Switched to stereo. Audio is exptected on left channel.\n");
			stereo=1;
		}; // end else - if

	}; // end else - if
}; // end of

/* 48 Khz sampling rate */
val=RATE;
snd_pcm_hw_params_set_rate_near(alsahandle, params, &val, &dir);

/* Try to set the period size to 960 frames (20 ms at 48Khz sampling rate). */
frames = PERIOD;
snd_pcm_hw_params_set_period_size_near(alsahandle, params, &frames, &dir);

// set_period_size_near TRIES to set the period size to a certain
// value. However, there is no garantee that the audio-driver will accept
// this. So read the actual configured value from the device

// get actual periode size from driver for capture and playback
// capture
snd_pcm_hw_params_get_period_size(params, &frames, &dir);

thisframes = frames;


if (frames != PERIOD) {
	fprintf(stderr,"ALSA-CAPTURE: warningperiode could not be set: wanted %d, got %d \n",PERIOD,(int) thisframes);
}; // end if

/* Write the parameters to the driver */
ret = snd_pcm_hw_params(alsahandle, params);
if (ret < 0) {
	fprintf(stderr,"ALSA-CAPTURE: unable to set hw parameters: %s\n", snd_strerror(ret));
	return(-1);
}


if (stereo) {
	buffersize=thisframes<<2; // = * 4 (stereo and 16 bit/sample)
} else {
	// mono
	buffersize=thisframes<<1; // = * 2 (16 bit/sample)
}; // end else - if
audiobuffer=malloc(buffersize);

if (!audiobuffer) {
	fprintf(stderr,"Error: malloc audiobuffer: %s",strerror(errno));
	exit(-1);
}; // end if


// init vars
global.in=0; // set "gpioin" marker to 0
state=0;
gpioin=0;


// if stereo, actual buffersize send is half (only left channel)
if (stereo) {
	thisbuffersize=buffersize >> 1;
} else {
	thisbuffersize=buffersize;
}; // end else - if



while (FOREVER) {
// get audio


	framesleft=thisframes;

	p=audiobuffer;

	while (framesleft > 0) {
		framesreceived = snd_pcm_readi(alsahandle, p, framesleft);


		if (framesreceived == -EAGAIN) {
			// receiving EAGAIN mains no data
			usleep(250);
		} else {
			if (framesreceived < framesleft) {
				p += framesreceived;
			}; // end if
			framesleft -= framesreceived;
		}; // end if

	// keep on reading audio untill sufficient data read
	}; // end while


	// do we send out data ???


	// checo gpio inpuit
	// gpio is actually read from the readgpio thread
	// communication between the subthread and the main application is done via global
	inpbit=global.in;

	// inpbit can be 0 (key off), 1 (key on), -1 (undefined or not yet init).
	// we concider state "-1" to be same as "off".

	// if state is 1, stop after 1 second 
	if (state) {
		if (inpbit==1) {
			gpioin=50; // runs every 20 ms, so 1 second
		} else {
			gpioin--;

			// exit program when 0
			if (!gpioin) {
				exit(0);
			}; // end if
		}; // end else - if
	} else {
	// if state is 0, we need at least 500 ms of 1 for state to change
		if (inpbit==1) {
			gpioin++;

			// if 25 * 20 ms, move to state 1;
			if (gpioin >= 25) {
				state=1;
			}; // end if
		} else {
			// decrease if possible
			if (gpioin>0) {
				gpioin--;
			}; // end if
		}; // end else - if
	}; // end else - if

	if (state) {
		// State = 1: write audio

		// if stereo, only write left channel
		if (stereo) {
			int loop;

			int16_t *p1, *p2;

			// start at 2th element (in stereo format); which is 3th (in mono format)
			p1=&audiobuffer[1];
			p2=&audiobuffer[2];
	
			for (loop=1; loop < thisframes; loop++) {
				*p1=*p2;
				p1++; p2 += 2;
			}; // end for
		}; // end if

		fwrite(audiobuffer,thisbuffersize,1,stdout);
		fprintf(stderr,"T");
	}; // end if
}; // end while (endless loop)


fprintf(stderr,"Error: main application drops out of endless loop. Should not happen! \n");
exit(-1);

}; // end application


// functions (threads)

void * funct_readgpio (void * globalin) {
globaldatastr * p_global;

p_global=(globaldatastr *)globalin;

int ret;
char c;
int fails=0;
int unexpectedchar=0;

int oldstate=-1;
int newstate;

int fd;
char gpiofname[40];

// init  vars
p_global->in=-1;
fails=0;
unexpectedchar=0;

snprintf(gpiofname,40,"/sys/class/gpio/gpio%d/value",GPIOINFILE);

// open files
fd = open(gpiofname, O_RDONLY | O_NDELAY, 0);

if (fd < 0) {
	fprintf(stderr,"Error: Can't open %s for reading! \n",gpiofname);
	exit(-1);
}; // end if

// endless loop
while (FOREVER) {
	// read pin from gpio

	// init newstate
	lseek(fd,0,SEEK_SET); // move back to beginning of file
	ret=read(fd,&c,1);

	if (!ret) {
		// something went wrong! Set state to unknown, sleep 100 ms and retry
		p_global->in = -1;
		usleep(100000);

		// maximum 2 seconds (20 * 100 ms) of failures
		fails++;

		if (fails >= 20) {
			fprintf(stderr,"Error: Cannot read from gpio for more then 20 times. Exiting!!\n");
			exit(-1);
		}; // end if

		continue;
	}; // end

	// reinit vars
	fails=0;


	if (c == '1') {
		if (INVERT) {
			newstate=0;
		} else {
			newstate=1;
		}; // end else - if
	} else if (c == '0') {
		if (INVERT) {
			newstate=1;
		} else {
			newstate=0;
		}; // end else - if
	} else {
			fprintf(stderr,"Error: Read unexpected char from GPIO file: \"%c\" (%02X)\n",c,c);
			unexpectedchar++;

			if (unexpectedchar >= 100) {
				fprintf(stderr,"Error: Read unexpected char form more then 100 times. Exiting!!\n");
				exit(-1);
			}; // end if

			// try again
			continue;
	}; // end else - elsif - if

	// re-init var
	unexpectedchar=0;

	if (newstate != oldstate) {
		fprintf(stderr,"GPIO keypress: was %d now %d\n",oldstate,newstate);

		if (newstate) {
			p_global->in=1;
		} else {
			p_global->in=0;
		}; // end else - if

		oldstate=newstate;
	}; // end if

	// sleep 50 ms
	usleep(50000);
	
}; // end endless loop
};
