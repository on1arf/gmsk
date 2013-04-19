/* gpio init: create file-structure in /sys/class/gpio */
// Should be installed as "setuid" as this application need
// root-access to create the GPIO files

// version 20130418: initial release

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



// defines
#define MAXGPIO 99999
#define FNAMESIZE 40

// for (f)printf
#include <stdio.h>

// for strcmp
#include <string.h>

// for exit
#include <unistd.h>
#include <stdlib.h>

// for stat, open
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// error number
#include <errno.h>


// functions declared further down
int stat_dir (int, struct stat , int ,char *);
int write_check (char * , char * );

//////////////////////
// main application
/////////////////

int main (int argc, char ** argv) {

int ret, stat_ok, loop, len;

char *gpio_c=NULL;
char gpio_ret[6];
int gpio=-1;
int direction=0; // 0 = in, 1 = out

char fname[FNAMESIZE];
char txt[6];


struct stat s;

for (loop=1; loop < argc; loop++) {
	if (strcmp(argv[loop],"-gpio_in") == 0) {
		if (gpio_c) {
			// gpio pin number already entered
			fprintf(stderr,"Error: only one gpio pin can be provided at once\n");
			exit(-1);
		}; // end if
		if (loop+1 < argc) {
			loop++;
			gpio_c=argv[loop];
			direction=0;
			continue;
		}; // end if
	} else if (strcmp(argv[loop],"-gpio_out") == 0) {
		if (gpio_c) {
			// gpio pin number already entered
			fprintf(stderr,"Error: only one gpio pin can be provided at once\n");
			exit(-1);
		}; // end if
		if (loop+1 < argc) {
			loop++;
			gpio_c=argv[loop];
			direction=1;
			continue;
		}; // end if
	}; // end if
}; // end if

if (gpio_c == NULL) {
	fprintf(stderr,"Usage: %s { -gpio_in <pin> | -gpio_out <pin> }\n",argv[0]);
	exit(-1);
}; // end if

gpio=atoi(gpio_c);


// as this application runs with root priviledges.
// do some more checks

// convert value back to string
snprintf(gpio_ret,6,"%d",gpio);

if ((strcmp(gpio_c,gpio_ret) != 0)) {
	fprintf(stderr,"Error: strange result for gpio value convertion from %s to %s!\n",gpio_c,gpio_ret);
	exit(-1);
}; // end if

// check value of gpio pin, should be between 0 and MAXGPIO
if ((gpio < 0) || (gpio >= MAXGPIO)) {
	fprintf(stderr,"Error: gpio port number should be between 0 and %d\n",MAXGPIO);
	exit(-1);
}; // exit if

snprintf(fname,FNAMESIZE,"/sys/class/gpio/gpio%d",gpio);
ret=stat(fname,&s);

// is it a directory?
stat_ok=stat_dir(ret,s,errno,fname);

if (stat_ok == 1) {
	fprintf(stderr,"Error: %s exists but is not a directory. Existing!\n",fname);
	exit(-1);
} else if (stat_ok == 2) {
	int fd;
	// gpio port does not exist. Export it
	snprintf(fname,FNAMESIZE,"/sys/class/gpio/export");

	fd=open(fname,O_WRONLY); // open WITHOUT create

	if (!fd) {
		fprintf(stderr,"Error: could not open %s: %d (%s)\n",fname,errno,strerror(errno));
		exit(-1);
	}; // end if

	len=snprintf(txt,6,"%d\n",gpio);
	if (len) {
		ret=write(fd,txt,len+1);
	} else {
		fprintf(stderr,"Internal error on snprintf\n");
		exit(-1);
	}; // end if

	if (ret != len+1) {
		fprintf(stderr,"Error: write to export gpio failed: %d (%s)\n",errno,strerror(errno));
		exit(-1);
	}; // end if

	// close file
	close(fd);

	// OK, is the directory now created ?
	snprintf(fname,FNAMESIZE,"/sys/class/gpio/gpio%d",gpio);
	ret=stat(fname,&s);

	// is it a directory?
	if (stat_dir(ret,s,errno, fname) != 0) {
		fprintf(stderr,"Error: exporting gpio failed!\n");
		exit(-1);
	}; // end if

}; // end if

// OK, we have a valid gpio pin.
// write direction
snprintf(fname,FNAMESIZE,"/sys/class/gpio/gpio%d/direction",gpio);
if (direction) {
	ret=write_check ("out", fname);
} else {
	ret=write_check ("in", fname);
}; // end else - if

if (ret < 0) {
	exit(-1);
}; // end if

// write active_low
snprintf(fname,FNAMESIZE,"/sys/class/gpio/gpio%d/active_low",gpio);
ret=write_check ("0", fname);

if (ret < 0) {
	exit(-1);
}; // end if

// change mod for "value" field
snprintf(fname,FNAMESIZE,"/sys/class/gpio/gpio%d/value",gpio);
if (chmod(fname,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH) < 0) {
	fprintf(stderr,"Error: chmod failed: %d (%s)\n",errno,strerror(errno));
}; // end if

exit(0);

}; 


