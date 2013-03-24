//////////////////////
// API version of the GMSK modem for 10m / VHF / UHF communication
// using codec2


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

// Version 20130314: initial release: API c2gmsk version / bitrate control + versionid codes
// Version 20130324: convert into .so shared library

// id codes that map a version-id of c2gmsk with a specific "code".
// this code (24 bits) is send just after the start-sync of the stream

// These codes are chosen to have a maximum average distance between
// all possible combination. At least a minimum distance of 8 is provided
unsigned int c2gmsk_idcode_4800[]={0x27F301, 0x1B933C, 0x778528, 0xACFE38, 0x065CE4, 0xC0E5DF, 0x9D8FBF, 0xF220E2, 0x684843, 0xB97AE2, 0xDD4499, 0x742B5D, 0x9E5C23, 0xC21096, 0x217FCE,0x5EA2D0};
unsigned int c2gmsk_idcode_2400[]={0xE02619, 0xEF7FCE, 0xA315C5, 0x51EF0B, 0x33F22A, 0x8AEB48, 0xE2A352, 0xB09535, 0x0844BC, 0x0FAEF2, 0x3D98F6, 0x9400B9, 0x7E6C91, 0x495126, 0xD6BA66, 0x1D499B};
