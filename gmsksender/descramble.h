/* descramble.h */

// Functions for processing the radio-header:
// descramble
// deinterleave
// FECdecoder
// FECencoder

// (C) 2011 Jonathan Naylor G4KLX

// This code was originally written by JOnathan Naylor, G4KLX, as part
// of the "pcrepeatercontroller" project
// More info:
// http://groups.yahoo.com/group/pcrepeatercontroller



// Changes:
// Convertion from C++ to C

// Version 20111106: initial release descramble, deinterleave, FECdecoder
// Version 20111213: initial release FECencoder and interleave




#include <stdio.h>
#include <string.h>

// function traceBack
int traceBack (int * out, int * m_pathMemory0, int * m_pathMemory1, int * m_pathMemory2, int * m_pathMemory3) {
	enum FEC_STATE { S0, S1, S2, S3 } state;
	int loop;
	int length=0;

	state=S0;

	for (loop=329; loop >= 0; loop--, length++) {

		switch (state) {
			case S0: // if state S0
				if (m_pathMemory0[loop]) {
					state = S2; // lower path
				} else {
					state = S0; // upper path
				}; // end else - if
				out[loop]=0;
				break;
			
			case S1: // if state S1
				if (m_pathMemory1[loop]) {
					state = S2; // lower path
				} else {
					state = S0; // upper path
				}; // end else - if
				out[loop]=1;
				break;
			
			case S2: // if state S2
				if (m_pathMemory2[loop]) {
					state = S3; // lower path
				} else {
					state = S1; // upper path
				}; // end else - if
				out[loop]=0;
				break;
			
			case S3: // if state S3
				if (m_pathMemory3[loop]) {
					state = S3; // lower path
				} else {
					state = S1; // upper path
				}; // end else - if
				out[loop]=1;
				break;
			
		}; // end switch
	}; // end for

return(length);
}; // end function



// function viterbiDecode

void viterbiDecode (int n, int *data, int *m_pathMemory0, int *m_pathMemory1, int *m_pathMemory2, int *m_pathMemory3, int *m_pathMetric) {
	int tempMetric[4];
	int metric[8];
	int loop;

	int m1;
	int m2;

	metric[0]=(data[1]^0)+(data[0]^0);
	metric[1]=(data[1]^1)+(data[0]^1);
	metric[2]=(data[1]^1)+(data[0]^0);
	metric[3]=(data[1]^0)+(data[0]^1);
	metric[4]=(data[1]^1)+(data[0]^1);
	metric[5]=(data[1]^0)+(data[0]^0);
	metric[6]=(data[1]^0)+(data[0]^1);
	metric[7]=(data[1]^1)+(data[0]^0);

	// Pres. state = S0, Prev. state = S0 & S2
	m1=metric[0]+m_pathMetric[0];
	m2=metric[4]+m_pathMetric[2];
	if (m1<m2) {
		m_pathMemory0[n]=0;
		tempMetric[0]=m1;
	} else {
		m_pathMemory0[n]=1;
		tempMetric[0]=m2;
	}; // end else - if

	// Pres. state = S1, Prev. state = S0 & S2
	m1=metric[1]+m_pathMetric[0];
	m2=metric[5]+m_pathMetric[2];
	if (m1<m2) {
		m_pathMemory1[n]=0;
		tempMetric[1]=m1;
	} else {
		m_pathMemory1[n]=1;
		tempMetric[1]=m2;
	}; // end else - if

	// Pres. state = S2, Prev. state = S2 & S3
	m1=metric[2]+m_pathMetric[1];
	m2=metric[6]+m_pathMetric[3];
	if (m1<m2) {
		m_pathMemory2[n]=0;
		tempMetric[2]=m1;
	} else {
		m_pathMemory2[n]=1;
		tempMetric[2]=m2;
	}

	// Pres. state = S3, Prev. state = S1 & S3
	m1=metric[3]+m_pathMetric[1];
	m2=metric[7]+m_pathMetric[3];
	if (m1 < m2) {
		m_pathMemory3[n]=0;
		tempMetric[3]=m1;
	} else {
		m_pathMemory3[n]=1;
		tempMetric[3]=m2;
	}; // end else - if

	for (loop=0;loop<4;loop++) {
		m_pathMetric[loop]=tempMetric[loop];
	}; // end for

}; // end function ViterbiDecode


// function FECdecoder 
// returns outlen
int FECdecoder (int * in, int * out) {
int outLen;

int m_pathMemory0[330]; memset(m_pathMemory0,0,330*sizeof(int));
int m_pathMemory1[330]; memset(m_pathMemory1,0,330*sizeof(int));
int m_pathMemory2[330]; memset(m_pathMemory2,0,330*sizeof(int));
int m_pathMemory3[330]; memset(m_pathMemory3,0,330*sizeof(int));
int m_pathMetric[4];

int loop,loop2;

int n=0;

for (loop=0;loop<4;loop++) {
	m_pathMetric[loop]=0;
}; // end for


for (loop2=0;loop2<660;loop2+=2, n++) {
	int data[2];

	if (in[loop2]) {
		data[1]=1;
	} else {
		data[1]=0;
	}; // end else - if

	if (in[loop2+1]) {
		data[0]=1;
	} else {
		data[0]=0;
	}; // end else - if

	viterbiDecode(n, data, m_pathMemory0, m_pathMemory1, m_pathMemory2, m_pathMemory3, m_pathMetric);
}; // end for

outLen=traceBack(out, m_pathMemory0, m_pathMemory1, m_pathMemory2, m_pathMemory3);

// Swap endian-ness
// code removed (done converting bits into octets), done in main program

//for (loop=0;loop<330;loop+=8) {
//	int temp;
//	temp=out[loop];out[loop]=out[loop+7];out[loop+7]=temp;
//	temp=out[loop+1];out[loop+1]=out[loop+6];out[loop+6]=temp;
//	temp=out[loop+2];out[loop+2]=out[loop+5];out[loop+5]=temp;
//	temp=out[loop+3];out[loop+3]=out[loop+4];out[loop+4]=temp;
//}

return(outLen);

}; // end function FECdecoder


