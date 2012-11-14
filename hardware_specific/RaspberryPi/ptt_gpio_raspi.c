// PTT_GPIO_RASPI

// PTT driver for GMSK modem.
// Sets GPIO port based on the presents of a posix filelock on file
// Raspberry pi version using bcm2835 library

// This application forks a seperate process that runs as "root"
// to do GPIO switching

// Known problems:
// If the main application unexpectably dies, the child process (that is
// reading input from the parent process) does not die

#include <errno.h>
#include <string.h>
#include <stdio.h>

#define _GNU_SOURCE
#include <unistd.h>

// needed for exit
#include <stdlib.h>

// needed for GPIO
#include <bcm2835.h>

// posix threads + inter-process control
#include <pthread.h>

#include <sys/stat.h>
#include <fcntl.h>

// needed for getppid
#include <sys/types.h>
#include <unistd.h>

#define FOREVER 1


//////////////////// CONFIGURATION: CHANGE BELOW 
// use pin 11 (output) to drive PTT
#define PIN RPI_GPIO_P1_11

//////////////////// END
//////////////////// DO NOT CHANGE ANYTHING BELOW UNLESS YOU KNOW WHAT
//////////////////// YOU ARE DOING

// global data structure
typedef struct {
	int out;
	int readpipe;
	int writepipe;
} globaldatastr;

// global data itself
globaldatastr global;


// defined below
void * funct_writetochild (void * globalin);
void * do_child (globaldatastr * p_global);

