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





int parsecliopts(int argc, char ** argv, char * retmsg) {

// local vars
int paramloop;

char * usage="Usage: gmsksender [-h] [-v] [-r] [-s] [-z] [-i] -f inputfilename.dvtool outputfilename.raw";
	
// init vars
global.fnamein=NULL;
global.fnameout=NULL;
global.verboselevel=0;
global.raw=0;


// usage:
// receiver [-v] [-r] [-z] [-s] [-i] -i filein fileout.wav
// known options

for (paramloop=1; paramloop <argc; paramloop++) {
	char * thisarg=argv[paramloop];

	if (strcmp(thisarg,"-f") == 0) {
		// -f: file IN

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			global.fnamein=argv[paramloop];
		}; // end if
	} else if (strcmp(thisarg,"-v") == 0) {
		// -v: verbose 
		global.verboselevel++;
	} else if (strcmp(thisarg,"-r") == 0) {
		// -r: raw data
		global.raw=1;
	} else if (strcmp(thisarg,"-s") == 0) {
		// -s: resync
		global.sync=1;
	} else if (strcmp(thisarg,"-z") == 0) {
		// -z: zap slowspeeddata 
		global.zap=1;
	} else if (strcmp(thisarg,"-i") == 0) {
		// -i: invert
		global.invert=1;
	} else if (strcmp(thisarg,"-h") == 0) {
		// -h: help
		snprintf(retmsg,RETMSGSIZE,"%s\n",usage);
		return(-1);
	} else {
		if (!(global.fnameout)) {
			global.fnameout=argv[paramloop];
		} else {
			snprintf(retmsg,RETMSGSIZE,"Error: To many arguments. %s\n",usage);
			return(-1);
		}; 
	}; // end else - elsif - if

}; // end for

// Done all parameters. Check if we have sufficient parameters

if (!(global.fnamein)) {
	snprintf(retmsg,RETMSGSIZE,"Error: input file missing.\n%s\n",usage);
	return(-1);
}; // end if

if (!(global.fnameout)){
	snprintf(retmsg,RETMSGSIZE,"Error: output file missing.\n%s\n",usage);
	return(-1);
}; // end if

// All done, return
return(0);

}; // end function parse cli
