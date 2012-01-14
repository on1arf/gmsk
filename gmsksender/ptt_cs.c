#define FOREVER 1

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


// include functions
#include "ptt_global.h"
#include "ptt_parsecliopts.h"


int main (int argc, char ** argv) {
char retmsg[RETMSGSIZE];

int fd_lock=-1;
int fd_serial=-1;
int ret;


int locked;
int lastlock=-1;
int action=0;

int state;

const int flags_set= TIOCM_CTS | TIOCM_DSR | TIOCM_RTS | TIOCM_DTR;
const int flags_clear = ~flags_set;


// parse cli options
ret=parsecliopts(argc, argv, retmsg);

if (ret) {
	// ret != 0: error or warning
	fprintf(stderr,"%s",retmsg);

	// ret < 0: error -> exit
	if (ret <0 ) {
		exit(-1);
	};
}; // end if




// open files (lockfile and serialdevice)

fd_lock=open(global.lockfilename,O_RDONLY|O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
if (fd_lock < 0) {
	fprintf(stderr,"Error: open lockfile %s failed! Reason %d (%s)\n",global.lockfilename, errno, strerror(errno));
	exit(-1);
}; // end if




fd_serial=open(global.serialdevicename,O_RDWR);
if (fd_serial < 0) {
	fprintf(stderr,"Error: open serial device %s failed! Reason %d (%s)\n",global.serialdevicename, errno, strerror(errno));
	exit(-1);
}; // end if

// get current state, just to check if we can "ioctl" to the device
ret=ioctl(fd_serial,TIOCMGET, &state);

if (ret <0) {
	fprintf(stderr,"Error could not get state of serial device %s. Reason: %d(%s)\n",global.serialdevicename,errno,strerror(errno));
	exit(-1);
}; // end if

if (global.verboselevel >= 1) {
	fprintf(stderr,"Starting lock loop.\n");
}; // end if

while (FOREVER) {

	if (global.verboselevel >= 2) {
		fprintf(stderr,"Lock check.\n");
	}; // end if

	// check state of lock
	ret=lockf(fd_lock,F_TEST,0);

	if (ret == 0) {
		if (global.verboselevel >= 2) {
			fprintf(stderr,"Lock found.\n");
		}; // end if

		locked=1;

	} else if (ret== -1) {
		if (global.verboselevel >= 2) {
			fprintf(stderr,"No lock found.\n");
		}; // end if

		locked=0;
	} else {
		fprintf(stderr,"Error during lock-test: %d (%s)\n",errno, strerror(errno));
		locked=-1;
	}; // end if	

	if ((global.verboselevel >= 1) && (locked != lastlock)){
		fprintf(stderr,"Lock change: %d (was %d)\n",locked, lastlock);
	}; // end if

	// get current state
	ret=ioctl(fd_serial,TIOCMGET, &state);

	if (ret <0) {
		fprintf(stderr,"Error could not get state of serial device %s. Reason: %d(%s)\n",global.serialdevicename,errno,strerror(errno));
		exit(-1);
	}; // end if


	if (locked >= 0) {
		if (locked) {
			if (global.invert) {
				action=0;
			} else {
				action=1;
			}; // end else - if
		} else {
			if (global.invert) {
				action=1;
			} else {
				action=0;
			}; // end else - if
		}; // end else - if

		// NOTE, INVERSION!
		// A flag "SET" should be translated by default to a LOW signal on
		// RTS/CTS/DSR/RTS. As a "0" is translated to a +5 V on RS232
		if (action) {
			state &= flags_clear;
		} else {
			state |= flags_set;
		}; // end else - if
	}; // end if

	// set new state
	ret=ioctl(fd_serial,TIOCMSET, &state);
	if (ret < 0) {
		fprintf(stderr,"Error could not set state of serial device %s. Reason: %d(%s)\n",global.serialdevicename,errno,strerror(errno));
		exit(-1);
	}; // end if


	// save last lock status
	lastlock=locked;

	// sleep for 100 ms
	usleep(100000);

}; // end while

	
}; // end main
