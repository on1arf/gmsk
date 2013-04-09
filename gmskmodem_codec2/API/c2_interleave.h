//////////////////////
// API version of the GMSK modem for 10m / VHF / UHF communication
// using codec2
// version 0 (versionid 0x27f301): 4800 bps, 1/3 repetition code FEC


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


// Release information
// version 20130310 initial release
// Version 20130314: API c2gmsk version / bitrate control + versionid codes
// Version 20130324: convert into .so shared library

// interleave_c2.h

// CODEC2 octet interleaving for 4800 bps, changes order of 2nd and 3th octet in DV pattern
// Used to avoid repeating the same 7-octet char-pattern which results in harmonics and interference

const int interl2[7]= { 15, 14, 13, 12, 11, 10, 16}; // reverse order, shift one to the left, then add 7 (2nd group) + 3 (get past sync)
const int interl3[7]= { 18, 22, 17, 21, 23, 20, 19}; // repeat {+4 , +2}, move up if already used at that position in octet 1 or 2; then add 14 (3th octet) + 3 (get past sync)
