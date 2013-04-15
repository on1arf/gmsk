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




// function "keyin"
// detects <enter> to switch "on" or "off" status
// started as a subthread
void * funct_ptt_keyin (void * globalin) {
globaldatastr * pglobal;
pglobal=(globaldatastr *) globalin;

// local vars
struct termios tio;
int stop;


// toggle "transmit" flag based on enter begin pushed

// first disable echo on local console
// get termiod struct for stdin
tcgetattr(0,&tio);
// disable echo
tio.c_lflag &= (~ECHO);
// write change to console
tcsetattr(0,TCSANOW,&tio);


fprintf(stderr,"press <enter> to switch on or off\n");
fprintf(stderr,"press \"x\" to stop\n");
fprintf(stderr,"\n");

fprintf(stderr,"audio is OFF! \n");

stop=0;
while (!stop) {
	int c;

	c=getchar();

	// is it lf?
	if (c == 0x0a) {
		if (pglobal->transmit) {
			// ON to OFF
			fprintf(stderr,"audio is OFF! \n");
			pglobal->transmit=0;
		} else {
			// OFF to ON
			fprintf(stderr,"audio is ON! \n");
			pglobal->transmit=1;
		}; // end if

	} else if ((c == 'x') || (c == 'X')) {
		fprintf(stderr,"STOP! \n");
		stop=1;
	}; // end else - if

}; // end while

// reenable ECHO

// reenable echo on local console
// get termiod struct for stdin
tcgetattr(0,&tio);
// disable echo
tio.c_lflag |= (ECHO);
// write change to console
tcsetattr(0,TCSANOW,&tio);

// exit application
global.transmit=-1;
exit(-1);

}; // end function funct_audioin
///////////////////////////////////////////////////////////////////////

