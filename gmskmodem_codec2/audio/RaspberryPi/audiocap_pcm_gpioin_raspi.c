/* audio in */


#define RATE 8000
#define PERIOD 160

// ALSA
#include <alsa/asoundlib.h>

// needed for GPIO
#include <bcm2835.h>
#include <stdlib.h>

// posix threads + inter-process control
#include <pthread.h>


// needed for getppid
#include <sys/types.h>
#include <unistd.h>

#define FOREVER 1

//////////////////// CONFIGURATION: CHANGE BELOW 
// use pin 12 (output) to read button
#define PIN RPI_GPIO_P1_12

//////////////////// END
//////////////////// DO NOT CHANGE ANYTHING BELOW UNLESS YOU KNOW WHAT
//////////////////// YOU ARE DOING


// global data structure
typedef struct {
	int in;
	int readpipe;
	int writepipe;
} globaldatastr;

// global data itself
globaldatastr global;



// functions defined below
void * funct_readfromchild (void *);
void * do_child (globaldatastr *);


int main (int argc, char ** argv) {
int ret;
snd_pcm_hw_params_t *params;
snd_pcm_uframes_t frames;

char defaultcapturedevice[]="hw:CARD=system,DEV=0";
char *capturedevice;
int numchannels;
int val;
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

int pipefd[2];

pid_t pid;

uid_t myuid, myeuid; // uid and effective uid
gid_t mygid, myegid; // gid and effective gid

pthread_t thr_readfromchild;

// check UID levels.
// GPIO needs root priviledges; however to assure overall security it
// is a "bad idea" (c) to run the whole application as root (or as "sudo");
// Therefor, the application forces the implementation of the "setuid"
// method: the application needs to be owned by root and installed with
// the "setuid" bits set.
// This can be achieved as follows:
// chown root:root ./audioin_gpioin ; chmod 6755 ./audioin_gpioin
//
// The application will then, with exception of the actual code doing GPIO,
// drop its own prividges to non-root 
// As "setuid" applies to all threads, the GPIO-code that needs to run
// as root runs as a seperate process; not just a thread

myuid=getuid(); myeuid=geteuid();
mygid=getgid(); myegid=getegid();

if (myuid == 0) {
	fprintf(stderr,"Error: UID = Root: Please do not start this application as root: use setuid method: sudo chown root:root %s ; sudo chmod 6755 %s\n",argv[0],argv[0]);
	exit(-1);
}; // end if

if (mygid == 0) {
	fprintf(stderr,"Error: GID = Root; Please do not start this application as root: Use setuid method: sudo chown root:root %s ; sudo chmod 6755 %s\n",argv
[0],argv[0]);
	exit(-1);
}; // end if


if (myeuid != 0) {
	fprintf(stderr,"Error: EUID not ROOT: GPIO needs root priviledges. Please use setuid method: sudo chown root:root %s ; chmod 6755 %s\n",argv[0],argv[0]);
	exit(-1);
}; // end if

if (myegid != 0) {
	fprintf(stderr,"Error: EGID not ROOT: GPIO needs root priviledges. Please use setuid method: sudo chown root:root %s ; chmod 6755 %s\n",argv[0],argv[0]);
	exit(-1);
}; // end if


// init data
stereo=1;

// if 1st argument exists, use it as capture device
if (argc >= 2) {
	capturedevice = argv[1];
} else {
	// no argument given; use default
	capturedevice = defaultcapturedevice;
}; //

// create pipe for communication with child process
ret=pipe(pipefd);
// pipefd[0] = READ, pipefd[1] = WRITE

if (ret < 0) {
	fprintf(stderr,"Error in pipe: %d(%s)\n",errno,strerror(errno));
}; // end if

// copy write pipe to global data, so it can be used by subthread
global.readpipe=pipefd[0];
global.writepipe=pipefd[1];

// create and start child process
pid=fork();

if (!pid) {
	// child process

	// init gpio
	if (!bcm2835_init()) {
		fprintf(stderr,"bmc2825 failed! \n");
		exit(-1);
	}; // end if

	// set pin to be input
	bcm2835_gpio_fsel(PIN, BCM2835_GPIO_FSEL_INPT);

	while (FOREVER) {
		do_child(&global);
	}; // end while

	fprintf(stderr,"Error: child process dropped out of endless loop: should not happen!\n");
	exit(-1);
}; // end if


// continue main application.
// first drop priviledge levels

// drop priviledge level
ret=setuid(myuid);

if (ret < 0) {
	fprintf(stderr,"Error: setuid failed: %s\n",strerror(errno));
	exit(-1);
}; // end if

ret=setgid(mygid);
if (ret < 0) {
	fprintf(stderr,"Error: setgid failed: %s\n",strerror(errno));
	exit(-1);
}; // end if

// close unused write pipe (not used in parent process)
close (pipefd[1]);


// create subthread.
// this thread will receive input from the child process and
// pass that on to the main thread
pthread_create (&thr_readfromchild, NULL, funct_readfromchild, (void *) &global);


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


	// read gpio
	// data is actually read from child-process, communicated via a FIFO pipe
	// to a thread which communicates via global data to the main application
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

			int16_t * audiobuffer16;

			audiobuffer16=audiobuffer;

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

void * funct_readfromchild (void * globalin) {
globaldatastr * p_global;

p_global=(globaldatastr *)globalin;

int ret;
char c;
int failed=0;

// init  vars
p_global->in=-1;

while (FOREVER) {
	ret = read(p_global->readpipe,&c,1);

	if (ret) {
		if (c == 0x30) {
			// 0x30 = ascii '0'
			p_global->in=0;
		} else if (c == 0x31) {
			// 0x31 = ascii '1'
			p_global->in=1;
		} else {
			// error: unknown char
			fprintf(stderr,"Warning: parent received unknown char from child: %02X\n",c);
		}; // end else - elsif - if
		
		// sleep 50 ms
		usleep(50000);

		// reinit vars
		failed=0;
	} else {
		// error in read
		fprintf(stderr,"Error: parent did not receive any data from parent!\n");

		// signal state to main application
		p_global->in=-1;

		// sleep 1 second
		sleep(1);

		failed++;

		if (failed > 10) {
			fprintf(stderr,"Error: parent did not receive any data from parent during more then 10 tries. Exiting!\n");
			exit(-1);
		}; // end if
		
	}; // end if

}; // end while

};

// child process. Actual process running as ROOT

// Initialisation of bmc2835 and input pin already done in main application

void * do_child (globaldatastr * p_global) {
char c;

int oldstate=-1;
int newstate;
int repeat;
int ret;
int failed=0;


// endless loop
while (FOREVER) {
	// read pin from gpio


	// first check: check parent process id

	if (getppid() == 1) {
		// if this is 1, this means that the parent process has died; so the program should terminate
		fprintf(stderr,"Error: child process detects no more parent process: quiting! \n");
		exit(-1);
	}; // end if


	// init newstate
	newstate=bcm2835_gpio_lev(PIN);
//fprintf(stderr,"oldstate = %d, newstate = %d \n",oldstate,newstate);

	if (newstate != oldstate) {
		fprintf(stderr,"GPIO keypress: was %d now %d\n",oldstate,newstate);

		if (newstate) {
			c=0x31;	// ascii "1"
		} else {
			c=0x30;	// ascii "0"
		}; // end else - if

		ret=write(global.writepipe,&c,1);

		if (ret != 1) {
			fprintf(stderr,"Error: child write to FIFO failed: %s \n",strerror(errno));
			failed++;

			if (failed > 10) {
				// stop if we could not send anaything during 10 tries
				// perhaps child has died?

				fprintf(stderr,"Error: child write to FIFO failed after more then 10 times. Stopping!!!!  \n");
				exit(-1);

				// sleep 1 second
				sleep(1);
			}; // end else - if

		} else {
			// write was successfull

			oldstate=newstate;
			failed=0;

		}; // end if
	} else {
		// data has not changed
		// do not send data to parent

		// unless, 1 second ( 20 times 50 ms) has passed. Send anyhow as keep-alive

		failed++;

		if (failed >= 20) {
			//ret=write(p_global->writepipe,&c,1);
			ret=write(global.writepipe,&c,1);

			if (ret != 1) {
				failed++;

				if (failed > 10) {
					// stop if we could not send anaything during 10 tries
					// perhaps child has died?

					fprintf(stderr,"Error: child write to FIFO failed after more then 10 times. Stopping!!!!  \n");
					exit(-1);

				}; // end if
				// sleep 1 second
				sleep(1);

			} else {
				// write is successfull. Reset counters
				failed=0;
			}; // end if

		}; // end if
	}; // end if

	// sleep 50 ms
	usleep(50000);
	
}; // end endless loop
};
