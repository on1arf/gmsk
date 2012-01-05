/* parsecli.h */


// version 20111107: initial release
// version 20120105: raw files, out to stdout + small changes names of vars

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


int parsecliopts(int argc, char ** argv, char * retmsg) {

char * usage="Usage: receiver [-h] [-v] [-4 | -6] [-r] [-rs hex] [-rss size] [-ri] [-ai] [-d] [-dd] [-da] [-s] [-m] {-a alsadevice | -f inputfilename} {outputfilename | -u udphost udpport}\n";
char * help="Usage: receiver [-h] [-v] [-4 | -6] [-r] [-rs hex] [-rss size] [-ri] [-ai] [-d] [-dd] [-da] [-s] [-m] {-a alsadevice | -f inputfilename} {outputfilename | -u udphost udpport}\n\n Options:\n -h: help (this text)\n -v: verbose\n \n -r: enable raw-mode (NON D-STAR format)\n -rs: RAW-mode frame syncronisation pattern (default: 0x7650, as used by D-STAR)\n -rss: RAW-mode frame syncronisation pattern size (default: 15 bits, as used by D-STAR)\n -ri: RAW-mode bitsequence invert (bits read/written from left (bit7) to right (bit0))\n -m: add begin and end MARKERS to raw data output\n \n -d: dump stream data\n -dd: dump more stream data\n -da: dump average audio-level data\n \n -s: stereo: input alsa-device is stereo\n -ai: input audio inverted (needed for certain types of ALSA devices)\n \n -f: output filename (use \"-\"- for stdout)\n -u: stream out received data over UDP (port + udp port needed)\n -4: UDP host hostname lookup ipv4 only\n -6: UDP host hostname lookup ipv6 only\n";

// local vars
int paramloop;

int rawsyncset=0;
int rawsyncsizeset=0;

// temp data
int rawsyncin;


// init vars
global.fileorcapture=-1;
global.fnameout=NULL;
global.outtostdout=0;
global.dumpstream=0;
global.dumpaverage=0;
global.audioinvert=0;
global.stereo=0;
global.udpport=0;
global.udphost=NULL;
global.verboselevel=0;
global.ipv4only=0;
global.ipv6only=0;
global.sendmarker=0;
global.raw=0;
global.rawsync=0x7650; // raw sync, frame syncronisation size. Default of 0x7650, as used by D-STAR
global.rawsyncsize=15; // number of bits in frame syn size, default is 15, as used by D-STAR 
global.rawinvert=0;


// usage:
// receiver [-d] [-dd] [-da] [-i] [-s] {-a alsadevice | -f filein} { fileout | -u udphost:udpport }
// known options

for (paramloop=1; paramloop <argc; paramloop++) {
	char * thisarg=argv[paramloop];

	if (strcmp(thisarg,"-a") == 0) {
		// -a: alsa device

		if (global.fileorcapture != -1) {
			snprintf(retmsg,RETMSGSIZE,"Error: -a and -f are mutually exclusive\n");
			return(-1);
		}; // end if

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			global.capturedevice=argv[paramloop];
			global.fileorcapture=0;
		}; // end if
	} else if (strcmp(thisarg,"-u") == 0) {
		// -u udphost:udpport


		// is there a next argument?
		if (paramloop+2 < argc ) {
			paramloop++;
			
			global.udphost=argv[paramloop];

			paramloop++;
			global.udpport=atoi(argv[paramloop]);

			if ((global.udpport <= 0) || (global.udpport > 65535)) {
				snprintf(retmsg,RETMSGSIZE,"Error: portnumber must be between 1 and 65535\n");
				return(-1);
			}; // end if

		} else {
			snprintf(retmsg,RETMSGSIZE,"Error: -u needs two additional arguments: host and port\n");
			return(-1);
		}; // end if
	} else if (strcmp(thisarg,"-f") == 0) {
		// -f: file name

		if (global.fileorcapture != -1) {
			snprintf(retmsg,RETMSGSIZE,"Error: -a and -f are mutually exclusive\n");
			return(-1);
		}; // end if
 
		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			global.fnamein=argv[paramloop];
			global.fileorcapture=1;
		}; // end if
	} else if (strcmp(thisarg,"-d") == 0) {
		// -d: dump stream data
		if (! global.dumpstream) {
			global.dumpstream=1;
		}; // end if
	} else if (strcmp(thisarg,"-dd") == 0) {
		// -dd: dump more data
		global.dumpstream=2;
	} else if (strcmp(thisarg,"-da") == 0) {
		// -da: dump average audio level data
		global.dumpaverage=1;
		if (! global.dumpstream) {
			global.dumpstream=1;
		}; // end if
	} else if (strcmp(thisarg,"-m") == 0) {
		// -m: send begin/end markers
		global.sendmarker=1;
	} else if (strcmp(thisarg,"-r") == 0) {
		// -r: raw
		global.raw=1;
	} else if (strcmp(thisarg,"-rs") == 0) {
		// -rs: raw sync pattern
		if (paramloop+1 < argc) {
			paramloop++;
			rawsyncin=atoi(argv[paramloop]);
			rawsyncset=1;
		}; // end if
	} else if (strcmp(thisarg,"-rss") == 0) {
		// -rs: raw sync pattern
		if (paramloop+1 < argc) {
			paramloop++;
			global.rawsyncsize=atoi(argv[paramloop]);
			rawsyncsizeset=1;
		}; // end if
	} else if (strcmp(thisarg,"-ri") == 0) {
		// -ri: raw invert
		global.rawinvert=1;
	} else if (strcmp(thisarg,"-ai") == 0) {
		// -ai: audio invert
		global.audioinvert=1;
	} else if (strcmp(thisarg,"-s") == 0) {
		// -s: stereo
		global.stereo=1;
	} else if (strcmp(thisarg,"-v") == 0) {
		// -v: verbose
		global.verboselevel++;
	} else if (strcmp(thisarg,"-h") == 0) {
		// -h: help
		fprintf(stderr,"%s\n\n",help);
		exit(-1);
	} else if (strcmp(thisarg,"-4") == 0) {
		// -4: ipv4 only
		global.ipv4only=1;
	} else if (strcmp(thisarg,"-6") == 0) {
		// -6: ipv6 only
		global.ipv6only=1;
	} else {
		// last option: output file name

		// filename may only be defined once
		if (global.fnameout) {
			snprintf(retmsg,RETMSGSIZE,"Error: only one output filename allowed\n");
			return(-1);
		}; // end if

		global.fnameout=argv[paramloop];
		
		if (strcmp(global.fnameout,"-") == 0) {
			global.outtostdout=1;
		}; // end if

	}; // end else - elsif - if

}; // end for

