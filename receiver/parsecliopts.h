/* parsecli.h */



int parsecliopts(int argc, char ** argv, char * retmsg) {

// local vars
int paramloop;
	
// init vars
global.fileorcapture=-1;
global.fnameout=NULL;
global.dumpstream=0;
global.dumpaverage=0;
global.invert=0;
global.stereo=0;


// usage:
// receiver [-d] [-dd] [-da] [-i] [-s] {-a alsadevice | -f filein} fileout
// known options

for (paramloop=1; paramloop <argc; paramloop++) {
	char * thisarg=argv[paramloop];

	if (strcmp(thisarg,"-a") == 0) {
		// -a: alsa device

		if (global.fileorcapture != -1) {
			snprintf(retmsg,160,"Error: -a and -f are mutually exclusive\n");
			return(-1);
		}; // end if

		// is there a next argument?
		if (paramloop+1 < argc) {
			paramloop++;
			global.capturedevice=argv[paramloop];
			global.fileorcapture=0;
		}; // end if
	} else if (strcmp(thisarg,"-f") == 0) {
		// -f: file name

		if (global.fileorcapture != -1) {
			snprintf(retmsg,160,"Error: -a and -f are mutually exclusive\n");
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
	} else if (strcmp(thisarg,"-i") == 0) {
		// -i: invert
		global.invert=1;
	} else if (strcmp(thisarg,"-s") == 0) {
		// -s: stereo
		global.stereo=1;
	} else {
		// last option: output file name

		// filename may only be defined once
		if (global.fnameout) {
			snprintf(retmsg,160,"Error: only one output filename allowed\n");
			return(-1);
		}; // end if

		global.fnameout=argv[paramloop];

	}; // end else - elsif - if

}; // end for

// Done all parameters. Check if we have sufficient parameters

if (global.fileorcapture == -1) {
	snprintf(retmsg,160,"Error: input source missing.\nUsage: receiver [-i] [-d] [-dd] [-da] [-s] {-a alsadevice | -f inputfilename} outputfilename\n");
	return(-1);
}; // end if

if (!(global.fnameout)) {
	snprintf(retmsg,160,"Error: output file missing.\nUsage: receiver [-i] [-d] [-dd] [-da] [-s] {-a alsadevice | -f inputfilename} outputfilename\n");
	return(-1);
}; // end if


// All done, return
return(0);

}; // end function parse cli
