// PTT LOCKING FUNCTIONS



/*
 *      Copyright (C) 2012 by Kristoff Bonne, ON1ARF
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


// Release information
// version 20130326 initial release
// version 20130404: added half duplex support



// These functions are started as a thread, and based on the global
// variable g_global.transmitptt initiate an action:
// - set the DSR/DTS/CTS/RTS high
// - transmit all "0" over serial port (almost all "high")
// - set up a posix lock to a file
// - drive a GPIO pin


// /////////////////////////
// PTT LOCKFILE ////////////

void * funct_pttlockfile (void * globaldatain) {
struct globaldata *pglobal;
pglobal=(struct globaldata *) globaldatain;

int action;
int ret;
int lockfd=-1;
// open (create if needed) PTTlockfile.
// The sender application will set up a POSIX lock on that file
// when the PTT needs to be pushed
// That way, an external application can monitor this and
// do the actual hardware invention to switch the PTT


lockfd=open(pglobal->pttlockfile, O_WRONLY|O_CREAT,  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

if (lockfd == -1) {
	fprintf(stderr,"Error: Lockfile %s could not be opened or created. Error %d (%s)\n",pglobal->pttlockfile, errno, strerror(errno));
	exit(-1);
}; // end if


// endless loop
while (FOREVER) {
	// sleep 200 ms
	usleep(200000);

	if (pglobal->transmitptt) {
		action=1;
	} else {
		action=0;
	};

	if (pglobal->ptt_invert) {
		// invert bit
		action ^= 1;
	}; // end if

	if (action) {
		ret=lockf(lockfd,F_TLOCK,0);

		if (ret < 0) {
			fprintf(stderr,"Error: PTT lock serial device %s failed! Reason %d (%s)\n",pglobal->pttcsdevice,errno,strerror(errno));
			exit(-1);
		}; // end if
	} else {
		ret=lockf(lockfd,F_ULOCK,0);
		if (ret < 0) {
			fprintf(stderr,"Error: PTT lock serial device %s failed! Reason %d (%s)\n",pglobal->pttcsdevice,errno,strerror(errno));
			exit(-1);
		}; // end if
	};

}; // end while

fprintf(stderr,"Dropped out of endless loop in funct_pttlockfile. Should not happen! \n");

}; // end function ptt lockfile



// /////////////////////////
// PTT CS //////////////////

void * funct_pttcs (void * globaldatain) {
struct globaldata * pglobal;

pglobal=(struct globaldata *) globaldatain;

int fd_serial=-1;
int ret;
int action;

int state;

const int flags_set= TIOCM_CTS | TIOCM_DSR | TIOCM_RTS | TIOCM_DTR;
const int flags_clear = ~flags_set;


fd_serial=open(pglobal->pttcsdevice,O_RDWR);

if (fd_serial<0) {
	fprintf(stderr,"Error: open serial device %s failed! Reason %d (%s)\n",pglobal->pttcsdevice,errno,strerror(errno));
	exit(-1);
}; // end if

// get current state, just to check if we can "ioctl" to the device
ret=ioctl(fd_serial,TIOCMGET, &state);

if (ret <0) {
	fprintf(stderr,"Error could not get state of serial device %s. Reason: %d(%s)\n",pglobal->pttcsdevice,errno,strerror(errno));
	exit(-1);
}; // end if


while (FOREVER) {
	// sleep 200 ms
	usleep(200000);

	if (pglobal->transmitptt) {
		action=1;
	} else {
		action=0;
	};

	if (pglobal->ptt_invert) {
		// invert bit
		action ^= 1;
	}; // end if


	// get current state
	ret=ioctl(fd_serial,TIOCMGET, &state);

	if (ret <0) {
		fprintf(stderr,"Error could not get state of serial device %s. Reason: %d(%s)\n",pglobal->pttcsdevice,errno,strerror(errno));
		exit(-1);
	}; // end if

	
	// note: INVERSION!
	// a flag "set" should be translated by default to a LOW signal

	if (action) {
		state &= flags_clear;
	} else {
		state |= flags_set;
	}; // end else - if

	// set new state
	ret=ioctl(fd_serial,TIOCMSET, &state);
	if (ret < 0) {
		fprintf(stderr,"Error could not set state of serial device %s. Reason: %d(%s)\n",pglobal->pttcsdevice,errno,strerror(errno));
		exit(-1);
	}; // end if

}; // end while

fprintf(stderr,"Dropped out of endless loop in funct_pttcs. Should not happen! \n");
}; // end function ptt control signals


// /////////////////////////
// PTT TX //////////////////

void * funct_ptttx (void * globaldatain) {
struct globaldata * pglobal;

pglobal=(struct globaldata  *) globaldatain;

int fd_serial;
int action;
int ret;
int loop;

struct termios tio;

char serialoutbuffer[96];
// 100 ms of data at 38k4 = 384 octets
// send 4 times 96 octets 


// init memory
memset(serialoutbuffer,0x00,sizeof(serialoutbuffer));

fd_serial=open(pglobal->ptttxdevice,O_RDWR);

if (fd_serial < 0) {
	fprintf(stderr,"Error: open serial device %s failed! Reason %d (%s)\n",pglobal->ptttxdevice, errno, strerror(errno));
	exit(-1);
}; // end if

// set serial port to 38K4 baud
ret=tcgetattr(fd_serial,&tio);
if (ret <0) {
	fprintf(stderr,"Error, tcgetattr on serial device %s failed! Reason %d (%s)\n",pglobal->ptttxdevice, errno, strerror(errno));
	exit(-1);
}; // end if

ret=cfsetospeed(&tio, B38400);
if (ret <0) {
	fprintf(stderr,"Error, cfsetospeed on serial device %s failed! Reason %d (%s)\n",pglobal->ptttxdevice, errno, strerror(errno));
	exit(-1);
}; // end if



// write attributes
ret=tcsetattr(fd_serial,TCSANOW,&tio);
if (ret <0) {
	fprintf(stderr,"Error, tcsetattr on serial device %s failed! Reason %d (%s)\n",pglobal->ptttxdevice, errno, strerror(errno));
	exit(-1);
}; // end if

while (FOREVER) {
	if (pglobal->transmitptt) {
		action=1;
	} else {
		action=0;
	};

	if (pglobal->ptt_invert) {
		// invert bit
		action ^= 1;
	}; // end if

	if (action) {
		// write 4 time 96 octets; which is 384 octets and takes 100 ms at 38400 bps
		for (loop=0; loop <= 3; loop++) {
			ret=write(fd_serial,serialoutbuffer,sizeof(serialoutbuffer));
			if (!ret) {
				fprintf(stderr,"Error, PTT write failed for serial device %s failed! Reason %d (%s)\n",pglobal->ptttxdevice, errno, strerror(errno));
				exit(-1);
			}; // end if
		}; // end for
	} else {
		usleep(100000);
		// sleep for 100 ms
	}; // end if

}; // end while

fprintf(stderr,"Dropped out of endless loop in funct_pttcs. Should not happen! \n");
}; // end function ptt control transmit


// /////////////////////////
// GPIO   //////////////////

// This application uses the /sys/class/gpio kernel-drivers to do GPIO
// operations. The application assumes that the kernel-driver
// filestructure has been correctly initialised and configured

// E.g. on a raspberry pi, the initialion to read data from GPIO pin 17
// (i.e. the 11th (!!!) pin of the GPIO pins of the RPi) requires root
// priviledges and can be does as follows:

// # echo "17" > /sys/class/gpio/export
// (This will create a directory /sys/class/gpio/gpio17)
// # echo "out" > /sys/class/gpio/gpio17/direction
// # chmod 666 /sys/class/gpio/gpio17/value

// Note, if the proper file-priviledges are set on
// /sys/class/gpio/gpioXX/value, reading or writing gpio data
// does not require root access

// The status of the GPIO pin can be set as follows:
// $ echo "0" > /sys/class/gpio/gpio17/value
// $ echo "1" > /sys/class/gpio/gpio17/value
void * funct_pttgpio (void * globaldatain) {
struct globaldata * pglobal;

pglobal=(struct globaldata *) globaldatain;

char gpiofname[40];

int fd;
int action;
int ret;

int tries;

char on = '1';
char off = '0';
// open file for GPIO
snprintf(gpiofname,40,"/sys/class/gpio/gpio%d/value",pglobal->ptt_gpiopin);

fd = open (gpiofname, O_WRONLY | O_NDELAY, 0);

if (fd < 0) {
        fprintf(stderr,"Error: cannot open GPIO device %s for writing: %d (%s)!\n",gpiofname, errno, strerror(errno));
        exit(-1);
}; // end if


// init GPIO, also check if GPIO works or not
tries=0;
while (tries < 10) {
	tries++;

	if (PTTGPIO_INITSTATE) {
		action=1;
	} else {
		action=0;
	}; // end else - if

	if (pglobal->ptt_invert) {
		// invert bit
		action ^= 1;
	}; // end if

	if (action) {
		lseek(fd,0,SEEK_SET); // move back to beginning of file
		ret=write(fd,&on,1);
	} else {
		lseek(fd,0,SEEK_SET); // move back to beginning of file
		ret=write(fd,&off,1);
	}; // end else - if

	// success ???
	if (!ret) {
		// something went wrong
		fprintf(stderr,"Error PTT_GPIO: write to GPIO file %s failed: %s! \n",gpiofname,strerror(errno));

		// try up to 10 times
		if (tries < 10) {
			// sleep 100 ms, then try again
			usleep(100000);
			continue;
		} else {
			fprintf(stderr,"Error PTT_GPIO: write to GPIO file failed more then 10 times. Exiting!\n");
			exit(-1);
		}; // end if
	} else {
		// success!, drop out of loop
		tries=10; 
	}; // end if
}; // end if

while (FOREVER) {
	if (pglobal->transmitptt) {
		action=1;
	} else {
		action=0;
	};

	if (pglobal->ptt_invert) {
		// invert bit
		action ^= 1;
	}; // end if

	if (action) {
		// Turn it on
		lseek(fd,0,SEEK_SET); // move back to beginning of file
		ret=write(fd,&on,1);
	} else {
		// Turn it off
		lseek(fd,0,SEEK_SET); // move back to beginning of file
		ret=write(fd,&off,1);
	}; // end else - if

	// some error checking
	if (!ret) {
		// something went wrong
		fprintf(stderr,"Error PTT_GPIO: write to GPIO file %s failed. Reason %d (%s)! \n",gpiofname,errno,strerror(errno));

		// sleep 100 ms, then try again
		usleep(100000);
		continue;
	}; // end if

	// sleep 100 ms
	usleep(100000);

}; // end while


fprintf(stderr,"Dropped out of endless loop in funct_pttgpio. Should not happen! \n");
}; // end function ptt control transmit

