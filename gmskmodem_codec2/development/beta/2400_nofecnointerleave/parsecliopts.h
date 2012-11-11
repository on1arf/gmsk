/* parsecli.h */


// version 20120202: initial release

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


int parsecliopts(int argc, char ** argv, char * retmsg) {

#ifdef _USEALSA
char * usage="Usage: gmskmodem [-h] [-v] [-4 | -6] [-sb sec] [-se sec] [-audioinvert {n,r,s,b}] [-d] [-dd] [-da] [-s] [-m] [-lf PTTlockfile.lck] {-ria alsadevice | -rif inputfilename} {-rof outputfilename | -rou udphost udpport} {-sif senderinputfilename | -sif - | -sit tcpport | -siu udpport} {-soa alsadevice | -sof senderoutputfile.c2s} [-id idfile idfrequency] [-noreceiver] [-nosender]\n";
char * help="Usage: receiver [-h] [-v] [-4 | -6] [-rs hex] [-rss size] [-audioinvert {n,r,s,b}] [-d] [-dd] [-da] [-s] [-m] [-lf PTTlockfile.lck] {-ria alsadevice | -rif inputfilename} {-rof outputfilename | -rou udphost udpport} {-sif senderinputfilename | -sif - | -sit tcpport | -siu udpport} {-soa alsadevice | -sof senderoutputfile.c2s} [-id idfile idfrequency] [-noreceiver] [-nosender] \n\n Options:\n -h: help (this text)\n -v: verbose\n \n -sb: length of silence at beginning of transmission (1/10 seconds)(SENDER)\n -se: length of silence at end of transmission (1/10 seconds)(SENDER)\n\n -lf: lockfile use to signal PTT switching\n \n -d: dump stream data(RECEIVER)\n -dd: dump more stream data(RECEIVER)\n -da: dump average audio-level data(RECEIVER)\n \n -s: stereo: input alsa-device is stereo(RECEIVER)\n -audioinvert: input audio inverted (needed for certain types of ALSA devices): 'n' (no), 'r' (receive), 's' (sender), 'b' (both) (RECEIVER AND SENDER)\n \n RECEIVER INPUT AND OUTPUT:\n -ria: INPUT ALSA DEVICE \n -rif: INPUT FILE \n -rof: output filename (use \"-\" for stdout)\n -rou: stream out received data over UDP (port + udp port needed)\n SENDER INPUT AND OUTPUT:\n -sif: input file (use \"-\" for stdin)\n -sit: input TCP port\n -siu: input UDP port\n -soa: OUTPUT alsa device\n -sof: OUTPUT file\n\n -4: UDP host hostname lookup ipv4 only(RECEIVER)\n -6: UDP host hostname lookup ipv6 only(RECEIVER) \n\n -noreceiver: disables receiver module\n -nosender: disables sender module\n";
#else
char * usage="Usage: gmskmodem [-h] [-v] [-4 | -6] [-sb sec] [-se sec] [-audioinvert {n,r,s,b}] [-d] [-dd] [-da] [-s] [-m] {-rif inputfilename} {-rof outputfilename | -rou udphost udpport} {-sif senderinputfilename.c2s | -sif - | -sit tcpport | -siu udpport} {-sof senderoutputfile.c2s} [-id idfile idfrequency] [-noreceiver] [-nosender]\n";
char * help="Usage: receiver [-h] [-v] [-4 | -6] [-audioinvert {n,r,s,b}] [-d] [-dd] [-da] [-s] [-m] {-rif inputfilename} {-rof outputfilename | -rou udphost udpport} {-sif senderinputfilename | -sif - | -sit tcpport | -siu udpport} {-sof senderoutputfile.c2s} [-id idfile idfreq] [-noreceiver] [-nosender] \n\n Options:\n -h: help (this text)\n -v: verbose\n \n -sb: length of silence at beginning of transmission (1/10 seconds)(SENDER)\n -se: length of silence at end of transmission (1/10 seconds)(SENDER)\n\n -d: dump stream data(RECEIVER)\n -dd: dump more stream data(RECEIVER)\n -da: dump average audio-level data(RECEIVER)\n \n -s: stereo: input file is stereo(RECEIVER)\n -audioinvert: input audio inverted: 'n' (no), 'r' (receive), 's' (sender), 'b' (both) (RECEIVER AND SENDER)\n \n RECEIVER INPUT AND OUTPUT:\n -rif: INPUT FILE \n -rof: output filename (use \"-\" for stdout)\n -rou: stream out received data over UDP (port + udp port needed)\n SENDER INPUT AND OUTPUT:\n -sif: input file (use \"-\" for stdin)\n -sit: input TCP port\n -siu: input UDP port\n -sof: OUTPUT file\n\n -4: UDP host hostname lookup ipv4 only(RECEIVER)\n -6: UDP host hostname lookup ipv6 only(RECEIVER) \n\n -noreceiver: disables receiver module\n -nosender: disables sender module\n";

#endif
// local vars
int paramloop;

// init vars

// RECEIVER
#ifdef _USEALSA
r_global.fileorcapture=-1;
#endif
r_global.fnameout=NULL;
r_global.outtostdout=0;
r_global.dumpstream=0;
r_global.dumpaverage=0;
r_global.stereo=0;
r_global.udpout_port=0;
r_global.udpout_host=NULL;
r_global.ipv4only=0;
r_global.ipv6only=0;
r_global.disable=0;

// SENDER
#ifdef _USEALSA
s_global.alsaname=NULL;
s_global.pttlockfile=NULL;
s_global.pttcsdevice=NULL;
s_global.ptttxdevice=NULL;
s_global.ptt_invert=0;
s_global.alsaready=0;
#endif
s_global.fnamein=NULL;
s_global.infromstdin=0;
s_global.fnameout=NULL;
s_global.fileoralsa=-1;
s_global.silencebegin=DEFAULT_SILENCEBEGIN;
s_global.silenceend=DEFAULT_SILENCEEND;
s_global.udpport=0;
s_global.tcpport=0;
s_global.disable=0;
s_global.idfile=NULL;
s_global.idfreq=0;
s_global.alsaunderrun=0;


// SENDER AND RECEIVER
g_global.verboselevel=0;
g_global.audioinvert=0;

// usage:

for (paramloop=1; paramloop <argc; paramloop++) {
	char * thisarg=argv[paramloop];



	if (strcmp(thisarg,"-rif") == 0) {
		// -rif: RECEIVER INPUT file name

		#ifdef _USEALSA
		if (r_global.fileorcapture == 0) {
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: receiver ALSA input and file input are mutually exclusive\n");
			return(-1);
		}; // end if
		#endif
 
		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			r_global.fnamein=argv[paramloop];
			#ifdef _USEALSA
			r_global.fileorcapture=1;
			#endif
		}; // end if
	#ifdef _USEALSA
	} else if (strcmp(thisarg,"-ria") == 0) {
		// -ria: RECEIVER INPUT alsa device

		if (r_global.fileorcapture == 1) {
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: -ria and -rif are mutually exclusive\n");
			return(-1);
		}; // end if

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			r_global.capturedevice=argv[paramloop];
			r_global.fileorcapture=0;
		}; // end if
	#endif
	} else if (strcmp(thisarg,"-rou") == 0) {
		// -rou: RECEIVER OUTPUT udphost udpport

		// is there a next argument?
		if (paramloop+2 < argc ) {
			paramloop++;
			
			r_global.udpout_host=argv[paramloop];

			paramloop++;
			r_global.udpout_port=atoi(argv[paramloop]);

			if ((r_global.udpout_port <= 0) || (r_global.udpout_port > 65535)) {
				snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: portnumber must be between 1 and 65535\n");
				return(-1);
			}; // end if

		} else {
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: -u needs two additional arguments: host and port\n");
			return(-1);
		}; // end if
	} else if (strcmp(thisarg,"-id") == 0) {
		// -id: idfile idfreq

		// is there a next argument?
		if (paramloop+2 < argc ) {
			paramloop++;
			
			s_global.idfile=argv[paramloop];

			paramloop++;
			s_global.idfreq=atoi(argv[paramloop]);

			if (s_global.idfreq < 1) {
				snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: ID repeat frequency should be at least 1\n");
				return(-1);
			}; // end if

		} else {
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: -id needs two additional arguments: idfile and idfrequency\n");
			return(-1);
		}; // end if
	} else if (strcmp(thisarg,"-rof") == 0) {
		// -rof: RECEIVER OUTPUT file name

		// filename may only be defined once
		if (r_global.fnameout) {
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: only one output filename allowed\n");
			return(-1);
		}; // end if

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			r_global.fnameout=argv[paramloop];
 		} else {
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: Missing argument.\n%s\n",usage);
			return(-1);
		}; // end if
		
		if (strcmp(r_global.fnameout,"-") == 0) {
			r_global.outtostdout=1;
		}; // end if

	} else if (strcmp(thisarg,"-d") == 0) {
		// -d: dump stream data
		if (! r_global.dumpstream) {
			r_global.dumpstream=1;
		}; // end if
	} else if (strcmp(thisarg,"-dd") == 0) {
		// -dd: dump more data
		r_global.dumpstream=2;
	} else if (strcmp(thisarg,"-da") == 0) {
		// -da: dump average audio level data
		r_global.dumpaverage=1;

		// dump audio includes dump stream 
		if (! r_global.dumpstream) {
			r_global.dumpstream=1;
		}; // end if
	#ifdef _USEALSA
	} else if (strcmp(thisarg,"-ptt_cs") == 0) {
		// -ptt_cs: serial device PTT control-signal

		if ((s_global.pttlockfile) || (s_global.pttcsdevice) || (s_global.ptttxdevice)){
		// PTT already defined
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: PTT serial device or Lockfile can only be defined once!\n%s\n",usage);
			return(-1);
		}; // end if

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			s_global.pttcsdevice=argv[paramloop];
		}; // end if

	} else if (strcmp(thisarg,"-ptt_tx") == 0) {
		// -ptt_tx: serial device PTT TX-out

		if ((s_global.pttlockfile) || (s_global.pttcsdevice) || (s_global.ptttxdevice)){
		// PTT already defined
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: PTT serial device or Lockfile can only be defined once!\n%s\n",usage);
			return(-1);
		}; // end if

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			s_global.ptttxdevice=argv[paramloop];
		}; // end if

	} else if (strcmp(thisarg,"-ptt_lf") == 0) {
		// -ptt_lf: PTT lock file

		if ((s_global.pttlockfile) || (s_global.pttcsdevice) || (s_global.ptttxdevice)){
		// PTT already defined
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: PTT serial device or Lockfile can only be defined once!\n%s\n",usage);
			return(-1);
		}; // end if

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			s_global.pttlockfile=argv[paramloop];
		}; // end if
	} else if (strcmp(thisarg,"-pttinvert") == 0) {
		// -pttinvert
		s_global.ptt_invert=1;
	#endif
	} else if (strcmp(thisarg,"-sb") == 0) {
		// -sb: silence begin

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			s_global.silencebegin=atoi(argv[paramloop]);
 		}; // end if
	} else if (strcmp(thisarg,"-se") == 0) {
		// -se: silence end

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			s_global.silenceend=atoi(argv[paramloop]);
		}; // end if

	} else if (strcmp(thisarg,"-s") == 0) {
		// -s: stereo
		r_global.stereo=1;
	} else if (strcmp(thisarg,"-v") == 0) {
		// -v: verbose
		g_global.verboselevel++;
	} else if (strcmp(thisarg,"-h") == 0) {
		// -h: help
		fprintf(stderr,"%s\n\n",help);
		exit(-1);
	} else if (strcmp(thisarg,"-4") == 0) {
		// -4: ipv4 only
		r_global.ipv4only=1;
	} else if (strcmp(thisarg,"-6") == 0) {
		// -6: ipv6 only
		r_global.ipv6only=1;
	} else if (strcmp(thisarg,"-noreceiver") == 0) {
		// -noreceiver: disable receiver
		r_global.disable=1;
	} else if (strcmp(thisarg,"-nosender") == 0) {
		// -nosender: disable sender
		s_global.disable=1;
	} else if (strcmp(thisarg,"-sif") == 0) {
		// -sif: SENDER INPUT file

		// SENDER input can be either file, tcp or udp
		if ((s_global.tcpport) || (s_global.udpport) || (s_global.fnamein)){
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: sender input defined multiple times\n");
			return(-1);
		}; // end if

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			s_global.fnamein=argv[paramloop];

			if (strcmp(s_global.fnamein,"-") == 0) {
				s_global.infromstdin=1;
			}; // end if

		}; // end if

	} else if (strcmp(thisarg,"-sit") == 0) {
		// -sif: SENDER INPUT TCP port

		// SENDER input can be either file, tcp or udp
		if ((s_global.tcpport) || (s_global.udpport) || (s_global.fnamein)){
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: sender input defined multiple times\n");
			return(-1);
		}; // end if

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			s_global.tcpport=atoi(argv[paramloop]);
		}; // end if

	} else if (strcmp(thisarg,"-siu") == 0) {
		// -sif: SENDER INPUT UDP port

		// SENDER input can be either file, tcp or udp
		if ((s_global.tcpport) || (s_global.udpport) || (s_global.fnamein)){
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: sender input defined multiple times\n");
			return(-1);
		}; // end if

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			s_global.udpport=atoi(argv[paramloop]);
		}; // end if

	#ifdef _USEALSA
	} else if (strcmp(thisarg,"-soa") == 0) {
		// -soa: SENDER OUTPUT alsadevice

		// is there a next argument?
		if (paramloop+1 < argc) {
			s_global.fileoralsa=1;
			paramloop++;
			s_global.alsaname=argv[paramloop];
		} else {
			s_global.fileoralsa=-1;
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: Missing argument.\n%s\n",usage);
			return(-1);
		}; // end if
	#endif
	} else if (strcmp(thisarg,"-sof") == 0) {
		if (s_global.fileoralsa==1) {
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: File-out and alsa-out are mutually exclusive.\n%s\n",usage);
			return(-1);
		} else {
			if (paramloop+1 < argc) {
				paramloop++;
				s_global.fileoralsa=0;
				s_global.fnameout=argv[paramloop];
			} else {
				s_global.fileoralsa=-1;
				snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: Missing argument.\n%s\n",usage);
				return(-1);
			}; // end else - if
		}; // end else - if
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
if ((s_global.disable) && (r_global.disable)) {
	snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: Disabling both receiver and sender makes no sence.\n%s\n",usage);
	return(-1);
}; // end if


// RECEIVER CHECKS
if (!r_global.disable) {
	#ifdef _USEALSA
	if (r_global.fileorcapture == -1) {
		snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: receiver input source missing.\n%s\n",usage);
		return(-1);
	}; // end if
	#endif

	if ((r_global.ipv4only) && (r_global.ipv6only)){
		snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: IPv4-only and IPv6-only are mutually exclusive\n%s\n",usage);
		return(-1);
	}; // end if

	if ((!(r_global.fnameout)) && (!(r_global.udpout_port))) {
		snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: receiver output filename or udp host/port missing.\n%s\n",usage);
		return(-1);
	}; // end if

}; // end if (RECEIVER not disabled)

// SENDER CHECKS
if (!s_global.disable) {
	if ((!(s_global.fnamein)) && (!(s_global.tcpport)) && (!(s_global.udpport))) {
		snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: SENDER input methode missing. Should be -sif, -siu or -sip!\n%s\n",usage);
		return(-1);
	}; // end if

	if (s_global.fileoralsa == -1) {
		snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: SENDER output file or alsa-device missing.\n%s\n",usage);
		return(-1);
	}; // end if

	#ifdef _USEALSA
	if ((s_global.fileoralsa == 0) && ((s_global.pttlockfile) || (s_global.pttcsdevice) || (s_global.ptttxdevice))) {
		snprintf(retmsg,PARSECLIRETMSGSIZE,"Warning: PTT switching does not make sence when not using SENDER alsa-out. Ignored!\n");
		s_global.pttlockfile=NULL;
		s_global.pttcsdevice=NULL;
		s_global.ptttxdevice=NULL;
		return(1);
	}; // end if
	#endif

}; // end if (SENDER not disabled)


// All done, return
return(0);

}; // end function parse cli
