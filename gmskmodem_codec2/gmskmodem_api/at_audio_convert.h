// samplerate conversion tools


// based on fdmdv_8_to_48 and fdmdv_48_to_8
// written by David Rowe 


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


// version 20130326 initial release
// version 20130404: added half duplex support



// original notice
/*---------------------------------------------------------------------------*\
                                                       
  FUNCTION....: fdmdv_8_to_48()      
  AUTHOR......: David Rowe                            
  DATE CREATED: 9 May 2012

  Changes the sample rate of a signal from 8 to 48 kHz.  Experience
  with PC based modems has shown that PC sound cards have a more
  accurate sample clock when set for 48 kHz than 8 kHz.

  n is the number of samples at the 8 kHz rate, there are FDMDV_OS*n samples
  at the 48 kHz rate.  A memory of FDMDV_OS_TAPS/FDMDV_OS samples is reqd for
  in8k[] (see t48_8.c unit test as example).

  This is a classic polyphase upsampler.  We take the 8 kHz samples
  and insert (FDMDV_OS-1) zeroes between each sample, then
  FDMDV_OS_TAPS FIR low pass filter the signal at 4kHz.  As most of
  the input samples are zeroes, we only need to multiply non-zero
  input samples by filter coefficients.  The zero insertion and
  filtering are combined in the code below and I'm too lazy to explain
  it further right now....

\*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*\
                                                       
  FUNCTION....: fdmdv_48_to_8()      
  AUTHOR......: David Rowe                            
  DATE CREATED: 9 May 2012

  Changes the sample rate of a signal from 48 to 8 kHz.
 
  n is the number of samples at the 8 kHz rate, there are FDMDV_OS*n
  samples at the 48 kHz rate.  As above however a memory of
  FDMDV_OS_TAPS samples is reqd for in48k[] (see t48_8.c unit test as example).

  Low pass filter the 48 kHz signal at 4 kHz using the same filter as
  the upsampler, then just output every FDMDV_OS-th filtered sample.

\*---------------------------------------------------------------------------*/


// some defines
#define FDMDV_OS                 6         /* oversampling rate           */
#define FDMDV_OS_TAPS           48         /* number of OS filter taps    */

// convert 8k to 48k
void convert_8kto48k ( int16_t *in, int16_t *out, int reset) {

// local storage for floating point operations
static float in_f[328]; // note: 328 elements: 8 last samples of previous audio frame + 320 samples of new frame
float out_f[1920];


// local vars
int i, j, k;
static int init=1;

int16_t * p_i; float * p_f; // vars used to convert integer to float and visa-versa
float *p_in, *p_out, *p_filter; // vars used in filter operation


// INITIALISATION, run first time function is called
if ((init) || (reset)){
	// pre-initiase 8 octets in buffer of previous frame
	memset(in_f,0,8 * sizeof(float));

	init=0;
}; // end if

// convert integer to float
// samples 0 upto 7 contain last samples of previous frame, already copied
// samples 8 upto 327 contain new frame
p_i=in; p_f=&in_f[8];
for (i=8; i < 327; i++) {
	*p_f=(float)*p_i;

	// move up pointers
	p_f++; p_i++;
}; // end if

*p_f=(float)*p_i; // last element



// ORIGINAL CODE by David Rowe to convert 20 ms of audio
// calling function
//    for(i=0; i<160; i++)
//        in8k[8+i] = in8k_short[i];
//    fdmdv_8_to_48(out48k, &in8k[8], 160);
//    for(i=0; i<8; i++)
//        in8k[i] = in8k[i+160];
// (...)
// function fdmdv_8_to_48:
// for(i=0; i<160; i++) {
//  for(j=0; j<6; j++) {
//   out48k[i*6+j] = 0.0;
//		l=0;
//    for(k=0; k<48; k+=6) {
//     out48k[i*6+j] += fdmdv_os_filter[k+j]*in8k[i-l];
//     l++
//		}; // end for (k)
//    out48k[i*6+j] *= 6;
//   }; // end for (j)
//  }; // end for (i)   


// set pointer to output array, start with sample 0
p_out=out_f;

// preset all elements "out" to zero
memset(p_out,0,1920*sizeof(float));


// note: we start with 8th sample in the input array as we count back -7 to relative possition of start
for (i=8; i<328; i++) {
	for (j=0; j < 6; j++) {
		// set filter pointer to correct place in filter
		p_filter=(float *) &fdmdv_os_filter[j];

		// p_in pointer start at position "i" in input, and counts BACK
		p_in=&in_f[i];

		for (k=0; k < 8; k++) {
			*p_out += *p_filter * *p_in;

			// move up filter 6 places (all 5 other values in input array are zero
			//									anyway, so there is no need to multiply) 
			p_filter += 6;

			// move down input pointer
			p_in--;
		}; // end for (k)

		*p_out *= 6;

		// move up output array pointer
		p_out++;
	}; // end for (j)
}; // end for (i)



// convert output floats to integers
p_i=out; p_f=out_f;
for (i=0; i < 1919; i++) {
	*p_i=(int16_t)*p_f;

	// move up pointers
	p_f++; p_i++;
}; // end if

*p_i=(int16_t)*p_f; // last element


// copy last 8 octets of input current frame to beginning of buffer, to be used for next frame
memcpy(in_f, &in_f[320], 8 * sizeof(float));

// done
return;
}; // end function convert 8k to 48 k


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////

