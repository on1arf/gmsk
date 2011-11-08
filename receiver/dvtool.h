/* dvtool.h */

// structures for the .dvtool file format

// version 20111107: initial release

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



typedef struct {
	char headertext[6]; // "DVTOOL"
	uint32_t filesize; // LittleEndian
} str_dvtoolheader ; // end structure


typedef struct{
	uint16_t length; // 0x0038 (LittleEndian)
	char headertext[4]; // "DSVT"
	uint8_t configordata; // 0x10
	uint8_t unknown1[3]; // 0x00, 0x00, 0x00
	uint8_t dataorvoice; // 0x20
	uint8_t unknown2[3]; // 0x00, 0x01, 0x01
	uint16_t streamid;
	uint8_t framecounter; // 0x80
	uint8_t flags[3];
	char RPT2[8];
	char RPT1[8];
	char YOUR[8];
	char MY[8];
	char MYEXT[4];
	uint16_t checksum;
} str_configframe ; // end structure


typedef struct{
	uint16_t length; // 0x001b (Little Endian)
	char headertext[4]; // "DSVT"
	uint8_t configordata; // 0x20
	uint8_t unknown1[3]; // 0x00, 0x00, 0x00
	uint8_t dataorvoice; // 0x20
	uint8_t unknown2[3]; // 0x00, 0x01, 0x01
	uint16_t streamid;
	uint8_t framecounter; // goes up from 0 upto 20
	char ambe[9];
	char slowspeeddata[3];
} str_dvframe ; // end structure

