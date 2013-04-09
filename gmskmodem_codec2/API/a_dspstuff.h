// DSP related functions and data //
// "API" version

/*
 *      Original copyright:
 *      Copyright (C) 2011 by Jonathan Naylor, G4KLX
 *
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

// These is the low-level DSP functions that make up the gmsk demodulator:
// firfilter
// modulate
// demodulate

// This code is largely based on the code from the "pcrepeatercontroller"
// project, written by Jonathan Naylor 
// More info: 
// http://groups.yahoo.com/group/pcrepeatercontroller



// Changes:
// Convert FIR-filter to integer
// Change PLL values so to make PLLINC and SMALLPLLINC match integer-boundaries
// Concert C++ to C


// Version 20111106: initial release demodulation
// Version 20111213: initial release modulation
// Version 20120105: no change
// Version 20130228: modified for API: local vars moved to session
// Version 20130314: API c2gmsk version / bitrate control + versionid codes
// Version 20130324: convert into .so shared library



// global data


#if _USEFLOAT == 1

float coeffs_table_modulate [] = {
        6.455906007234699e-014F, 1.037067381285011e-012F, 1.444835156335346e-011F,
        1.745786683011439e-010F, 1.829471305298363e-009F, 1.662729407135958e-008F,
        1.310626978701910e-007F, 8.959797186410516e-007F, 5.312253663302771e-006F,
        2.731624380156465e-005F, 1.218217140199093e-004F, 4.711833994209542e-004F,
        1.580581180127418e-003F, 4.598383433830095e-003F, 1.160259430889949e-002F,
        2.539022692626253e-002F, 4.818807833062393e-002F, 7.931844341164322e-002F,
        1.132322945270602e-001F, 1.401935338024111e-001F, 1.505383695578516e-001F,
        1.401935338024111e-001F, 1.132322945270601e-001F, 7.931844341164328e-002F,
        4.818807833062393e-002F, 2.539022692626253e-002F, 1.160259430889949e-002F,
        4.598383433830090e-003F, 1.580581180127420e-003F, 4.711833994209542e-004F,
        1.218217140199093e-004F, 2.731624380156465e-005F, 5.312253663302753e-006F,
        8.959797186410563e-007F, 1.310626978701910e-007F, 1.662729407135958e-008F,
        1.829471305298363e-009F, 1.745786683011426e-010F, 1.444835156335356e-011F,
        1.037067381285011e-012F, 6.455906007234699e-014F};


float coeffs_table_demodulate [] = {
        -0.000153959924563F,  0.000000000000000F,  0.000167227768379F,  0.000341615513437F,
         0.000513334449696F,  0.000667493753523F,  0.000783901543032F,  0.000838293462576F,
         0.000805143268199F,  0.000661865814384F,  0.000393913058926F, -0.000000000000000F,
        -0.000503471198655F, -0.001079755887508F, -0.001671728086040F, -0.002205032425392F,
        -0.002594597675000F, -0.002754194565297F, -0.002608210441859F, -0.002104352817854F,
        -0.001225654870420F,  0.000000000000000F,  0.001494548041184F,  0.003130012785731F,
         0.004735238379172F,  0.006109242742194F,  0.007040527007323F,  0.007330850462455F,
         0.006821247169795F,  0.005417521811131F,  0.003112202160626F, -0.000000000000000F,
        -0.003715739376345F, -0.007727358782391F, -0.011638713107503F, -0.014992029537478F,
        -0.017304097563429F, -0.018108937286588F, -0.017003180218569F, -0.013689829477969F,
        -0.008015928769710F,  0.000000000000000F,  0.010154104792614F,  0.022059114281395F,
         0.035162729807337F,  0.048781621388364F,  0.062148583345584F,  0.074469032280094F,
         0.084982001723750F,  0.093020219991183F,  0.098063819576269F,  0.099782731268437F,
         0.098063819576269F,  0.093020219991183F,  0.084982001723750F,  0.074469032280094F,
         0.062148583345584F,  0.048781621388364F,  0.035162729807337F,  0.022059114281395F,
         0.010154104792614F,  0.000000000000000F, -0.008015928769710F, -0.013689829477969F,
        -0.017003180218569F, -0.018108937286588F, -0.017304097563429F, -0.014992029537478F,
        -0.011638713107503F, -0.007727358782391F, -0.003715739376345F, -0.000000000000000F,
         0.003112202160626F,  0.005417521811131F,  0.006821247169795F,  0.007330850462455F,
         0.007040527007323F,  0.006109242742194F,  0.004735238379172F,  0.003130012785731F,
         0.001494548041184F,  0.000000000000000F, -0.001225654870420F, -0.002104352817854F,
        -0.002608210441859F, -0.002754194565297F, -0.002594597675000F, -0.002205032425392F,
        -0.001671728086040F, -0.001079755887508F, -0.000503471198655F, -0.000000000000000F,
         0.000393913058926F,  0.000661865814384F,  0.000805143268199F,  0.000838293462576F,
         0.000783901543032F,  0.000667493753523F,  0.000513334449696F,  0.000341615513437F,
         0.000167227768379F,  0.000000000000000F, -0.000153959924563F
};

const int firstnonzero_modulate=0; // use complete table
const int lastnonzero_modulate=40; // use complete table

#else
	#if _INTMATH == 64
		// PART1 : DEMODULATION DATA: 32 bits used of 32 bits
		int32_t coeffs_table_demodulate [] = {
		-330625, 0, 359119, 733614, 1102377, 1433432, 
		1683416, 1800222, 1729032, 1421346, 845922, 0, 
		-1081195, -2318757, -3590008, -4735270, -5571855, -5914587, 
		-5601089, -4519063, -2632073, 0, 3209518, 6721652, 
		10168847, 13119499, 15119417, 15742881, 14648517, 11634040, 
		6683403, 0, -7979489, -16594376, -24993945, -32195137, 
		-37160267, -38888647, -36514051, -29398683, -17214075, 0, 
		21805774, 47371588, 75511384, 104757736, 133463064, 159921024, 
		182497456, 199759408, 210590448, 214281776, 210590448, 199759408, 
		182497456, 159921024, 133463064, 104757736, 75511384, 47371588, 
		21805774, 0, -17214075, -29398683, -36514051, -38888647, 
		-37160267, -32195137, -24993945, -16594376, -7979489, 0, 
		6683403, 11634040, 14648517, 15742881, 15119417, 13119499, 
		10168847, 6721652, 3209518, 0, -2632073, -4519063, 
		-5601089, -5914587, -5571855, -4735270, -3590008, -2318757, 
		-1081195, 0, 845922, 1421346, 1729032, 1800222, 
		1683416, 1433432, 1102377, 733614, 359119, 0, 
		-330625
		}; // end table


		// PART 2: MODULATION data: 32 bits used of 32 bits
		int32_t coeffs_table_modulate [] = {
		0, 0, 0, 
		0, 4, 36, 
		281, 1924, 11408, 
		58661, 261610, 1011859, 
		3394272, 9874953, 24916382, 
		54525096, 103483112, 170335056, 
		243164496, 301063328, 323278688, 
		301063328, 243164496, 170335056, 
		103483112, 54525096, 24916382, 
		9874953, 3394272, 1011859, 
		261610, 58661, 11408, 
		1924, 281, 36, 
		4, 0, 0, 
		0,0
		}; // end table

const int firstnonzero_modulate=4; // element 4 = 5th element in table
const int lastnonzero_modulate=36; // element 36 = 37th element in table


#else
	#if _INTMATH == 3212
		// PART1 : DEMODULATION DATA: 20 bits used of 32 bits
		int32_t coeffs_table_demodulate [] = {
		-160, 0, 175, 358, 538, 700, 822, 879,
		844, 694, 413, 0, -527, -1131, -1752,
		-2311, -2720, -2887, -2734, -2206, -1284,
		0, 1567, 3282, 4965, 6406, 7383, 7687, 7153,
		5681, 3263, 0, -3895, -8102, -12203, -15719,
		-18144, -18988, -17828, -14354, -8404, 0,
		10647, 23131, 36871, 51151, 65168, 78086,
		89110, 97539, 102827, 104630, 102827, 97539,
		89110, 78086, 65168, 51151, 36871, 23131, 10647,
		0, -8404, -14354, -17828, -18988, -18144, -15719,
		-12203, -8102, -3895, 0, 3263, 5681, 7153, 7687,
		7383, 6406, 4965, 3282, 1567, 0, -1284, -2206,
		-2734, -2887, -2720, -2311, -1752, -1131, -527,
		0, 413, 694, 844, 879, 822, 700, 538, 358, 175,
		0, -160
		}; // end table

		// PART 2: MODULATION data: 20 bits used of 32 bits
		int32_t coeffs_table_modulate [] = {
		0, 0, 0, 0, 0, 0, 0, 1, 6, 29,
		128, 494, 1657, 4822, 12166,
		26624, 50529, 83171, 118733, 147004,
		157851, 147004, 118733, 83171,
		50529, 26624, 12166, 4822, 1657, 494,
		128, 29, 6, 1, 0, 0, 0, 0, 0, 0, 0
		}; // end table
const int firstnonzero_modulate=7; // element 7 = 8th element in table
const int lastnonzero_modulate=33; // element 33 = 34th element in table


#else
	#if _INTMATH == 3210
		// PART1 : DEMODULATION DATA: 22 bits used of 32 bits
		int32_t coeffs_table_demodulate [] = {

		-645, 0, 701, 1433, 2153, 2800, 3288, 3516,
		3377, 2776, 1652, 0, -2111, -4528, -7011, -9248,
		-10882, -11551, -10939, -8825, -5140, 0, 6269,
		13128, 19861, 25624, 29530, 30748, 28610, 22723, 13054,
		0, -15584, -32410, -48815, -62880, -72578, -75953,
		-71316, -57418, -33620, 0, 42589, 92523, 147483,
		204605, 260670, 312346, 356440, 390155, 411309, 418519,
		411309, 390155, 356440, 312346, 260670, 204605, 147483,
		92523, 42589, 0, -33620, -57418, -71316, -75953,
		-72578, -62880, -48815, -32410, -15584, 0, 13054,
		22723, 28610, 30748, 29530, 25624, 19861, 13128, 6269,
		0, -5140, -8825, -10939, -11551, -10882, -9248, -7011,
		-4528, -2111, 0, 1652, 2776, 3377, 3516, 3288, 2800,
		2153, 1433, 701, 0, -645
		}; // end table

		// PART 2: MODULATION data: 22 bits used of 32 bits
		int32_t coeffs_table_modulate [] = {
		0, 0, 0, 0, 0, 0, 1, 4, 22, 115, 511, 1976, 6629, 19287,
		48665, 106494, 202115, 332686, 474931, 588014, 631404,
		588014, 474931, 332686, 202115, 106494, 48665, 19287,
		6629, 1976, 511, 115, 22, 4, 1, 0, 0, 0, 0, 0, 0
		}; // end table

const int firstnonzero_modulate=6; // element 6 = 7th element in table
const int lastnonzero_modulate=34; // element 34 = 35 element in table


#endif
#endif
#endif
#endif


const int coeffs_size_demodulate=103;
const int buffersize_demodulate=2060; // coeffs_size_demodulate * 20;

const int coeffs_size_modulate=41;
const int buffersize_modulate=820; // coeffs_size_modulate * 20;

// some defines for the PLL (part of demodulation)

#define PLLMAX 3200 
#define PLLHALF 1600
// PLL increase = PLLMAX / 10
#define PLLINC 320

// INC = 32 (not used in application itself)
//#define INC 32

// SMALL PLL Increase = PLLINC / INC
#define SMALLPLLINC 10

///////////////////////////////////////
/// function firfilter for demodulation
// floating point math: input = float, output = float
// 64 bit integer math: input = 16 bit integer, output = 64 bit integer
// 32 bit integer math: input = 16 bit integer, output = 32 bit integer
#if _USEFLOAT == 1
float firfilter_demodulate(struct c2gmsk_session * sessid, float val) {
#else
	#if _INTMATH == 64
	int64_t firfilter_demodulate(struct c2gmsk_session * sessid, int16_t val) {
	#else
	// int32_20 and int32_22
	int32_t firfilter_demodulate(struct c2gmsk_session * sessid, int16_t val) {
	#endif
#endif

// static vars moved to session
// static int pointer;
// static int init=1;


//int64_t retval;
#if _USEFLOAT == 1
// statis vars moved to session
// static float *buffer;
float retval;
float *ptr1;
float *ptr2;
#else
// static int16_t *buffer;
int16_t *ptr1;
int32_t *ptr2;

	#if _INTMATH == 64 
	int64_t retval;
	#else
	// int32_20 and int32_22
	int32_t retval;
	#endif
#endif

int bufferloop;
int ret;


// check if sessid really points to session
ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if



if (sessid->dd_filt_init) {
	sessid->dd_filt_init=0;

	// allocate memory for buffer
#if _USEFLOAT == 1
	sessid->dd_filt_buffer=malloc(buffersize_demodulate * sizeof(float));
#else
	sessid->dd_filt_buffer=malloc(buffersize_demodulate * sizeof(int16_t));
#endif

	if (!sessid->dd_filt_buffer) {
		// oeps. Could not create buffer
		return(C2GMSK_RET_NOMEMORY);
	}; // end if


	// init buffer: all zero for first "coeffs_size_demodulate" elements.
#if _USEFLOAT == 1
	memset(sessid->dd_filt_buffer,0,coeffs_size_demodulate * sizeof(float));
#else
	memset(sessid->dd_filt_buffer,0,coeffs_size_demodulate * sizeof(int16_t));
#endif

	// init vars
	sessid->dd_filt_pointer=coeffs_size_demodulate;

	// END INIT
}; // end if

// main part of function starts here
sessid->dd_filt_buffer[sessid->dd_filt_pointer]=val;
sessid->dd_filt_pointer++;


// go throu all the elements of the coeffs table
ptr1=&sessid->dd_filt_buffer[sessid->dd_filt_pointer-coeffs_size_demodulate];
ptr2=coeffs_table_demodulate;

retval=0;
for (bufferloop=0;bufferloop<coeffs_size_demodulate;bufferloop++) {
#if _USEFLOAT == 1
	retval += (*ptr1++) * (*ptr2++);
#else
# if _INTMATH == 64
	retval += (((int64_t) *ptr1++) * ((int64_t) *ptr2++));
#else
	// int32_10 and int32_12
	retval += (*ptr1++ * *ptr2++);
#endif
#endif
}; // end for


// If pointer has moved past of end of buffer, copy last "coeffs_size_demodulate"
// elements to the beginning of the table and move pointer down
if (sessid->dd_filt_pointer >= buffersize_demodulate) {
#if _USEFLOAT == 1
	memmove(sessid->dd_filt_buffer,&sessid->dd_filt_buffer[buffersize_demodulate-coeffs_size_demodulate],coeffs_size_demodulate*sizeof(float));
#else
	memmove(sessid->dd_filt_buffer,&sessid->dd_filt_buffer[buffersize_demodulate-coeffs_size_demodulate],coeffs_size_demodulate*sizeof(int16_t));
#endif
	sessid->dd_filt_pointer=coeffs_size_demodulate;
}; // end if

return(retval);

};


///////////////////////////////////////
/// function firfilter for modulation
#if _USEFLOAT == 1
float firfilter_modulate(struct c2gmsk_session * sessid, float val) {
#else
	#if _INTMATH == 64
	int64_t firfilter_modulate(struct c2gmsk_session * sessid, int16_t val) {
	#else
	// int32_10 and int32_12
	int32_t firfilter_modulate(struct c2gmsk_session * sessid, int16_t val) {
	#endif

#endif

// static vars moved to session
//static int pointer;
//static int init=1;


#if _USEFLOAT == 1
// static vars moved to session
// static float *buffer;
float retval;
float *ptr1;
float *ptr2;
static float *p_firstnonzero_modulate;
#else
// static vars moved to session
// static int16_t *buffer;
int16_t *ptr1;
int32_t *ptr2;
static int32_t *p_firstnonzero_modulate;

	#if _INTMATH == 64
		int64_t retval;
	#else
		// int32_10 and int32_12
		int32_t retval;
	#endif

#endif

int bufferloop;
int ret;

// check if sessid really points to session
ret=checksign_sess(sessid);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if


if (sessid->md_filt_init) {
	sessid->md_filt_init=0;

	// allocate memory for buffer
#if _USEFLOAT == 1
	sessid->md_filt_buffer=malloc(buffersize_modulate * sizeof(float));
#else
	sessid->md_filt_buffer=malloc(buffersize_modulate * sizeof(int16_t));
#endif

	if (!sessid->md_filt_buffer) {
		// oeps. Could not create buffer
		return(C2GMSK_RET_NOMEMORY);
	}; // end if


	// init buffer: all zero for first "coeffs_size_demodulate" elements.
#if _USEFLOAT == 1
	memset(sessid->md_filt_buffer,0,coeffs_size_modulate * sizeof(float));
#else
	memset(sessid->md_filt_buffer,0,coeffs_size_modulate * sizeof(int16_t));
#endif

	// init vars

	sessid->md_filt_pointer=coeffs_size_modulate;
	p_firstnonzero_modulate=&coeffs_table_modulate[firstnonzero_modulate];

	// END INIT
}; // end if

// main part of function starts here
sessid->md_filt_buffer[sessid->md_filt_pointer]=val;
sessid->md_filt_pointer++;


// go throu all the elements of the coeffs table
ptr1=&sessid->md_filt_buffer[sessid->md_filt_pointer-coeffs_size_modulate];
ptr2=p_firstnonzero_modulate;

retval=0;
for (bufferloop=firstnonzero_modulate;bufferloop<=lastnonzero_modulate;bufferloop++) {
#if _USEFLOAT == 1
	retval += (*ptr1++) * (*ptr2++);
#else
#if _INTMATH == 64
	retval += ((int64_t) *ptr1++) * ((int64_t) *ptr2++);
#else
	// int32_10 adn int32_1
	retval += (*ptr1++ * *ptr2++);
#endif
#endif
}; // end for


// If pointer has moved past of end of buffer, copy last "coeffs_size_demodulate"
// elements to the beginning of the table and move pointer down
if (sessid->md_filt_pointer >= buffersize_modulate) {
#if _USEFLOAT == 1
	memmove(sessid->md_filt_buffer,&sessid->md_filt_buffer[buffersize_modulate-coeffs_size_modulate],coeffs_size_modulate*sizeof(float));
#else
	memmove(sessid->md_filt_buffer,&sessid->md_filt_buffer[buffersize_modulate-coeffs_size_modulate],coeffs_size_modulate*sizeof(int16_t));
#endif
	sessid->md_filt_pointer=coeffs_size_modulate;
}; // end if

return(retval);

}; // end function firfilter_modulate


///////////////////////////////////////
/// function demodulate
int demodulate (struct c2gmsk_session * sessid, int16_t audioin) {

// static vars moved to "session"
// static int init=1;
// static int last;
//static int m_pll;

#if _USEFLOAT == 1
float filterret;
#else
signed long long filterret;
#endif

int bit;


if (sessid->dd_dem_init) {
	sessid->dd_dem_init=0;
 
	// init vars
	sessid->dd_dem_last=0;
	sessid->dd_dem_m_pll=0;

// end INIT part of function
}; // end if


// main part of function starts here

#if _USEFLOAT == 1
filterret=firfilter_demodulate(sessid, (float)audioin);
#else
	#if _INTMATH == 3210
	// reduce audio to 10 bits (down from 16)
	filterret=firfilter_demodulate(sessid, audioin >> 6);
	#else
	#if _INTMATH == 3212
	// reduce audio to 12 bits (down from 16)
	filterret=firfilter_demodulate(sessid, audioin >> 4);
	#else
	// 64 bit integer math: process audio as 16 bits
	filterret=firfilter_demodulate(sessid, audioin);
	#endif
	#endif

#endif

// audio invert: (0:no), 1:receive, (2:transmit), 3=both
if (sessid->d_audioinvert & 0x01) {
	if (filterret > 0) {
		bit=0;
	} else {
		bit=1;
	}; // end else - if
} else {
	if (filterret > 0) {
		bit=1;
	} else {
		bit=0;
	}; // end else - if
}; // end else - if


// PLL loop: used to detect points where values passes over the "zero"

if (bit != sessid->dd_dem_last) {
	if (sessid->dd_dem_m_pll < PLLHALF) {
		sessid->dd_dem_m_pll += SMALLPLLINC;
	} else {
		sessid->dd_dem_m_pll -= SMALLPLLINC;
	}; // end else - if
}; // end if

sessid->dd_dem_last=bit;

sessid->dd_dem_m_pll += PLLINC;


if (sessid->dd_dem_m_pll >= PLLMAX) {
	sessid->dd_dem_m_pll -= PLLMAX;

	if (bit) {
		return(0);
	} else {
		return(1);
	}; // end if
			
} else {
	return(-1);
}; // end else - if

}; // end function demodulate


