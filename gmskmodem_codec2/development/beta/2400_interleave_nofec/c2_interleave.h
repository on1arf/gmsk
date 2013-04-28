// interleave.h

// functions and datastructures for interleaving
// interleaving and deinterleaving table
// 2400 bps modem, 1400 bps voice codec, NO SYNC present (96 bits)
int c2_interleavetable_2400_1400_nosync [] = {
32, 57,  0,  1,  2, 27, 28, 29,
54, 55, 10, 35, 48, 49, 50,  3,
 4,  5, 30, 31,  8,  6, 76, 85,
 9,  7, 77, 86, 58, 56, 78, 87,
11, 33, 79, 88, 24, 34, 80, 89,
25, 59, 81, 90, 26, 72, 82, 91,
51, 73, 83, 52, 74, 84, 53, 75
}; // end 

int c2_revinterleavetable_2400_1400_nosync [] = {
30, 18, 22, 23,  8,  3,  4,  5,
-1, -1, -1, -1, 39, 13, 31, 19,
-1, -1, -1, -1, -1, -1, -1, -1,
20, 21,  0,  1,  2, 43, 47, 35,
-1, -1, -1, -1, 12, 34, 38,  7,
-1, -1, -1, -1, -1, -1, -1, -1,
14, 15, 49, 52, 55,  9, 10, 11,
-1, -1, -1, -1, 46, 27,  6, 26,
-1, -1, -1, -1, -1, -1, -1, -1,
37, 25, 29, 17, 48, 51, 54, 42,
24, 28, 16, 50, 53, 41, 45, 33,
-1, -1, -1, -1, 40, 44, 32, 36
}; // end


// 2400 bps modem, 1400 bps voice codec, SYNC present (72 bits)
int c2_interleavetable_2400_1400_sync [] = {
 8,  9,  0,  1,  2,  3,  4,  5,
 6,  7, 34, 35, 36, 37, 38, 39,
40, 41, 42, 43, 10, 32, 48, 57,
11, 33, 49, 58, 24, 64, 50, 59,
25, 65, 51, 60, 26, 66, 52, 61,
27, 67, 53, 62, 28, 68, 54, 63,
29, 69, 55, 30, 70, 56, 31, 71
}; // end



int c2_revinterleavetable_2400_1400_sync [] = {
14, 15,  0,  1,  2,  3,  4  ,5,
-1, -1, -1, -1, 31, 19,  6,  7,
-1, -1, -1, -1, -1, -1, -1, -1,
49, 52, 55, 43, 47, 35, 39, 27,
 8,  9, 10, 11, 12, 13, 30, 18,
-1, -1, -1, -1, 20, 21, 22, 23,
53, 41, 45, 33, 37, 25, 29, 17,
40, 44, 32, 36, 24, 28, 16, 50,
48, 51, 54, 42, 46, 34, 38, 26
};


#define INTERLEAVETABLESIZE_BYTES_FORW_1400 7
#define INTERLEAVETABLESIZE_BYTES_REV_1400_NOSYNC 12 
#define INTERLEAVETABLESIZE_BYTES_REV_1400_SYNC 9 


void c2_interleave_2400_1400 (uint8_t * intable, uint8_t * outtable, int sync, int forward) {
int loop,loop2,tmp;
uint8_t i; // input char
uint8_t *p_intable;
int * interleavetable;
int tablesize=0;

// forward: 1 = forward, 0 = reverse
if (forward) {
	// "sync" or "no sync" table?
	if (sync) {
		interleavetable=c2_interleavetable_2400_1400_sync;

		// clear output table 72 bits = 9 octets)
		memset(outtable,0,9);
	} else {
		interleavetable=c2_interleavetable_2400_1400_nosync;

		// clear output table 96 bits = 12 octets)
		memset(outtable,0,12);
	}; // end if

	tablesize=INTERLEAVETABLESIZE_BYTES_FORW_1400;

	// forward: mask = 0x80
} else {
	// "sync" or "no sync" table?
	if (sync) {
		interleavetable=c2_revinterleavetable_2400_1400_sync;
		tablesize=INTERLEAVETABLESIZE_BYTES_REV_1400_SYNC;
	} else {
		interleavetable=c2_revinterleavetable_2400_1400_nosync;
		tablesize=INTERLEAVETABLESIZE_BYTES_REV_1400_NOSYNC;
	}; // end if

	// clear out table, always 7 octets
	memset(outtable,0,7);
}; // end if


p_intable=intable;

for (loop=0; loop<tablesize; loop++) {
	i=*p_intable;
//fprintf(stderr,"i = %02X \n",i);
	for (loop2=0; loop2<8; loop2++) {
		// read bits from MSB to LSB
		if (i & 0x80) {
			// set bit in outtable to 1
			tmp=*interleavetable;
//fprintf(stderr,"%d %d %d %d %d \n",loop,loop2,tmp,tmp>>3, tmp & 0x07);

			// only set data when there is a valid pointer
			if (tmp >= 0) {
				outtable[(tmp>>3)]  |= bit2octet[tmp & 0x07];
			}; // end if
		}; // end if

		// move up or down all bits
		i<<=1;

		// move up in interleaver table
		interleavetable++;
	}; // end for

	// done for this octet, move up in uptable
	p_intable++;
}; // end for

}; // end function


