#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// functions defined below

//int32_t coeffs_table_int [] = {
//	        0,         0,         0,
//	        0,         3,        35,
//	      281,      1924,     11407,
//	    58660,    261606,   1011843,
//	  3394220,   9874802,  24916002,
//	 54524264, 103481536, 170332464,
//	243160784, 301058720, 323273760,
//	301058720, 243160784, 170332464,
//	103481536,  54524264,  24916002,
//	  9874802,   3394220,   1011843,
//	   261606,     58660,     11407,
//	     1924,       281,        35,
//	        3,         0,         0,
//	        0,         0
//};

int32_t coeffs_table_int [] = {
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


const int coeffs_tablesize=41;

/// functions defined at the end of the program
int16_t process_return(int64_t filterret);
int64_t firfilter(int val, int32_t *taps, int tapsize, int *buffer, int buffersize);

int main (int argc, char ** argv) {

// buffer is 20 times the size of the coeffs table
int buffersize=coeffs_tablesize * 20;


// array that will hold the data, will be 20 times the length
int * buffer;


FILE * filein, *fileout, *fileout2;
unsigned char c;
int loop, loop2;

unsigned char bp[8];
int ret;

int64_t filterret;
int16_t ret16;

int64_t outbuff64[80];
int64_t * ptr_outbuff64;

int16_t outbuff16[80];
int16_t * ptr_outbuff16;



// program starts here

if (argc <= 3) {
	fprintf(stderr,"Error: missing filenames: need input and two output filenames.\n");
	exit(-1);
}; // end if


// allocate memory for buffer
buffer=malloc(buffersize * sizeof(int));

if (!buffer) {
	fprintf(stderr,"Error: could allocate memory for databuffer!\n");
	exit(-1);
}; // end if


memset(buffer,0,buffersize * sizeof(int));


// create bit patterns
c=0x01;
bp[0]=c;

for (loop=1;loop<=7;loop++) {
	c<<=1;
	bp[loop]=c;
}; // end for


// read data from file
filein=fopen(argv[1], "r");

if (!filein) {
	fprintf(stderr,"Error: input file %s could not be opened!\n",argv[1]);
	exit(-1);
}; // end if


fileout=fopen(argv[2], "w");

if (!fileout) {
	fprintf(stderr,"Error: output file %s could not be opened!\n",argv[2]);
	exit(-1);
}; // end if


fileout2=fopen(argv[3], "w");

if (!fileout2) {
	fprintf(stderr,"Error: output file %s could not be opened!\n",argv[2]);
	exit(-1);
}; // end if




ret=fread(&c,1,1,filein);

//c=0x0;
while (ret) {
	ptr_outbuff64=outbuff64;
	ptr_outbuff16=outbuff16;

	for (loop=7;loop>=0;loop--) {
		if (c & bp[loop]) {
			for (loop2=0;loop2<10;loop2++) {
				filterret=firfilter(-16384,coeffs_table_int,coeffs_tablesize,buffer,buffersize);
				ret16=process_return(filterret);
//printf("1%16llX %4X\n",filterret, ret16);
				*ptr_outbuff16=ret16;
				*ptr_outbuff64=filterret;
				ptr_outbuff64++;
				ptr_outbuff16++;

			}; // end for
		} else {
			for (loop2=0;loop2<10;loop2++) {
				filterret=firfilter(16384,coeffs_table_int,coeffs_tablesize,buffer,buffersize);
				ret16=process_return(filterret);
//printf("0%16llX %4X\n",filterret, ret16);
				*ptr_outbuff16=ret16;
				*ptr_outbuff64=filterret;
				ptr_outbuff64++;
				ptr_outbuff16++;

			}; // end for
		}; // end else - if
	}; // end for
	fwrite(&outbuff64,80 * sizeof(int64_t),1,fileout);
	fwrite(&outbuff16,80 * sizeof(int16_t),1,fileout2);

	ret=fread(&c,1,1,filein);
//c=0x0;
}; // end while

/// end main program
return(0);
}; // end main program


int16_t process_return(int64_t filterret) {
int bit30;
int16_t filterret16;

// remember 30th bit, is value just below unity
bit30=filterret & 0x40000000;

if (filterret >= 0) {
	// positive
	filterret16=((filterret & 0x3fff80000000LL)>>31);

	// add one for rounding if bit 30 was set
	if (bit30) {
		filterret16++;
	};
} else {
	// negative
	filterret16=-((-filterret & 0x3fff80000000LL)>>31);

	// subtract one for rounding if bit 30 was not set
	if (!(bit30)) {
		filterret16--;
	}; // end if
};

//printf("filterret = %llX, filterret16 = %X\n",filterret,filterret16);

return(filterret16);

}; // end void


	




/// function firfilter
int64_t firfilter(int val, int32_t *taps, int tapsize, int *buffer, int buffersize) {
static int pointer;
static int init=1;

//int64_t retval;
signed long long retval;

int bufferloop;

int *ptr1,*ptr2;

if (init) {
	init=0;

	pointer=tapsize;
}; // end if

buffer[pointer]=val;
pointer++;


ptr1=&buffer[pointer-tapsize];
ptr2=taps;

retval=0;
for (bufferloop=0;bufferloop<tapsize;bufferloop++) {
//printf("%X %X ",*ptr1,*ptr2);
	retval += ((long long) *ptr1++) * ((long long) *ptr2++);
//printf("%llX \n",retval);
}; // end for


// move memory if pointer to end of buffer
if (pointer >= buffersize) {
	memcpy(buffer,&buffer[buffersize-tapsize],tapsize*sizeof(int));
	pointer=tapsize;
}; // end if

return(retval);

};

