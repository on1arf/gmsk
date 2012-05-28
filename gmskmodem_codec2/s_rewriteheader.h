/* rewrite header */


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
// version 20120105: No change

// this is currently is hardcoded rewriting of the D-STAR configuration
// frame header

// uncomment and change the lines as needed.
// do not forget to uncomment the change-checksum line at the end

// this code will be made more dynamic in a later version of this
// software



int rewriteheader (str_configframe * configframe) {
	configframe->flags[0] |= 0x00;
	configframe->flags[1] |= 0x00;
	configframe->flags[2] |= 0x00;
	memcpy(&configframe->RPT2[0],"DIRECT  ",8);
	memcpy(&configframe->RPT1[0],"DIRECT  ",8);
	memcpy(&configframe->YOUR[0],"CQCQCQ  ",8);
	memcpy(&configframe->MY[0],"ON1ARF  ",8);
	memcpy(&configframe->MYEXT[0],"KRIS",4);

	configframe->checksum=htons(calc_fcs((unsigned char *) &configframe->flags[0], 39));
	return(0);
}; // end if