// Done all parameters. Check if we have sufficient parameters

if (global.fileorcapture == -1) {
	snprintf(retmsg,RETMSGSIZE,"Error: input source missing.\n%s\n",usage);
	return(-1);
}; // end if

if ((global.ipv4only) && (global.ipv6only)){
	snprintf(retmsg,RETMSGSIZE,"Error: IPv4-only and IPv6-only are mutually exclusive\n%s\n",usage);
	return(-1);
}; // end if

if ((!(global.fnameout)) && (!(global.udpport))) {
	snprintf(retmsg,RETMSGSIZE,"Error: output filename or udp host/port missing.\n%s\n",usage);
	return(-1);
}; // end if

if (rawsyncset) {
	if ((rawsyncin < 0) || (rawsyncin > 0xFFFF)) {
		snprintf(retmsg,RETMSGSIZE,"Error: RAW FRAME-SYNC pattern is 16 bit. Should be between 0x0000 and 0xFFFF.\n%s\n",usage);
		return(-1);
	} else {
		// data is good. Store it
		global.rawsync=(uint16_t)rawsyncin;
	}; // end if
}; // end if

if ((global.rawsyncsize <= 0) || (global.rawsyncsize > 16)) {
	snprintf(retmsg,RETMSGSIZE,"Error: RAW FRAME-SYNC SIZE should be between 1 and 16.\n%s\n",usage);
	return(-1);
}; // end if

if ((rawsyncsizeset) && (!(rawsyncset))) {
	snprintf(retmsg,RETMSGSIZE,"Warning: Setting RAW FRAME-SYNC SIZE without FRAME-SYNC does not make sence. Option ignored\n");
	// reset rawsyncsize to default
	global.rawsyncsize=15;
	return(1);
}; // end if

// All done, return
return(0);

}; // end function parse cli