/////////////////////////////////////////
/////////////////////////////
///////// END MAIN APPLICATION
/////////////////////////////
/////////////////////////////////////////


int stat_dir (int in_statret, struct stat in_stat, int in_errno,char * in_fname) {
// process result of "stat", based on result value of "stat" and "errno"
// returns: 0: exists, is directory
// returns: 1: exists, not a directory
// returns: 2: does not exit
if (in_statret == -1) {
	if (in_errno == ENOENT) {
		return(2); // does not exist
	} else {
		fprintf(stderr,"Error on stat_dir processing %s: %d (%s)\n",in_fname,in_errno,strerror(in_errno));
		exit(-1);
	}; // end else - if
} else {
	if (S_ISDIR(in_stat.st_mode)) {
		return(0);
	} else {
		return(1);
	}; // end else - if
}; // end if

// we should never get here
fprintf(stderr,"Internal Error on stat_dir processing %s: reached end. Should not happen\n",in_fname);

return(-1);

}; // end function stat_dir



int write_check (char * txt, char * filename) {
int fd;
int towrite, numchar;
char text[31]; // 30 char + NULL

// open file
fd=open(filename,O_WRONLY); // open for read!

if (!fd) {
	fprintf(stderr,"Error: opening file %s for write failed: %d (%s)\n",filename,errno,strerror(errno));
	exit(-1);
}; // end if

// to write data, copy so we can add a terminating \n
towrite=strlen(txt);

if (towrite > 30) {
	towrite=30;
}; // end if

memcpy(text,txt,towrite);

// add terminating \n
text[towrite]=0x0a; 
towrite++;

numchar=write(fd,text,towrite);

close(fd);


// write is done, now read again is write was successfull
// open file
fd=open(filename,O_RDONLY); // open for read!

if (!fd) {
	fprintf(stderr,"Error: opening file %s for read failed: %d (%s)\n",filename,errno,strerror(errno));
	exit(-1);
}; // end if

numchar=read(fd, text,30); // read up to 30 char

if (numchar == 0) {
	fprintf(stderr,"Error: reading file %s returned no text!\n",filename);
	return(-1);
} else if (numchar <= 0) {
	fprintf(stderr,"Error: reading file %s failed: %d (%s)\n",filename,errno,strerror(errno));
	exit(-1);
}; // end if

// close file
close(fd);


// replace terminating \n with null
text[numchar-1]=0;

// if the text to write different from what wrote?
if (strcmp(text,txt) != 0) {
	fprintf(stderr,"Error: write/read to file %s returned failed: write %s, read %s!\n",filename,txt,text);
	return(-1);
}; // end if

return(0);

}; // end function write_check
