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
char * usage="Usage: gmskmodem [-h] [-v] [-4 | -6] [-sb sec] [-se sec] [-rs hex] [-rss size] [-rawinvert] [-audioinvert {n,r,s,b}] [-d] [-dd] [-da] [-s] [-resync] [-z] [-m] [-lf PTTlockfile.lck] -format {d,r,s,c} [-dvsystem {d,c}] [-recformat {d,r,s}] {-ria alsadevice | -rif inputfilename} {-rof outputfilename | -rou udphost udpport} {-sif senderinputfilename | -sif - | -sit tcpport | -siu udpport} {-soa alsadevice | -sof senderoutputfile.raw} [-id idfile idfrequency] [-noreceiver] [-nosender]\n";
char * help="Usage: receiver [-h] [-v] [-4 | -6] [-rs hex] [-rss size] [-rawinvert] [-audioinvert {n,r,s,b}] [-d] [-dd] [-da] [-s] [-m] [-lf PTTlockfile.lck] -format {d,r,s} [-recformat {d,r,s}] {-ria alsadevice | -rif inputfilename} {-rof outputfilename | -rou udphost udpport} {-sif senderinputfilename | -sif - | -sit tcpport | -siu udpport} {-soa alsadevice | -sof senderoutputfile.raw} [-id idfile idfrequency] [-noreceiver] [-nosender] \n\n Options:\n -h: help (this text)\n -v: verbose\n \n -format: file/stdin-out format: d (D-STAR dvtool), s (D-STAR stream), c (codec2) or r (raw) (RECEIVER AND SENDER)\n -dvsystem: d (d-star), c (codec2)\n -recformat: overwrites global format-setting for receiver\n\n -rs: RAW-mode frame syncronisation pattern (default: 0x7650 for format D-STAR, 0x0567 for format codec2) (RECEIVER)\n -rss: RAW-mode frame syncronisation pattern size (default: 15 bits, as used by D-STAR)(RECEIVER)\n -rawinvert: RAW-mode bitsequence invert (bits read/written from left (bit7) to right (bit0)) (SENDER AND RECEIVER)\n -sb: length of silence at beginning of transmission (1/10 seconds)(SENDER)\n -se: length of silence at end of transmission (1/10 seconds)(SENDER)\n\n -resync:  resyncronize: overwrite 21-frame syncronisation pattern in slow-speed data with standard D-STAR pattern(SENDER)\n -z: Zap (delete) D-STAR slow-speed data information(SENDER)\n -lf: lockfile use to signal PTT switching\n -m: add begin and end MARKERS to raw data output(RECEIVER)\n \n -d: dump stream data(RECEIVER)\n -dd: dump more stream data(RECEIVER)\n -da: dump average audio-level data(RECEIVER)\n \n -s: stereo: input alsa-device is stereo(RECEIVER)\n -audioinvert: input audio inverted (needed for certain types of ALSA devices): 'n' (no), 'r' (receive), 's' (sender), 'b' (both) (RECEIVER AND SENDER)\n \n RECEIVER INPUT AND OUTPUT:\n -ria: INPUT ALSA DEVICE \n -rif: INPUT FILE \n -rof: output filename (use \"-\" for stdout)\n -rou: stream out received data over UDP (port + udp port needed)\n SENDER INPUT AND OUTPUT:\n -sif: input file (use \"-\" for stdin)\n -sit: input TCP port\n -siu: input UDP port\n -soa: OUTPUT alsa device\n -sof: OUTPUT file\n\n -4: UDP host hostname lookup ipv4 only(RECEIVER)\n -6: UDP host hostname lookup ipv6 only(RECEIVER) \n\n -noreceiver: disables receiver module\n -nosender: disables sender module\n";
#else
char * usage="Usage: gmskmodem [-h] [-v] [-4 | -6] [-sb sec] [-se sec] [-rs hex] [-rss size] [-rawinvert] [-audioinvert {n,r,s,b}] [-d] [-dd] [-da] [-s] [-resync] [-z] [-m] -format {d,r,s,c} [-dvsystem {d,c}] [-recformat {d,r,s}] {-rif inputfilename} {-rof outputfilename | -rou udphost udpport} {-sif senderinputfilename | -sif - | -sit tcpport | -siu udpport} {-sof senderoutputfile.raw} [-id idfile idfrequency] [-noreceiver] [-nosender]\n";
char * help="Usage: receiver [-h] [-v] [-4 | -6] [-rs hex] [-rss size] [-rawinvert] [-audioinvert {n,r,s,b}] [-d] [-dd] [-da] [-s] [-m] -format {d,r,s} [-recformat {d,r,s}] {-rif inputfilename} {-rof outputfilename | -rou udphost udpport} {-sif senderinputfilename | -sif - | -sit tcpport | -siu udpport} {-sof senderoutputfile.raw} [-id idfile idfreq] [-noreceiver] [-nosender] \n\n Options:\n -h: help (this text)\n -v: verbose\n \n -format: file/stdin-out format: d (D-STAR dvtool), s (D-STAR stream), c (codec2) or r (raw) (RECEIVER AND SENDER)\n -dvsystem: d (d-star), c (codec2)\n -recformat: overwrites global format-setting for receiver\n\n -rs: RAW-mode frame syncronisation pattern (default: 0x7650 for format D-STAR, 0x0567 for format CODEC2) (RECEIVER)\n -rss: RAW-mode frame syncronisation pattern size (default: 15 bits, as used by D-STAR)(RECEIVER)\n -rawinvert: RAW-mode bitsequence invert (bits read/written from left (bit7) to right (bit0)) (SENDER AND RECEIVER)\n -sb: length of silence at beginning of transmission (1/10 seconds)(SENDER)\n -se: length of silence at end of transmission (1/10 seconds)(SENDER)\n\n -resync:  resyncronize: overwrite D-STAR 21-frame syncronisation pattern in slow-speed data with standard pattern(SENDER)\n -z: Zap (delete) D-STAR slow-speed data information(SENDER)\n -m: add begin and end MARKERS to raw data output(RECEIVER)\n \n -d: dump stream data(RECEIVER)\n -dd: dump more stream data(RECEIVER)\n -da: dump average audio-level data(RECEIVER)\n \n -s: stereo: input file is stereo(RECEIVER)\n -audioinvert: input audio inverted: 'n' (no), 'r' (receive), 's' (sender), 'b' (both) (RECEIVER AND SENDER)\n \n RECEIVER INPUT AND OUTPUT:\n -rif: INPUT FILE \n -rof: output filename (use \"-\" for stdout)\n -rou: stream out received data over UDP (port + udp port needed)\n SENDER INPUT AND OUTPUT:\n -sif: input file (use \"-\" for stdin)\n -sit: input TCP port\n -siu: input UDP port\n -sof: OUTPUT file\n\n -4: UDP host hostname lookup ipv4 only(RECEIVER)\n -6: UDP host hostname lookup ipv6 only(RECEIVER) \n\n -noreceiver: disables receiver module\n -nosender: disables sender module\n";

