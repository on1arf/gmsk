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
// version 20120322 initial release


// These functions are started as a thread, and based on the global
// variable g_global.sending initiate an action:
// - set the DSR/DTS/CTS/RTS high
// - transmit all "0" over serial port (almost all "high")
// - set up a posix lock to a file

// /////////////////////////
// PTT LOCKFILE ////////////

void * funct_pttlockfile (void * c_globaldatain) {
c_globaldatastr *p_c_global;
g_globaldatastr *p_g_global;
s_globaldatastr *p_s_global;

p_c_global=(c_globaldatastr *) c_globaldatain;
p_s_global=p_c_global->p_s_global;
p_g_global=p_c_global->p_g_global;

int action;
int ret;
int lockfd=-1;
// open (create if needed) PTTlockfile.
// The sender application will set up a POSIX lock on that file
// when the PTT needs to be pushed
// That way, an external application can monitor this and
// do the actual hardware invention to switch the PTT


lockfd=open(s_global.pttlockfile, O_WRONLY|O_CREAT,  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

if (lockfd == -1) {
	fprintf(stderr,"Error: Lockfile %s could not be opened or created. Error %d (%s)\n",s_global.pttlockfile, errno, strerror(errno));
	exit(-1);
}; // end if


// endless loop
while (FOREVER) {
	// sleep 200 ms
	usleep(200000);

	if (p_g_global->sending) {
		action=1;
	} else {
		action=0;
	};

	if (p_s_global->ptt_invert) {
		// invert bit
		action ^= 1;
	}; // end if

	if (action) {
		ret=lockf(lockfd,F_TLOCK,0);

		if (ret < 0) {
			fprintf(stderr,"Error: PTT lock serial device %s failed! Reason %d (%s)\n",p_s_global->pttcsdevice,errno,strerror(errno));
			exit(-1);
		}; // end if
	} else {
		ret=lockf(lockfd,F_ULOCK,0);
		if (ret < 0) {
			fprintf(stderr,"Error: PTT lock serial device %s failed! Reason %d (%s)\n",p_s_global->pttcsdevice,errno,strerror(errno));
			exit(-1);
		}; // end if
	};

}; // end while

fprintf(stderr,"Dropped out of endless loop in funct_pttlockfile. Should not happen! \n");

}; // end function ptt lockfile



// /////////////////////////
// PTT CS //////////////////

void * funct_pttcs (void * c_globaldatain) {
c_globaldatastr *p_c_global;
g_globaldatastr *p_g_global;
s_globaldatastr *p_s_global;

p_c_global=(c_globaldatastr *) c_globaldatain;
p_s_global=p_c_global->p_s_global;
p_g_global=p_c_global->p_g_global;

int fd_serial=-1;
int ret;
int action;

int state;

const int flags_set= TIOCM_CTS | TIOCM_DSR | TIOCM_RTS | TIOCM_DTR;
const int flags_clear = ~flags_set;


fd_serial=open(p_s_global->pttcsdevice,O_RDWR);

if (fd_serial<0) {
	fprintf(stderr,"Error: open serial device %s failed! Reason %d (%s)\n",p_s_global->pttcsdevice,errno,strerror(errno));
	exit(-1);
}; // end if

// get current state, just to check if we can "ioctl" to the device
ret=ioctl(fd_serial,TIOCMGET, &state);

if (ret <0) {
	fprintf(stderr,"Error could not get state of serial device %s. Reason: %d(%s)\n",p_s_global->pttcsdevice,errno,strerror(errno));
	exit(-1);
}; // end if


while (FOREVER) {
	// sleep 200 ms
	usleep(200000);

	if (p_g_global->sending) {
		action=1;
	} else {
		action=0;
	};

	if (p_s_global->ptt_invert) {
		// invert bit
		action ^= 1;
	}; // end if


	// get current state
	ret=ioctl(fd_serial,TIOCMGET, &state);

	if (ret <0) {
		fprintf(stderr,"Error could not get state of serial device %s. Reason: %d(%s)\n",p_s_global->pttcsdevice,errno,strerror(errno));
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
		fprintf(stderr,"Error could not set state of serial device %s. Reason: %d(%s)\n",p_s_global->pttcsdevice,errno,strerror(errno));
		exit(-1);
	}; // end if

}; // end while

fprintf(stderr,"Dropped out of endless loop in funct_pttcs. Should not happen! \n");
}; // end function ptt control signals


// /////////////////////////
// PTT TX //////////////////

void * funct_ptttx (void * c_globaldatain) {
c_globaldatastr *p_c_global;
g_globaldatastr *p_g_global;
s_globaldatastr *p_s_global;

p_c_global=(c_globaldatastr *) c_globaldatain;
p_s_global=p_c_global->p_s_global;
p_g_global=p_c_global->p_g_global;

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

fd_serial=open(p_s_global->ptttxdevice,O_RDWR);

if (fd_serial < 0) {
	fprintf(stderr,"Error: open serial device %s failed! Reason %d (%s)\n",p_s_global->ptttxdevice, errno, strerror(errno));
	exit(-1);
}; // end if

// set serial port to 38K4 baud
ret=tcgetattr(fd_serial,&tio);
if (ret <0) {
	fprintf(stderr,"Error, tcgetattr on serial device %s failed! Reason %d (%s)\n",p_s_global->ptttxdevice, errno, strerror(errno));
	exit(-1);
}; // end if

ret=cfsetospeed(&tio, B38400);
if (ret <0) {
	fprintf(stderr,"Error, cfsetospeed on serial device %s failed! Reason %d (%s)\n",p_s_global->ptttxdevice, errno, strerror(errno));
	exit(-1);
}; // end if



// write attributes
ret=tcsetattr(fd_serial,TCSANOW,&tio);
if (ret <0) {
	fprintf(stderr,"Error, tcsetattr on serial device %s failed! Reason %d (%s)\n",p_s_global->ptttxdevice, errno, strerror(errno));
	exit(-1);
}; // end if

while (FOREVER) {
	if (p_g_global->sending) {
		action=1;
	} else {
		action=0;
	};

	if (p_s_global->ptt_invert) {
		// invert bit
		action ^= 1;
	}; // end if

	if (action) {
		// write 4 time 96 octets; which is 384 octets and takes 100 ms at 38400 bps
		for (loop=0; loop <= 3; loop++) {
			ret=write(fd_serial,serialoutbuffer,sizeof(serialoutbuffer));
			if (!ret) {
				fprintf(stderr,"Error, PTT write failed for serial device %s failed! Reason %d (%s)\n",p_s_global->ptttxdevice, errno, strerror(errno));
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