// function deinterleave
void deinterleave (int * in, int * out) {

int k=0;
int loop=0;
// function starts here

// init vars
k=0;

for (loop=0;loop<660;loop++) {
	out[k]=in[loop];

	k += 24;

	if (k >= 672) {
		k -= 671;
	} else if (k >= 660) {
		k -= 647;
	}; // end elsif - if
}; // end for

}; // end function deinterleave



/// function scramble

void scramble (int * in,int * out) {

static const int SCRAMBLER_TABLE_BITS[] = {
	0,0,0,0,1,1,1,0,1,1,1,1,0,0,1,0,1,1,0,0,1,0,0,1,0,0,0,0,0,0,1,0,
	0,0,1,0,0,1,1,0,0,0,1,0,1,1,1,0,1,0,1,1,0,1,1,0,0,0,0,0,1,1,0,0,
	1,1,0,1,0,1,0,0,1,1,1,0,0,1,1,1,1,0,1,1,0,1,0,0,0,0,1,0,1,0,1,0,
	1,1,1,1,1,0,1,0,0,1,0,1,0,0,0,1,1,0,1,1,1,0,0,0,1,1,1,1,1,1,1,0,
	0,0,0,1,1,1,0,1,1,1,1,0,0,1,0,1,1,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,
	0,1,0,0,1,1,0,0,0,1,0,1,1,1,0,1,0,1,1,0,1,1,0,0,0,0,0,1,1,0,0,1,
	1,0,1,0,1,0,0,1,1,1,0,0,1,1,1,1,0,1,1,0,1,0,0,0,0,1,0,1,0,1,0,1,
	1,1,1,1,0,1,0,0,1,0,1,0,0,0,1,1,0,1,1,1,0,0,0,1,1,1,1,1,1,1,0,0,
	0,0,1,1,1,0,1,1,1,1,0,0,1,0,1,1,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,
	1,0,0,1,1,0,0,0,1,0,1,1,1,0,1,0,1,1,0,1,1,0,0,0,0,0,1,1,0,0,1,1,
	0,1,0,1,0,0,1,1,1,0,0,1,1,1,1,0,1,1,0,1,0,0,0,0,1,0,1,0,1,0,1,1,
	1,1,1,0,1,0,0,1,0,1,0,0,0,1,1,0,1,1,1,0,0,0,1,1,1,1,1,1,1,0,0,0,
	0,1,1,1,0,1,1,1,1,0,0,1,0,1,1,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,1,
	0,0,1,1,0,0,0,1,0,1,1,1,0,1,0,1,1,0,1,1,0,0,0,0,0,1,1,0,0,1,1,0,
	1,0,1,0,0,1,1,1,0,0,1,1,1,1,0,1,1,0,1,0,0,0,0,1,0,1,0,1,0,1,1,1,
	1,1,0,1,0,0,1,0,1,0,0,0,1,1,0,1,1,1,0,0,0,1,1,1,1,1,1,1,0,0,0,0,
	1,1,1,0,1,1,1,1,0,0,1,0,1,1,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,1,0,
	0,1,1,0,0,0,1,0,1,1,1,0,1,0,1,1,0,1,1,0,0,0,0,0,1,1,0,0,1,1,0,1,
	0,1,0,0,1,1,1,0,0,1,1,1,1,0,1,1,0,1,0,0,0,0,1,0,1,0,1,0,1,1,1,1,
	1,0,1,0,0,1,0,1,0,0,0,1,1,0,1,1,1,0,0,0,1,1,1,1,1,1,1,0,0,0,0,1,
	1,1,0,1,1,1,1,0,0,1,0,1,1,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,1,0,0,
	1,1,0,0,0,1,0,1,1,1,0,1,0,1,1,0,1,1,0,0,0,0,0,1,1,0,0,1,1,0,1,0,
	1,0,0,1,1,1,0,0,1,1,1,1,0,1,1,0};

const int SCRAMBLER_TABLE_BITS_LENGTH=720;

int loop=0;
int m_count=0;


for (loop=0; loop < 660; loop++) {
	out[loop] = in[loop] ^ SCRAMBLER_TABLE_BITS[m_count++];

	if (m_count >= SCRAMBLER_TABLE_BITS_LENGTH) {
		m_count = 0U;
	}; // end if
}; // end for

}; // end function scramble



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


void interleave (int * buffer_in, int * buffer_out) {

	static unsigned int INTERLEAVE_TABLE[] = {
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
	int * p;

	p=buffer_in;
	for (loop=0; loop < 660; loop++) {
		buffer_out[INTERLEAVE_TABLE[loop]]=*p;
		p++;	
	}; // end for
	
}; // end function interleave
