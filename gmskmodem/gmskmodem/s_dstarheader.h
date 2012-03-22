/* dstarheader.h */

// Functions for processing the d-star configuration-header:
// scramble
// interleave
// FECencoder

/*
 * Original Copyright:
 *      Copyright (C) 2010 by Jonathan Naylor, G4KLX
 *
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


// This code was originally written by JOnathan Naylor, G4KLX, as part
// of the "pcrepeatercontroller" project
// More info:
// http://groups.yahoo.com/group/pcrepeatercontroller



// Changes:
// Convertion from C++ to C

// Version 20111213: initial release scramble, FECencoder and interleave


#include <string.h>


/////////////////////
/// function scramble
void scramble_andconvertooctets (int * in, unsigned char * out) {


static unsigned char SCRAMBLE_TABLE_OCTETS[] = {
0x0e, 0xf2, 0xc9, 0x02, 0x26, 0x2e, 0xb6, 0x0c, 0xd4, 0xe7, 0xb4, 0x2a, 
0xfa, 0x51, 0xb8, 0xfe, 0x1d, 0xe5, 0x92, 0x04, 0x4c, 0x5d, 0x6c, 0x19,
0xa9, 0xcf, 0x68, 0x55, 0xf4, 0xa3, 0x71, 0xfc, 0x3b, 0xcb, 0x24, 0x08,
0x98, 0xba, 0xd8, 0x33, 0x53, 0x9e, 0xd0, 0xab, 0xe9, 0x46, 0xe3, 0xf8,
0x77, 0x96, 0x48, 0x11, 0x31, 0x75, 0xb0, 0x66, 0xa7, 0x3d, 0xa1, 0x57, 
0xd2, 0x8d, 0xc7, 0xf0, 0xef, 0x2c, 0x90, 0x22, 0x62, 0xeb, 0x60, 0xcd,
0x4e, 0x7b, 0x42, 0xaf, 0xa5, 0x1b, 0x8f, 0xe1, 0xde, 0x59, 0x20
};// end table

// Note, the SCRAMBLE_TABLE_OCTETS is 83 octets (664 bits), but we only need
// 660 of them (82.5 octets)


int loop=0, loop2=0;
unsigned char *p_out, *p_table;
int * p_in;
unsigned char c;

// init pointers;
p_in=in; p_out=out; p_table=SCRAMBLE_TABLE_OCTETS;

// convert bits to octets
loop2=0;
c=0x00;

for (loop=0; loop < 660; loop++) {
	// set right bit if bit is set
	if (*p_in) {
		c |= 1;
	}; 

	// move up input pointer
	p_in++;

	// left shift character if not at the end
	if ((loop2 < 7) && (loop < 659)){
		loop2++;
		c <<= 1;
	} else {
		if (loop == 659) {
			// if last octet, only 4 bits filled up, shift up 4 move
			c <<= 4;
		}; // end if

		// c is completely full

		// apply scrambling
		c ^= *p_table;

		// save c and re-init
		*p_out = c;
		c=0x00;
		loop2=0;

		// move up all pointers
		p_out++;
		p_table++;
	}; // end else - if

}; // end for


}; // end function scramble and convert to octets



///////////////////////
/// function FECencoder
int FECencoder (int * buffer_in, int * buffer_out) {
	// local vars
	int loop;
	int d;

	int d1 = 0;
	int d2 = 0;

	// pointers
	int * p1, *p2;

	// init vars
	d1=0; d2=0;
	p1=buffer_in; p2=buffer_out;

	for (loop=0;loop < 330; loop++) {
		d = *p1;
		*p2 = (d + d1 + d2) % 2; p2++;
		*p2 = (d + d2) % 2; p2++;

		// keep 2 last vars
		d2 = d1; d1 = d;

		p1++;
	}

return(0);
}; // end function FECencode


///////////////////////
/// function interleave
void interleave (int * buffer_in, int * buffer_out) {

static int INTERLEAVE_TABLE[] = {
0, 28, 56, 84, 112, 140, 168, 196, 224, 252, 280, 308, 336, 363, 390, 417, 444,
471, 498, 525, 552, 579, 606, 633, 1, 29, 57, 85, 113, 141, 169, 197, 225, 253,
281, 309, 337, 364, 391, 418, 445, 472, 499, 526, 553, 580, 607, 634, 2, 30, 58,
86, 114, 142, 170, 198, 226, 254, 282, 310, 338, 365, 392, 419, 446, 473, 500,
527, 554, 581, 608, 635, 3, 31, 59, 87, 115, 143, 171, 199, 227, 255, 283, 311,
339, 366, 393, 420, 447, 474, 501, 528, 555, 582, 609, 636, 4, 32, 60, 88, 116,
144, 172, 200, 228, 256, 284, 312, 340, 367, 394, 421, 448, 475, 502, 529, 556,
583, 610, 637, 5, 33, 61, 89, 117, 145, 173, 201, 229, 257, 285, 313, 341, 368,
395, 422, 449, 476, 503, 530, 557, 584, 611, 638, 6, 34, 62, 90, 118, 146, 174,
202, 230, 258, 286, 314, 342, 369, 396, 423, 450, 477, 504, 531, 558, 585, 612,
639, 7, 35, 63, 91, 119, 147, 175, 203, 231, 259, 287, 315, 343, 370, 397, 424,
451, 478, 505, 532, 559, 586, 613, 640, 8, 36, 64, 92, 120, 148, 176, 204, 232,
260, 288, 316, 344, 371, 398, 425, 452, 479, 506, 533, 560, 587, 614, 641, 9,
37, 65, 93, 121, 149, 177, 205, 233, 261, 289, 317, 345, 372, 399, 426, 453, 480,
507, 534, 561, 588, 615, 642, 10, 38, 66, 94, 122, 150, 178, 206, 234, 262, 290,
318, 346, 373, 400, 427, 454, 481, 508, 535, 562, 589, 616, 643, 11, 39, 67, 95,
123, 151, 179, 207, 235, 263, 291, 319, 347, 374, 401, 428, 455, 482, 509, 536,
563, 590, 617, 644, 12, 40, 68, 96, 124, 152, 180, 208, 236, 264, 292, 320, 348,
375, 402, 429, 456, 483, 510, 537, 564, 591, 618, 645, 13, 41, 69, 97, 125, 153,
181, 209, 237, 265, 293, 321, 349, 376, 403, 430, 457, 484, 511, 538, 565, 592,
619, 646, 14, 42, 70, 98, 126, 154, 182, 210, 238, 266, 294, 322, 350, 377, 404,
431, 458, 485, 512, 539, 566, 593, 620, 647, 15, 43, 71, 99, 127, 155, 183, 211,
239, 267, 295, 323, 351, 378, 405, 432, 459, 486, 513, 540, 567, 594, 621, 648,
16, 44, 72, 100, 128, 156, 184, 212, 240, 268, 296, 324, 352, 379, 406, 433, 460,
487, 514, 541, 568, 595, 622, 649, 17, 45, 73, 101, 129, 157, 185, 213, 241, 269,
297, 325, 353, 380, 407, 434, 461, 488, 515, 542, 569, 596, 623, 650, 18, 46, 74,
102, 130, 158, 186, 214, 242, 270, 298, 326, 354, 381, 408, 435, 462, 489, 516,
543, 570, 597, 624, 651, 19, 47, 75, 103, 131, 159, 187, 215, 243, 271, 299, 327,
355, 382, 409, 436, 463, 490, 517, 544, 571, 598, 625, 652, 20, 48, 76, 104, 132,
160, 188, 216, 244, 272, 300, 328, 356, 383, 410, 437, 464, 491, 518, 545, 572,
599, 626, 653, 21, 49, 77, 105, 133, 161, 189, 217, 245, 273, 301, 329, 357, 384,
411, 438, 465, 492, 519, 546, 573, 600, 627, 654, 22, 50, 78, 106, 134, 162, 190,
218, 246, 274, 302, 330, 358, 385, 412, 439, 466, 493, 520, 547, 574, 601, 628,
655, 23, 51, 79, 107, 135, 163, 191, 219, 247, 275, 303, 331, 359, 386, 413, 440,
467, 494, 521, 548, 575, 602, 629, 656, 24, 52, 80, 108, 136, 164, 192, 220, 248,
276, 304, 332, 360, 387, 414, 441, 468, 495, 522, 549, 576, 603, 630, 657, 25, 53,
81, 109, 137, 165, 193, 221, 249, 277, 305, 333, 361, 388, 415, 442, 469, 496, 523,
550, 577, 604, 631, 658, 26, 54, 82, 110, 138, 166, 194, 222, 250, 278, 306, 334,
362, 389, 416, 443, 470, 497, 524, 551, 578, 605, 632, 659, 27, 55, 83, 111, 139,
167, 195, 223, 251, 279, 307, 335};

int loop;
int * p_in, *p_table;

p_in=buffer_in; p_table = INTERLEAVE_TABLE;

for (loop=0; loop < 660; loop++) {
	buffer_out[*p_table]=*p_in;
	p_in++; p_table++;	
}; // end for

}; // end function interleave