//// function octetreorder_tx
int transmitorder_nosync [] = {
 0,  3,  6,  9,
 1,  4,  7, 10,
 2,  5,  8, 11
}; 

int transmitorder_sync [] = {
 3,  6,  9,
 4,  7,  10,
 5,  8,  11
}; 

// reorder bits for transmission order TRANSMIT
void interleaveTX_tx (unsigned char * in, unsigned char * out, int withsync) {
int loop1, loop2;
int *p; // pointer to transmitorder table
unsigned char *out_pointer;
int outbit_counter; // output bit counter

int numoctet; // number of octets; can be 9 (if with sync) or 12 (if no sync pattern present)
uint8_t mask_out=0x01; // we start with bit on right
uint8_t mask_in;
outbit_counter=0;


// if sync pattern is present, out is only 9 octets
if (withsync) {
	numoctet=9;

	// copy sync pattern (24 bits = 3 octets)
	memcpy(out,in,3); // 

	// set "out" octet pointer to start of out buffer
	out_pointer = &out[3];

	// clear output buffer
	memset(out_pointer, 0, 9);

} else {
	numoctet=12;

	// set "out" octet pointer to start ofstart of out buffer
	out_pointer = out;

	// clear output buffer
	memset(out, 0, 12);
}


// init mask
mask_in = 0x01;

for (loop1=0; loop1<8; loop1++) {
	// set pointer to beginning to transmitordertable;
	if (withsync) {
		p=transmitorder_sync;
	} else {
		p=transmitorder_nosync;
	}; // end if

	for (loop2=0; loop2<numoctet; loop2++) {
		// set "out" bit if "in" bit set
		if (in[*p] & mask_in) {
			*out_pointer |= mask_out;
		}; // end if


		outbit_counter++;
		// 8 or more?
		if (outbit_counter >= 8) {
			outbit_counter=0;
			// reset mask
			mask_out=0x01;

			// move up "out octet" pointer
			out_pointer++;
		} else {
			// move "out" mask to left
			mask_out <<= 1;
		}; // end if

		// move up pointer in transmissionorder table
		p++;
	}; // end for (loop2)

// move  "in" mask to left
mask_in <<= 1;
}; // end for (loop)

return;
}; // end function octetreorder_tx

// reorder bits for transmission order  RECEIVE
void interleaveTX_rx (unsigned char * in, unsigned char * out, int withsync) {
int loop1, loop2;
int *p; // pointer to transmitorder table
unsigned char *in_pointer;
int outbit_counter; // output bit counter

int numoctet; // number of octets; can be 9 (if with sync) or 12 (if no sync pattern present)
uint8_t mask_out; // we start with bit on right
uint8_t mask_in;
outbit_counter=0;

// if sync pattern is present, out is only 9 octets
if (withsync) {
	numoctet=9;

	// copy sync pattern (24 bits = 3 octets)
	memcpy(out,in,3); // 

	// set "out" octet pointer to start of out buffer
	in_pointer = &in[3];

	// clear output buffer
	memset(&out[3], 0, 9);

} else {
	numoctet=12;

	// set "out" octet pointer to start ofstart of out buffer
	in_pointer = in;

	// clear output buffer
	memset(out, 0, 12);

}

// set pointer to beginning to transmitordertable;
if (withsync) {
	p=transmitorder_sync;
} else {
	p=transmitorder_nosync;
}; // end if


// init mask_out
mask_out = 0x01; // we start with bit on right

for (loop1=0; loop1<numoctet; loop1++) {

	// (re)init mask_in
	mask_in = 0x01;

	for (loop2=0; loop2<8; loop2++) {
//fprintf(stderr,"%d %d in = %02X, mask = %02X, p = %d, mask_out = %02X \n",loop1, loop2, *in_pointer, mask_in, *p, mask_out);
		if (*in_pointer & mask_in) {
			out[*p] |= mask_out;
		}; // end if


		// move up "in" mask
		mask_in <<= 1;


		// move up "out" 
		outbit_counter++;

		// more then number of octets?
		if (outbit_counter >= numoctet) {
			// reset counter
			outbit_counter=0;

			// reset pointer to beginning of table
			if (withsync) {
				p=transmitorder_sync;
			} else {
				p=transmitorder_nosync;
			}; // end if

			// move up output bit mask
			// mask
			mask_out <<=1;

		} else {
			// move "out" mask to left
			p++;
		}; // end if

	}; // end for (loop2)

	// move up in_pointer
	in_pointer++;

}; // end for (loop)

return;
}; // end function octetreorder_rx