// convert 48k to 8k
void convert_48kto8k ( int16_t *in, int16_t *out, int reset) {

// local storage for floating point operations
static float in_f[1968]; // 1920 + 48
float out_f[320];

// local vars
int i, j;
static int init=1;

int16_t * p_i; float * p_f; // vars used to convert integers into float and visa-versa

float *p_in, *p_out, *p_filter; // vars used in filter



// INITIALISATION, run first time function is called
if ((init) || (reset)){
	// pre-initiase 8 octets in buffer of previous frame
	memset(in_f,0,48 * sizeof(float));

	init=0;
}; // end if


// convert integer to float
// samples 0 upto 47 contain last samples of previous frame, already copied
// samples 48 to 1967 contain samples of last frame
p_i=in; p_f=&in_f[48];
for (i=48; i < 1967; i++) {
	*p_f=(float)*p_i;

	// move up pointers
	p_f++; p_i++;
}; // end if

*p_f=(float)*p_i; // last element


// preset all elements "out" to zero
memset(out_f,0,320*sizeof(float));


// ORIGINAL CODE by David Rowe to convert 20 ms of audio
// calling function
//  fdmdv_48_to_8(out8k, &in48k[48], 960);
//  for(i=0; i<48; i++)
//    in48k[i] = in48k[i+960];
// (...)
// function fdmdv_48_to_8:
// for(i=0; i<960; i++) {
//   out8k[i] = 0.0;
//   for(j=0; j<48; j++)
//    out8k[i] += fdmdv_os_filter[j]*in48k[i*6-j];
//  }


// set pointer to output array
p_out=out_f;
p_in=&in_f[48]; // p_in pointer start at position 48 in input, and counts BACK 48 times / sample

// note: we start with 49th sample in input array and go up 320 frames
for (i=48; i<368; i++) {
	// set filter pointer to correct place in filter
	p_filter = (float *)fdmdv_os_filter;

	for (j=0; j < 48; j++) {
		*p_out += *p_filter * *p_in;

		p_filter++;
		p_in--;

	}; // end for (j)

	// move up "out" pointer
	p_out++;

	p_in += 54; // original function is "out8k[i] = ... in48k[i*6-j]" , so undo the 48 times p_in-- and also add 6
}; // end for (i)


// copy last 48 octets of input current frame to beginning of buffer, to be used for next frame
memcpy(in_f, &in_f[1920], 48 * sizeof(float) );

// convert output floats to integers
p_i=out; p_f=out_f;
for (i=0; i < 319; i++) {
	*p_i=(int16_t)*p_f;

	// move up pointers
	p_f++; p_i++;
}; // end if

*p_i=(int16_t)*p_f; // last element


// done
return;
}; // end function convert 48k to 8 k

