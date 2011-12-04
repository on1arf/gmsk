/* initinfile.h */

int init_infile (char * retmsg) {
// init
global.filein=fopen(global.fnamein,"r");

if (!global.filein) {
	snprintf(retmsg,160,"Error: input file %s could not be opened!\n",global.fnamein);
	return(-1);
}; 

// all OK
return(0);
}; // end if