#endif
// local vars
int paramloop;

int syncpatternset=0;
int syncsizeset=0;

// temp data
int syncpatternin=0;


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
r_global.sendmarker=0;
r_global.syncpattern=0; 
r_global.syncsize=-1; 
r_global.recformat=0;
r_global.disable=0;

// SENDER
#ifdef _USEALSA
s_global.alsaname=NULL;
s_global.pttlockfile=NULL;
s_global.pttcsdevice=NULL;
s_global.ptttxdevice=NULL;
s_global.ptt_invert=0;
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


// SENDER AND RECEIVER
g_global.verboselevel=0;
g_global.dvsystem=0;
g_global.format=0;
g_global.audioinvert=0;
g_global.rawinvert=0;

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
	} else if (strcmp(thisarg,"-m") == 0) {
		// -m: send begin/end markers
		r_global.sendmarker=1;
	} else if (strcmp(thisarg,"-recformat") == 0) {
		// -recformat: format for receiver, overwrites global format setting:
		//         d (D-STAR dvtool), s (D-STAR stream) or r (NON D-STAR RAW)

		if (paramloop+1 < argc) {
			paramloop++;

			if ((argv[paramloop][0] == 'd') || (argv[paramloop][0] == 'D')) {
				r_global.recformat=1;
			} else if ((argv[paramloop][0] == 's') || (argv[paramloop][0] == 'S')) {
				r_global.recformat=2;
			} else if ((argv[paramloop][0] == 'r') || (argv[paramloop][0] == 'R')) {
				r_global.recformat=10;
			} else if ((argv[paramloop][0] == 'c') || (argv[paramloop][0] == 'C')) {
				r_global.recformat=20;
			} else {
				snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: Invalid receiver-format parameter %c, should be 'd', 's', 'c' or 'r'\n",argv[paramloop][0]);
				return(-1);
			}; // end else - elsif - elsif - if 
		} else {
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: Missing receiver-format parameter\n");
			return(-1);
		}; // end else - if
	} else if (strcmp(thisarg,"-format") == 0) {
		// -format: d (D-STAR dvtool), s (D-STAR stream) or r (NON D-STAR RAW)

		if (paramloop+1 < argc) {
			paramloop++;

			if ((argv[paramloop][0] == 'd') || (argv[paramloop][0] == 'D')) {
				g_global.format=1;
			} else if ((argv[paramloop][0] == 's') || (argv[paramloop][0] == 'S')) {
				g_global.format=2;
			} else if ((argv[paramloop][0] == 'r') || (argv[paramloop][0] == 'R')) {
				g_global.format=10;
			} else if ((argv[paramloop][0] == 'c') || (argv[paramloop][0] == 'C')) {
				g_global.format=20;
			} else {
				snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: Invalid format parameter %c, should be 'd', 's', 'c' or 'r'\n",argv[paramloop][0]);
				return(-1);
			}; // end else - elsif - elsif - if 
		} else {
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: Missing format parameter\n");
			return(-1);
		}; // end else - if
	} else if (strcmp(thisarg,"-rs") == 0) {
		// -rs: sync pattern
		if (paramloop+1 < argc) {
			paramloop++;
			syncpatternin=atoi(argv[paramloop]);
			syncpatternset=1;
		}; // end if
	} else if (strcmp(thisarg,"-rss") == 0) {
		// -rs: raw sync pattern
		if (paramloop+1 < argc) {
			paramloop++;
			r_global.syncsize=atoi(argv[paramloop]);
			syncsizeset=1;
		}; // end if
	} else if (strcmp(thisarg,"-rawinvert") == 0) {
		// -raw invert
		g_global.rawinvert=1;
	} else if (strcmp(thisarg,"-audioinvert") == 0) {
		// -audioinvert: 0 (none), 1 (receive), 2 (transmit), 3 (both)

		if (paramloop+1 < argc) {
			paramloop++;

			if ((argv[paramloop][0] == 'n') || (argv[paramloop][0] == 'N')) {
				g_global.audioinvert=0; // none
			} else if ((argv[paramloop][0] == 'r') || (argv[paramloop][0] == 'R')) {
				g_global.audioinvert=1; // receive
			} else if ((argv[paramloop][0] == 's') || (argv[paramloop][0] == 'S')) {
				g_global.audioinvert=2; // sender
			} else if ((argv[paramloop][0] == 'b') || (argv[paramloop][0] == 'B')) {
				g_global.audioinvert=3; // both
			} else {
				snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: Invalid rawinvert parameter %c, should be 'n', 'r', 't' or 'b'\n",argv[paramloop][0]);
				return(-1);
			}; // end else - if
		} else {
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: Missing rawinvert parameter\n");
			return(-1);
		}; // end else - if

	} else if (strcmp(thisarg,"-dvsystem") ==0) {
		// codec: 1 (ambe d-star), 2 (codec2)

		if (paramloop+1 < argc) {
			paramloop++;

			if ((argv[paramloop][0] == 'd') || (argv[paramloop][0] == 'D')) {
				g_global.dvsystem=1; // d-star
			} else if ((argv[paramloop][0] == 'c') || (argv[paramloop][0] == 'C')) {
				g_global.dvsystem=1; // codec2
			} else {
				snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: Invalid dvsystem parameter %c, should be 'd', 'c'\n",argv[paramloop][0]);
				return(-1);
			}; // end else - elsif - if

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
	} else if (strcmp(thisarg,"-resync") == 0) {
		// -s: resync
		s_global.sync=1;
	} else if (strcmp(thisarg,"-z") == 0) {
		// -z: zap slowspeeddata 
		s_global.zap=1;
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

// if no specific receiver format defined, global format parameter
if (!r_global.recformat) {
	r_global.recformat=g_global.format;
}; // end if

if (!g_global.dvsystem) {
	// try to determine dvsystem based on format-type
	// if format is "d-star" or "d-star stream", set dvsystem to d-star
	// if format is "codec2", set dvsystem to codec2
	if ((g_global.format == 1) || (g_global.format == 2)) {
		g_global.dvsystem=1; // d-sytar
	} else if (g_global.format == 20) {
		g_global.dvsystem=2; // codec2
	}; // end if
}; // end if


if ((!g_global.dvsystem) && (!g_global.format)) {
	snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: No DV-system or format given\n%s\n",usage);
	return(-1);
}; // end if

// synpatterns:
if (g_global.dvsystem == 1) {
	if (!syncpatternset) {
		r_global.syncpattern=0x7650; // raw sync frame syncronisation pattern. Default of 0x7650, as used by D-STAR
	}; // end if

	if (!syncsizeset) {
		r_global.syncsize=15; // number of bits in frame syn size, default is 15, as used by D-STAR 
	}; // end if
} else if (g_global.dvsystem == 2) {
	if (!syncpatternset) {
		r_global.syncpattern=0x0567; // raw sync frame syncronisation pattern. Default of 0x0567, as used by codec2
	}; // end if

	if (!syncsizeset) {
		r_global.syncsize=15; // number of bits in frame syn size, default is 15, as used by codec2
	}; // end if
} else {
	if (!syncpatternset) {
		snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: No default value for sync pattern known for this mode\n%s\n",usage);
		return(-1);
	}; // end if

	if (!syncsizeset) {
		snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: No default value for sync size known for this mode\n%s\n",usage);
		return(-1);
	}; // end if
}; // end else - elsif - if


////////////////////////////////////////
// ERROR CHECKING

// Check if we have sufficient parameters

// GLOBAL checks
if ((s_global.disable) && (r_global.disable)) {
	snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: Disabling both receiver and sender makes no sence.\n%s\n",usage);
	return(-1);
}; // end if

if (g_global.format == 0) {
	snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: format parameter missing.\n%s\n",usage);
	return(-1);
}; // end if

if (g_global.dvsystem == 0) {
	snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: DVsystem parameter missing.\n%s\n",usage);
	return(-1);
}; // end if

if (g_global.dvsystem == 1) {
	if ((g_global.format != 1) && (g_global.format != 2) && (g_global.format != 10)) {
		snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: DVsystem D-STAR requires format d-star, d-star stream or raw\n%s\n",usage);
		return(-1);
	}; // end if
} else if (g_global.dvsystem == 1) {
	if ((g_global.format != 10) && (g_global.format != 20)) {
		snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: DVsystem CODEC2 requires format codec2 or raw\n%s\n",usage);
		return(-1);
	}; // end if
}; // end elsif - if



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

	if (syncpatternset) {
		if ((syncpatternin < 0) || (syncpatternin > 0xFFFF)) {
			snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: RAW FRAME-SYNC pattern should be 16 bit, between 0x0000 and 0xFFFF.\n%s\n",usage);
			return(-1);
		} else {
			// data is good. Store it
			r_global.syncpattern=(uint16_t)syncpatternin;
		}; // end if
	}; // end if

	if ((r_global.syncsize <= 0) || (r_global.syncsize > 16)) {
		snprintf(retmsg,PARSECLIRETMSGSIZE,"Error: RAW FRAME-SYNC SIZE should be between 1 and 16.\n%s\n",usage);
		return(-1);
	}; // end if

	if ((syncsizeset) && (!(syncpatternset))) {
		snprintf(retmsg,PARSECLIRETMSGSIZE,"Warning: Setting RAW FRAME-SYNC SIZE without FRAME-SYNC does not make sence. Option ignored\n");
		// reset rawsyncsize to default
		r_global.syncsize=15;
		return(1);
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

	if ((g_global.rawinvert) && (g_global.format != 10)) {
		snprintf(retmsg,PARSECLIRETMSGSIZE,"Warning: Raw-invert option without option \"raw\" does not make sence. Ignored!\n");
		return(1);
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
