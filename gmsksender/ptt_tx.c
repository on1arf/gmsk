#define FOREVER 1


// PTT TX:
// switch the PTT of a transceiver based on outgoing data on TX-pins
// To do this, a string with as much "0" as possible is send over the
// serial port: i.e. the hex string 0x00
// This translated to a voltage of +5V on RS232
// the serial port is set to 38.4 Kbps, 8N1 

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>


// include functions
#include "ptt_global.h"
#include "ptt_parsecliopts.h"


int main (int argc, char ** argv) {
char retmsg[RETMSGSIZE];

char serialoutbuffer[96];
// 100 ms of data at 38k4 kbps = 384 Octet
// send 4 times 96 octets

int fd_lock;
int fd_serial;
int ret;

int locked;
int lastlock=-1;
int action=0;


struct termios tio;

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

// Init data-structures
memset(serialoutbuffer,0x00,sizeof(serialoutbuffer));


// open files (lockfile and serialdevice)

fd_lock=open(global.lockfilename,O_WRONLY|O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

if (fd_lock== -1) {
	fprintf(stderr,"Error: open lockfile %s failed! Reason %d (%s)\n",global.lockfilename, errno, strerror(errno));
	exit(-1);
}; // end if

fd_serial=open(global.serialdevicename,O_RDWR);

if (fd_serial < 0) {
	fprintf(stderr,"Error: open serial device %s failed! Reason %d (%s)\n",global.serialdevicename, errno, strerror(errno));
	exit(-1);
}; // end if

// set serial port to 38K4 baud
ret=tcgetattr(fd_serial,&tio);
if (ret <0) {
	fprintf(stderr,"Error, tcgetattr on serial device %s failed! Reason %d (%s)\n",global.serialdevicename, errno, strerror(errno));
	exit(-1);
}; // end if

ret=cfsetospeed(&tio, B38400);
if (ret <0) {
	fprintf(stderr,"Error, cfsetospeed on serial device %s failed! Reason %d (%s)\n",global.serialdevicename, errno, strerror(errno));
	exit(-1);
}; // end if



// write attributes
ret=tcsetattr(fd_serial,TCSANOW,&tio);
if (ret <0) {
	fprintf(stderr,"Error, tcsetattr on serial device %s failed! Reason %d (%s)\n",global.serialdevicename, errno, strerror(errno));
	exit(-1);
}; // end if



if (global.verboselevel >= 2) {
	fprintf(stderr,"Starting lock loop.\n");
}; // end if

while (FOREVER) {

	if (global.verboselevel >= 2) {
		fprintf(stderr,"lock check.\n");
	}; // end if

	// check state of lock
	ret=lockf(fd_lock,F_TEST,0);

	if (ret == 0) {
		if (global.verboselevel >= 2) {
			fprintf(stderr,"Lock found.\n");
		}; // end if

		locked=0;
	} else if (ret == -1) {
		if (global.verboselevel >= 2) {
			fprintf(stderr,"No lock found.\n");
		}; // end if

		locked=1;
	} else {
		fprintf(stderr,"Error during lock-test: %d (%s)\n",errno,strerror(errno));
		locked = -1;
	}; // end if	

	if ((global.verboselevel >= 1) && (locked != lastlock)){
		fprintf(stderr,"Lock change: %d (was %d)\n",locked, lastlock);
	}; // end if


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


	if (action) {
		// write 4 time 96 octets; which is 384 octets and takes 100 ms at 38400 bps
		write(fd_serial,serialoutbuffer,sizeof(serialoutbuffer));
	} else {
		// sleep for 100 ms
		usleep(100000);
	}; // end else - if


	// save locked status
	lastlock=locked;


}; // end while

	
}; // end main
