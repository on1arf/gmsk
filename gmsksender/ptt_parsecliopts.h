/* parsecli.h */

// This code is part of the "gmsksender" application

/*
 *      Copyright (C) 2011 by Kristoff Bonne, ON1ARF
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


// Release information:
// version 20111213: initial release
// version 20120105: alsa devices, PTT lockfile, raw file format and input from stdin


int parsecliopts(int argc, char ** argv, char * retmsg) {

// local vars
int paramloop;

char * usage="Usage: %s [-h] [-v] [-i] [-lf PTTlockfile.lck] [-d serialdevice]";

char * help="Usage: %s [-h] [-v] [-i] [-lf PTTlockfile.lck] [-d serialdevice]\n Options:\n -h: help (this text)\n -v: verbose\n -i: invert\n -lf: lock file\n -d: serial device";

// init vars
global.verboselevel=0;
global.invert=0;
global.lockfilename=NULL;
global.serialdevicename=NULL;

// check parameters
for (paramloop=1; paramloop <argc; paramloop++) {
	char * thisarg=argv[paramloop];

	if (strcmp(thisarg,"-v") == 0) {
		// -v: verbose 
		global.verboselevel++;
	} else if (strcmp(thisarg,"-i") == 0) {
		// -i: invert 
		global.invert=1;
	} else if (strcmp(thisarg,"-lf") == 0) {
		// -lf: lock file

		if (global.lockfilename) {
		// lockfile already defined
			snprintf(retmsg,RETMSGSIZE,"Error: PTTLockfile can only be defined once!\n%s\n",usage);
			return(-1);
		}; // end if

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			global.lockfilename=argv[paramloop];
		}; // end if
	} else if (strcmp(thisarg,"-d") == 0) {
		// -d: device (serial device)

		if (global.serialdevicename) {
		// lockfile already defined
			snprintf(retmsg,RETMSGSIZE,"Error: Serialdevice name can only be defined once!\n%s\n",usage);
			return(-1);
		}; // end if

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			global.serialdevicename=argv[paramloop];
		}; // end if

	} else if (strcmp(thisarg,"-h") == 0) {
		// -h: help
		fprintf(stderr,"%s\n\n",help);
		exit(-1);
	} else {
		snprintf(retmsg,RETMSGSIZE,"Error: To many arguments.\n%s\n",usage);
		return(-1);
	}; // end else - elsif - if

}; // end for

// Done all parameters. Check if we have sufficient parameters

if (!(global.lockfilename)) {
	snprintf(retmsg,RETMSGSIZE,"Error: lockfile name missing.\n%s\n",usage);
	return(-1);
}; // end if

if (!(global.serialdevicename)) {
	snprintf(retmsg,RETMSGSIZE,"Error: serial device name missing.\n%s\n",usage);
	return(-1);
}; // end if


// All done, return
return(0);

}; // end function parse cli
