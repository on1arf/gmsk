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




void * funct_ptt_gpio (void * globalin) {
globaldatastr * pglobal;

pglobal=(globaldatastr *)globalin;

int ret;
char c;
int fails=0;
int unexpectedchar=0;

int oldstate=-1;
int newstate;

int fd;
char gpiofname[40];

// init  vars
pglobal->transmit=-1;
fails=0;
unexpectedchar=0;

snprintf(gpiofname,40,"/sys/class/gpio/gpio%d/value",pglobal->ptt);

if (pglobal->verbose >= 1) {
	fprintf(stderr,"gpio classfile = %s\n",gpiofname);
}; // end if


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
		pglobal->transmit = -1;
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
		if (pglobal->pttinvert) {
			newstate=0;
		} else {
			newstate=1;
		}; // end else - if
	} else if (c == '0') {
		if (pglobal->pttinvert) {
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

		oldstate=newstate;
	}; // end if

	pglobal->transmit=newstate;

	// sleep 50 ms
	usleep(50000);
}; // end endless loop

fprintf(stderr,"Error: main readgpio thread drops out of endless loop. Should not happen! \n");
exit(-1);

};

