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

char * usage="Usage: gmsksender [-h] [-v] [-r] [-ri] [-sb sec] [-se sec] [-s] [-z] [-i] [-lf PTTlockfile.lck] -f [inputfilename.dvtool | - ] [-a alsadevice | outputfilename.raw]";

char * help="Usage: gmsksender [-h] [-v] [-r] [-ri] [-sb sec] [-se sec] [-s] [-z] [-i] [-lf PTTlockfile.lck] -f [inputfilename.dvtool | - ] [-a alsadevice | outputfilename.raw]\n Options:\n -h: help (this text)\n -v: verbose\n -r: RAW file format (non D-STAR format)\n -ri: RAW file bitsequence invert (bits read/written from left (bit7) to right (bit0))\n -sb: length of silence at beginning of transmission (seconds)\n -se: length of silence at end of transmission (seconds)\n -s: resyncronize: overwrite 21-frame syncronisation pattern in slow-speed data with standard D-STAR pattern\n -z: Zap (delete) D-STAR slow-speed data information\n -i: invert audio (needed for certain ALSA devices)\n -lf: lockfile use to signal PTT switches\n -f: input file (use \"-\" to read from standard-in)\n -a: output to alsa device";

// init vars
global.fnamein=NULL;
global.infromstdin=0;
global.fnameout=NULL;
global.alsaname=NULL;
global.lockfile=NULL;
global.verboselevel=0;
global.raw=0;
global.rawinvert=0;
global.fileoralsa=-1;
global.silencebegin=DEFAULT_SILENCEBEGIN;
global.silenceend=DEFAULT_SILENCEEND;


// usage:

for (paramloop=1; paramloop <argc; paramloop++) {
	char * thisarg=argv[paramloop];

	if (strcmp(thisarg,"-f") == 0) {
		// -f: file IN

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			global.fnamein=argv[paramloop];

			if (strcmp(global.fnamein,"-") == 0) {
				global.infromstdin=1;
			}; // end if

		}; // end if
	} else if (strcmp(thisarg,"-v") == 0) {
		// -v: verbose 
		global.verboselevel++;
	} else if (strcmp(thisarg,"-r") == 0) {
		// -r: raw data
		global.raw=1;
	} else if (strcmp(thisarg,"-ri") == 0) {
		// -ri: raw invert
		global.rawinvert=1;
	} else if (strcmp(thisarg,"-s") == 0) {
		// -s: resync
		global.sync=1;
	} else if (strcmp(thisarg,"-sb") == 0) {
		// -sb: silence begin

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			global.silencebegin=atoi(argv[paramloop]);
		}; // end if
	} else if (strcmp(thisarg,"-se") == 0) {
		// -se: silence end

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			global.silenceend=atoi(argv[paramloop]);
		}; // end if
	} else if (strcmp(thisarg,"-lf") == 0) {
		// -lf: lock file

		if (global.lockfile) {
		// lockfile already defined
			snprintf(retmsg,RETMSGSIZE,"Error: PTTLockfile can only be defined once!\n%s\n",usage);
			return(-1);
		}; // end if

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			global.lockfile=argv[paramloop];
		}; // end if
	} else if (strcmp(thisarg,"-z") == 0) {
		// -z: zap slowspeeddata 
		global.zap=1;
	} else if (strcmp(thisarg,"-i") == 0) {
		// -i: invert
		global.invert=1;
	} else if (strcmp(thisarg,"-a") == 0) {
		// -a: alsadevice out

		// is there a next argument?
		if (paramloop+1 < argc) {
			global.fileoralsa=1;
			paramloop++;
			global.alsaname=argv[paramloop];
		} else {
			global.fileoralsa=-1;
			snprintf(retmsg,RETMSGSIZE,"Error: Missing argument.\n%s\n",usage);
			return(-1);
		}; // end if
	} else if (strcmp(thisarg,"-h") == 0) {
		// -h: help
		fprintf(stderr,"%s\n\n",help);
		exit(-1);
	} else {
		if (!(global.fnameout)) {
			if (global.fileoralsa==1) {
				snprintf(retmsg,RETMSGSIZE,"Error: File-out and alsa-out are mutually exclusive.\n%s\n",usage);
				return(-1);
			} else {
				global.fileoralsa=0;
				global.fnameout=argv[paramloop];
			}; // end else - if
		} else {
			snprintf(retmsg,RETMSGSIZE,"Error: To many arguments.\n%s\n",usage);
			return(-1);
		}; 
	}; // end else - elsif - if

}; // end for

// Done all parameters. Check if we have sufficient parameters

if (!(global.fnamein)) {
	snprintf(retmsg,RETMSGSIZE,"Error: input file missing.\n%s\n",usage);
	return(-1);
}; // end if

if (global.fileoralsa == -1){
	snprintf(retmsg,RETMSGSIZE,"Error: output file or alsa-device missing.\n%s\n",usage);
	return(-1);
}; // end if

if ((global.rawinvert) && (!(global.raw))) {
	snprintf(retmsg,RETMSGSIZE,"Warning: Raw-invert option without option \"raw\" does not make sence. Ignored!\n");
	return(1);
}; // end if

if ((global.fileoralsa == 0) && (global.lockfile)) {
	snprintf(retmsg,RETMSGSIZE,"Warning: PTT lockfile does not make sence when not using alsa-out. Ignored!\n");
	global.lockfile=NULL;
	return(1);
}; // end if

// All done, return
return(0);

}; // end function parse cli