int main (int argc, char ** argv) {
int pipefd[2];
int ret;
pid_t pid;
int lockfd;

char * defaultlockfile="/var/tmp/ptt.lock";
char * lockfile;


uid_t myuid, myeuid; // uid and effective uid
gid_t mygid, myegid; // gid and effective gid

// vars for endless loop in main application
int lret, laststate, newstate;


pthread_t thr_writetochild;


// check UID levels.
// GPIO needs root priviledges; however to assure overall security it
// is a "bad idea" (c) to run the whole application as root (or as "sudo");
// Therefor, the application forces the implementation of the "setuid"
// method: the application needs to be owned by root and installed with
// the "setuid" bits set.
// This can be achieved as follows:
// chown root:root ./ptt_gpio ; chmod 6755 ./ptt_gpio
//
// The application will then, with exception of the actual code doing GPIO,
// drop its own prividges to non-root 
// As "setuid" applies to all threads, the GPIO-code that needs to run
// as root runs as a seperate process; not just a thread

myuid=getuid();
myeuid=geteuid();
mygid=getgid();
myegid=getegid();

if (myuid == 0) {
	fprintf(stderr,"Error: UID = Root; Please do not start this application as root. Use setuid method: sudo chown root:root %s ; sudo chmod 6755 %s\n",argv[0],argv[0]);
	exit(-1);
}; // end if

if (mygid == 0) {
	fprintf(stderr,"Error: GID = Root; Please do not start this application as root: Use setuid method: sudo chown root:root %s ; sudo chmod 6755 %s\n",argv[0],argv[0]);
	exit(-1);
}; // end if


if (myeuid != 0) {
	fprintf(stderr,"Error: EUID not ROOT: GPIO needs root priviledges. Please use setuid method: chown root:root %s ; chmod 6755 %s\n",argv[0],argv[0]);
	exit(-1);
}; // end if

if (myegid != 0) {
	fprintf(stderr,"Error: EGID not ROOT: GPIO needs root priviledges. Please use setuid method: chown root:root %s ; chmod 6755 %s\n",argv[0],argv[0]);
	exit(-1);
}; // end if


/// main part of main application

// init vars
global.out=0;


if (argc > 1) {
	lockfile=argv[1];
} else {
	lockfile=defaultlockfile;
}; // end else - if


// create pipe for communication with child process
ret=pipe(pipefd);
// pipefd[0] = READ, pipefd[1] = WRITE

if (ret < 0) {
	fprintf(stderr,"Error in pipe: %d(%s)\n",errno,strerror(errno));
}; // end if

// copy write pipe to global data, so it can be used by subthread
global.readpipe=pipefd[0];
global.writepipe=pipefd[1];


// create and start child process
pid=fork();

if (!pid) {
	// child-process
	// should run as root
	// does actual GPIO

	if (!bcm2835_init()) {
		fprintf(stderr,"Error: could not init BCM2835! \n");
		exit(-1);
	}; // end if

	// set the pin to be output
	bcm2835_gpio_fsel(PIN, BCM2835_GPIO_FSEL_OUTP);

	while (FOREVER) {
		do_child(&global);
	}; // end while

	fprintf(stderr,"Error: child process dropped out of endless loop: should not happen!\n");
	exit(-1);
	
}; // end if (child process)



// main application (parent process)

// drop priviledge level
ret=setuid(myuid);
ret=setgid(mygid);

if (ret < 0) {
	fprintf(stderr,"Error: setuid failed: %s\n",strerror(errno));
	exit(-1);
}; // end if


	
// close unused read pipe (not used in parent process)
close (global.readpipe);

// create subthread.
// this thread will receive input from the child process and
// pass that on to the main thread
pthread_create (&thr_writetochild, NULL, funct_writetochild, (void *) &global);


// main application: check lock-file and write status to global data
// create lock-file
lockfd=open(lockfile, O_WRONLY| O_CREAT , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
if (lockfd < 0) {
	fprintf(stderr,"Error: lockfile %s could not be opened or created. Error %d(%s)\n",lockfile,errno,strerror(errno));
	exit(-1);
}; // end if

// init vars for main loop
laststate=-1;

while (FOREVER) {

	lret = lockf(lockfd, F_TEST, 0);

	if (lret < 0) {
		// file is locked,
		global.out = 1;
		newstate = 1;
	} else {
	// file is not locked
		global.out = 0;
		newstate = 0;
	}; // end else - if

	if (laststate != newstate) {
		// wait for 1 second
		printf("last state was %d, now %d\n",laststate,newstate);
		laststate=newstate;
		sleep(1);
	}; // end if		

	// wait 50 ms
	usleep(50000);

}; // end endless loop while

fprintf(stderr,"Error: main application drops out of endless loop. Should not happen! \n");
exit(-1);


}; // end main application


///////////////////////////////
// function funct_writetochild
// subthread

void * funct_writetochild (void * globalin) {
globaldatastr * p_global;

// local vars
int ret;
char c, lastc;
int delay;
int failed;


// init global data
p_global = (globaldatastr *) globalin;
delay=0;
lastc=0; // set value to 0, forcing a send first time loop is run
failed=0;

// endless loop
while (FOREVER) {
	
	// check status, send 0x30 (ascii "0") or 0x31 (ascii "1")
	if (p_global->out) {
		c=0x31;
	} else {
		c=0x30;
	}; // end else - if

	if (c != lastc) {
		ret=write(p_global->writepipe,&c,1);

		if (ret != 1) {
			fprintf(stderr,"Error: write to FIFO failed: %s \n",strerror(errno));

			failed++;

			// stop if could not send anything during 10 seconds
			// (perhaps child has died?)

			if (failed > 10) {
				fprintf(stderr,"Error: write to FIFO failed after more then 10 tried. Stopping!!!\n");
				exit(-1);
			}; // end if

			sleep(1);
		} else {
			failed=0;
		}; // end if

		lastc=c;
		delay=0;
	} else {
		// data has not changed
		// do not send data to child.

		// however, do send data every 1 second (20 * 50 ms) as "keepalive"
		delay++;

		if (delay >= 20) {
			ret=write(p_global->writepipe,&c,1);

			if (ret != 1) {
				fprintf(stderr,"Error: write to FIFO failed: %s \n",strerror(errno));
				sleep(1);
			}; // end if

			lastc=c;
			delay=0;
		}; // end if
	}; //  end else - if

	// sleep for 50 ms
	usleep(50000);
}; // end loop forever

// out of endless loop, we should never get here
fprintf(stderr,"Error: write-to-child thread dropped out of endless loop. Should not happen!\n");
exit(-1);
}; // end function writetochild


///////////////////////////////
// function do_child
// subthread
void * do_child (globaldatastr * p_global) {
int ret;
char c;


// endless loop
while (FOREVER) {


	// first check: check parent process id

	if (getppid() == 1) {
		// if this is 1, this means that the parent process has died; so the program should terminate
		fprintf(stderr,"Error: child process detects no more parent process: quiting! \n");
		exit(-1);
	}; // end if


	// read data from PIPE
	ret=read(p_global->readpipe,&c,1);

	if (ret) {
		if (c == 0x30) {
			// Turn it off
			bcm2835_gpio_write(PIN, LOW);

		} else if (c == 0x31) {
			// Turn it on
			bcm2835_gpio_write(PIN, HIGH);

		} else {
			// error: unknown char
			fprintf(stderr,"Warning: Child received unknown char from parent: %02X\n",c);
		}; // end else - elsif - if
	
		// sleep 50 ms
		usleep(50000);
	} else {
		// error in read
		fprintf(stderr,"Error: Child did not receive any data from parent\n");

		// sleep 1 second
		sleep(1);
	};  // read successfull


}; // end while (endless loop)


}; // end function do_child
