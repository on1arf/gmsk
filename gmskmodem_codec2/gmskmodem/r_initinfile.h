/* initinfile.h */

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
// 20120105: initial release



int init_infile (char * retmsg) {
// init
r_global.filein=fopen(r_global.fnamein,"r");

if (!r_global.filein) {
	snprintf(retmsg,160,"Error: input file %s could not be opened!\n",r_global.fnamein);
	return(-1);
}; 

// all OK
return(0);
}; // end if
