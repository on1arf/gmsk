/* parsecli.h */


// PORTAUDIO / API / UDP-ONLY version


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

// version 20130326: initial release
// version 20130404: added half duplex support


int parsecliopts(int argc, char ** argv, char * retmsg, struct globaldata * pglobal) {

char * usage="Usage: gmskmodem [options] -adevice <audiodevice> [-exact] -udp <remote ip-address> <remote udp-port> <local udp-port>\n\
Use \"gmskmodem -h\" for full option list";

char * help="Usage: gmskmodem [-h] [-v] [-4 | -6] [-d] [-dd] [-da] [-sb <0.1sec>] [-se <0.1-sec>] [-id idfile idfreq] [{-ptt_cs serialdevice} | {-ptt_tx serialdevice} | {-ptt_gpio gpioport} | {-ptt_lf lockfile}] [-pttinvert] [-forcestereo {n,r,s,b}] [-audioinvert] -adevice <audiodevice> [-exact] -udp <remote ip-address> <remote udp-port> <local udp-port>\n\
\n\
Options:\n\
 -h: help (this text)\n\
 -v: verbose (repeat for more verbose)\n\
\n\
Mandatory arguments:\n\
 -udp <remote ip-address> <remote udp-port> <local udp-port>: IP-address + UDP port of remote \"audiotool\" application, local UDP-port to receive UDP-traffic\n\
 -adevice: portaudio device\n\
\n\
Optional arguments:\n\
 -exact: portaudio devicename must match exactly\n\
\n\
 -forcestereo: force audio device as stereo: 'n' (no), 'r' (receiver), 's' (sender), 'b' (both)\n\
 -audioinvert: sender audio invertion (SENDER). Receiver audio-invertion is auto-detected.\n\
 -halfduplex: audio devices half duplex.\n\
\n\
\n\
 -sb: length of silence at beginning of transmission (1/10 seconds)\n\
 -se: length of silence at end of transmission (1/10 seconds)\n\
\n\
 -ptt_cs: PTT switching via control lines (DSR/RTS/CTS/RTS)\n\
 -ptt_tx: PTT switching via sending 0x00 to serial device\n\
 -ptt_cs: PTT switching via gpio\n\
 -ptt_lf: PTT switching via posix lock on external file\n\
 -pttinvert: invert PTT switching \n\
\n\
  -d: dump stream data\n\
 -dd: dump more stream data\n\
 -da: dump average audio-level data\n\
\n\
 -4: UDP host hostname lookup ipv4 only\n\
 -6: UDP host hostname lookup ipv6 only\n\
\n\
";

// local vars
int paramloop;


// usage:

for (paramloop=1; paramloop <argc; paramloop++) {
	char * thisarg=argv[paramloop];


	if (strcmp(thisarg,"-id") == 0) {
		// -id: idfile idfreq

		// are there two additonal arguments?
		if (paramloop+2 < argc ) {
			paramloop++;
			
			pglobal->idfile=argv[paramloop];

			paramloop++;
			pglobal->idfreq=atoi(argv[paramloop]);

			if (pglobal->idfreq < 1) {
				snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: ID repeat frequency should be at least 1\n");
				return(-1);
			}; // end if

		} else {
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: -id needs two additional arguments: idfile and idfrequency\n");
			return(-1);
		}; // end if
	} else if (strcmp(thisarg,"-halfduplex") == 0) {
		pglobal->halfduplex=1;
	} else if (strcmp(thisarg,"-d") == 0) {
		// -d: dump stream data
		if (! pglobal->dumpstream) {
			pglobal->dumpstream=1;
		}; // end if
	} else if (strcmp(thisarg,"-dd") == 0) {
		// -dd: dump more data
		pglobal->dumpstream=2;
	} else if (strcmp(thisarg,"-da") == 0) {
		// -da: dump average audio level data
		pglobal->dumpaverage=1;

		// dump audio includes dump stream 
		if (! pglobal->dumpstream) {
			pglobal->dumpstream=1;
		}; // end if
	} else if (strcmp(thisarg,"-ptt_cs") == 0) {
		// -ptt_cs: serial device PTT control-signal

		if ((pglobal->pttlockfile) || (pglobal->pttcsdevice) || (pglobal->ptttxdevice) || (pglobal->ptt_gpiopin != -1)){
		// PTT already defined
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: PTT serial device, GPIO or Lockfile can only be defined once!\n%s\n",usage);
			return(-1);
		}; // end if

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			pglobal->pttcsdevice=argv[paramloop];
		} else {
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: missing argument for -ptt_cs\n");
			return(-1);
		}; // end if

	} else if (strcmp(thisarg,"-ptt_tx") == 0) {
		// -ptt_tx: serial device PTT TX-out

		if ((pglobal->pttlockfile) || (pglobal->pttcsdevice) || (pglobal->ptttxdevice) || (pglobal->ptt_gpiopin != -1)){
		// PTT already defined
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: PTT serial device, GPIO or Lockfile can only be defined once!\n%s\n",usage);
			return(-1);
		}; // end if

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			pglobal->ptttxdevice=argv[paramloop];
		} else {
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: missing argument for -ptt_tx\n");
			return(-1);
		}; // end if

	} else if (strcmp(thisarg,"-ptt_lf") == 0) {
		// -ptt_lf: PTT lock file

		if ((pglobal->pttlockfile) || (pglobal->pttcsdevice) || (pglobal->ptttxdevice) || (pglobal->ptt_gpiopin != -1)){
		// PTT already defined
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: PTT serial device, GPIO or Lockfile can only be defined once!\n%s\n",usage);
			return(-1);
		}; // end if

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			pglobal->pttlockfile=argv[paramloop];
		} else {
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: missing argument for -ptt_lf\n");
			return(-1);
		}; // end if

   } else if (strcmp(thisarg,"-ptt_gpio") == 0) {
      // -ptt_gpio: PTT GPIO pin

      if ((pglobal->pttlockfile) || (pglobal->pttcsdevice) || (pglobal->ptttxdevice) || (pglobal->ptt_gpiopin != -1)){
      // PTT already defined
         snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: PTT serial device, GPIO pin or Lockfile can only be defined once!\n%s\n",usage);
         return(-1);
      }; // end if

      // is there a next argument?
      if (paramloop+1 < argc) {
         paramloop++;
         pglobal->ptt_gpiopin=atoi(argv[paramloop]);

         if (pglobal->ptt_gpiopin < 0) {
            snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: PTT GPIO-pin must be 0 or more!\n");
            return(-1);
         }; // end if
		} else {
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: missing argument for -ptt_gpio\n");
			return(-1);
      }; // end if

	} else if (strcmp(thisarg,"-pttinvert") == 0) {
		// -pttinvert
		pglobal->ptt_invert=1;
	} else if (strcmp(thisarg,"-sb") == 0) {
		// -sb: silence begin

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			pglobal->silencebegin=atoi(argv[paramloop]);
		} else {
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: missing argument for -sb\n");
			return(-1);
 		}; // end if
	} else if (strcmp(thisarg,"-se") == 0) {
		// -se: silence end

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			pglobal->silenceend=atoi(argv[paramloop]);
		} else {
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: missing argument for -se\n");
			return(-1);
		}; // end if

	} else if (strcmp(thisarg,"-v") == 0) {
		// -v: verbose
		pglobal->verboselevel++;
	} else if (strcmp(thisarg,"-h") == 0) {
		// -h: help
		fprintf(stderr,"%s\n\n",help);
		exit(-1);
	} else if (strcmp(thisarg,"-4") == 0) {
		// -4: ipv4 only
		pglobal->ipv4only=1;
	} else if (strcmp(thisarg,"-6") == 0) {
		// -6: ipv6 only
		pglobal->ipv6only=1;
	} else if (strcmp(thisarg,"-audioinvert") == 0) {
		// -audioinvert: invert audio for sender
		pglobal->audioinvert_s=1;
	} else if (strcmp(thisarg,"-forcestereo") == 0) {
		// -forcestereo: 0 (none), 1 (receive), 2 (transmit), 3 (both)

		if (paramloop+1 < argc) {
    		paramloop++;

			if ((argv[paramloop][0] == 'n') || (argv[paramloop][0] == 'N')) {
				pglobal->forcestereo_r=0; pglobal->forcestereo_s=0;  // none
			} else if ((argv[paramloop][0] == 'r') || (argv[paramloop][0] == 'R')) {
				pglobal->forcestereo_r=1; pglobal->forcestereo_s=0; // receive
			} else if ((argv[paramloop][0] == 's') || (argv[paramloop][0] == 'S')) {
				pglobal->forcestereo_r=0; pglobal->forcestereo_s=1; // sender
			} else if ((argv[paramloop][0] == 'b') || (argv[paramloop][0] == 'B')) {
				pglobal->forcestereo_r=1; pglobal->forcestereo_s=1; // both
			} else {
				snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: Invalid force-stereo parameter %c, should be 'n', 'r', 't' or 'b'\n",argv[paramloop][0]);
				return(-1);
			}; // end else - if
		} else {
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: missing argument for -forcestereo\n");
			return(-1);
		}; // end else - if

	} else if (strcmp(thisarg,"-adevice") == 0) {
		// -adevice: audio device

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			pglobal->adevice=argv[paramloop];
		} else {
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: missing argument for -adevice\n");
			return(-1);
 		}; // end if
	} else if (strcmp(thisarg,"-exact") == 0) {
		// -exact: audio devicename must match exactly
		pglobal->exact=1;
	} else if (strcmp(thisarg,"-udp") == 0) {
		// -udp: remote ip-address, remote udp-port and local udp-port

		// we need at least 3 arguments
		if (paramloop+3 < argc) {
    		paramloop++;
			pglobal->udpout_host=argv[paramloop];

    		paramloop++;
			pglobal->udpout_port=atoi(argv[paramloop]);

    		paramloop++;
			pglobal->udpin_port=atoi(argv[paramloop]);


			// some checks
			if ((pglobal->udpout_port < 1) || (pglobal->udpout_port > 65535)) {
				snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: remote UDP port should be between 1 and 65535, got %d\n",pglobal->udpout_port);
				return(-1);
			}; // end if

			if ((pglobal->udpin_port < 1) || (pglobal->udpin_port > 65535)) {
				snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: local UDP port should be between 1 and 65535, got %d\n",pglobal->udpin_port);
				return(-1);
			}; // end if

		} else {
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: missing argument for -udp\n");
			return(-1);
		}; // end if


	} else {
		snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: Unknown argument: %s\n",argv[paramloop]);
		return(-1);
	}; // end else - elsif - if

}; // end for



// first test, if no parameters given, give usage
if (argc <= 2) {
	snprintf(retmsg,PARSECLIRETMSGSIZE,"%s\n",usage);
	return(-1);
}; // end if

// Done reading all parameters.
// set some implicit parameters


////////////////////////////////////////
// ERROR CHECKING

// Check if we have sufficient parameters

// GLOBAL checks
if (!(pglobal->adevice)) {
	snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: Audio device missing.\n%s\n",usage);
	return(-1);
}; // end if

if ((!(pglobal->udpout_host))  || (!(pglobal->udpout_port)) || (!(pglobal->udpin_port))) {
	snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: ipaddress/port/local-port for udp streaming missing.\n%s\n",usage);
	return(-1);
}; // end if


/////////////

// RECEIVER CHECKS
if ((pglobal->ipv4only) && (pglobal->ipv6only)){
	snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: IPv4-only and IPv6-only are mutually exclusive\n%s\n",usage);
	return(-1);
}; // end if


// SENDER CHECKS


// All done, return
return(0);

}; // end function parse cli
