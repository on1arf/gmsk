// input_sal

// Serder Input Source Abstraction Layer

// This code is designed to be able to read from file, stdin, TCP-stream 
// or UDP-stream using the same "input" code.

// It abstracts the input I/O from the "input_dvt", "input_raw" and
// "input_stream" layers

/*
 *      Copyright (C) 2012 by Kristoff Bonne, ON1ARF
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

// Release information:
// 20120224: Initial release


// COMMANDS
#define ISAL_CMD_INIT 0
#define ISAL_CMD_OPEN 1
#define ISAL_CMD_READ 2
#define ISAL_CMD_CLOSE 3

// return values
#define ISAL_RET_SUCCESS 0
#define ISAL_RET_FAIL 1
#define ISAL_RET_WARN 2
#define ISAL_RET_READ_EOF 10

// can input be reopened after close?
#define ISAL_INPUT_REOPENYES 1
#define ISAL_INPUT_REOPENNO 0

// error messages
#define ISAL_ERR_ALREADYINIT 0
#define ISAL_ERR_NOTINIT 1
#define ISAL_ERR_NOTOPEN 2
#define ISAL_ERR_CMDFAIL 3
#define ISAL_ERR_NOTYETIMPLEMENTED 10


// This version can only process one input stream at a time
int input_sal (int cmd, void * data, int len, int * retval2, char * retmsg) {

// vars for FILE in
static FILE * filein;

// vars for TCP in
static int localfd, clientfd;
static struct sockaddr_in6 localsa, clientsa;
static socklen_t clilen=sizeof(struct sockaddr_in6);
static int socketbindlisten_tcp=0;
static int tcp_connected=1;

// vars for UDP in
static int udpsock=-1;

static c_globaldatastr * p_c_global; 
static s_globaldatastr * p_s_global;


// other vars
static int init=0;
static int methode=0;
static int has_been_closed=0; // 0: not been closed before, 1: has been closed and can be reopened
										// 2: has been clossed and may not be reopened

static unsigned char *udpbuffer=NULL; // buffer to hold UDP packets
static int udp_begin=0; // pointer to begin of valid data in buffer
static ssize_t udp_size=0; // amount of valid data in buffer


// command INIT
// Input data = pointer to combined global data structure
// returns 0 

if (cmd == ISAL_CMD_INIT) {

	// check if not initialised
	if (init) {
		snprintf(retmsg,ISALRETMSGSIZE,"Error: SAL already initialised\n");
		*retval2=ISAL_ERR_ALREADYINIT;
		return(ISAL_RET_FAIL);
	}; // end

	init=1;
	filein=NULL;
	has_been_closed=0;
	clientfd=-1;

	p_c_global = (c_globaldatastr *) data;
	p_s_global = p_c_global->p_s_global;

	return(ISAL_RET_SUCCESS);
}; // end if


// command OPEN
// No input data (all info taken from global data)
// return: 0 on success, 1 on failure
// retval2:
// (if success): 0 if input is ending (e.g. single file)
//               1 if input is endless (stdin, tcp)¦
// (if failure): error id

if (cmd == ISAL_CMD_OPEN) {
	if (!(init)) {
	// not initialised
		snprintf(retmsg,ISALRETMSGSIZE,"Error: SAL not initialised\n");
		*retval2=ISAL_ERR_NOTINIT;
		return(ISAL_RET_FAIL);
	}; // end

	// check "has been closed" flag
	// if 2 ("has been closed and may not be reopenen"), return
	if (has_been_closed == 2) {
		snprintf(retmsg,ISALRETMSGSIZE,"Error: Input has been closed and may not be reopened\n");
		*retval2=ISAL_ERR_CMDFAIL;
		return(ISAL_RET_FAIL);
	}; // end if

	// input methode 1: stdin
	if (p_s_global->infromstdin) {
		filein=stdin;
		methode=1;

		// set retval2 to "1" (endless input)
		*retval2=1;
		return(0);
	}; // end if

	// input methode 2: file
	if (p_s_global->fnamein) {
		filein=fopen(p_s_global->fnamein,"r");

		if (!(filein)) {
			snprintf(retmsg,ISALRETMSGSIZE,"Error: could not open file %s\n",p_s_global->fnamein);
			*retval2=ISAL_ERR_CMDFAIL;
			return(ISAL_RET_FAIL);
		};
		// success
		*retval2=0; // input is ending, not endless
		methode=2;
		return(ISAL_RET_SUCCESS);
	}; // end if

	// input methode 3: TCP
	if (p_s_global->tcpport) {
		int ret;

		// init vars
		localsa.sin6_family=AF_INET6;
		localsa.sin6_port=htons(p_s_global->tcpport);
		memset(&localsa.sin6_addr,0,sizeof(struct in6_addr)); // address = "::" (all 0) -> we listen
																				// to all ip-addresses of host

		if (!socketbindlisten_tcp) {
			localfd=socket(AF_INET6,SOCK_STREAM,0);

			if (localfd < 0) {
				snprintf(retmsg,ISALRETMSGSIZE,"Error: creation of socket for TCP failed\n");
				*retval2=ISAL_ERR_CMDFAIL;
				return(ISAL_RET_FAIL);
			}; // end if

			// bind sockets
			ret=bind(localfd, (struct sockaddr *) &localsa, sizeof(localsa));
			if (ret) {
				snprintf(retmsg,ISALRETMSGSIZE,"Error: bind on socket for TCP failed\n");
				*retval2=ISAL_ERR_CMDFAIL;
				return(1);
			}; // end if


			// listen for incoming tcp sessions
			ret=listen(localfd,16); // max. 16 connections in backlog

			if (ret) {
				snprintf(retmsg,ISALRETMSGSIZE,"Error: listen on socket for TCP failed\n");
				*retval2=ISAL_ERR_CMDFAIL;
				return(1);
			}; // end if

			socketbindlisten_tcp=1;
		}; // end if

		// success

		// mark "connected" to 0; will be set the "1" the first time we try to read from the connection
		tcp_connected=0;

		// done, return
		methode=3;

		*retval2=1; // ENDLESS
		return(ISAL_RET_SUCCESS);
	}; // end if

	// input methode 4: UDP
	if (p_s_global->udpport) {
		int ret;

		// initialise UDP buffer
		if (!udpbuffer) {
			udpbuffer=malloc(1500); // we can ask up to 1500 octets

			if (!udpbuffer) {
				snprintf(retmsg,ISALRETMSGSIZE,"Error: malloc for UDP buffer\n");
				*retval2=ISAL_ERR_CMDFAIL;
				return(1);
			}; // end if
		}; // end if

		udpsock=socket(AF_INET6,SOCK_DGRAM, 0);


		// init vars
		localsa.sin6_family=AF_INET6;
		localsa.sin6_port=htons(p_s_global->udpport);
		localsa.sin6_flowinfo=0; // flows not used
		localsa.sin6_scope_id=0; // we listen on all interfaces
		memset(&localsa.sin6_addr,0,sizeof(struct in6_addr)); // address = "::" (all 0) -> we listen
																				// to all ip-addresses of host
		ret=bind(udpsock, (struct sockaddr *)&localsa, sizeof(localsa));

		if (ret < 0) {
			snprintf(retmsg,ISALRETMSGSIZE,"Error: bind for UDP failed: %d (%s)\n",errno,strerror(errno));
			*retval2=ISAL_ERR_CMDFAIL;
			return(1);
		}; // end if

		// done, return
		methode=4;

		*retval2=1; // ENDLESS
		return(ISAL_RET_SUCCESS);
	}; // end if

	// catchall
	// no methode found to open
	snprintf(retmsg,ISALRETMSGSIZE,"Error: unknown methode for SAL_OPEN\n");
	*retval2=ISAL_ERR_CMDFAIL;
	return(ISAL_RET_FAIL);
}; // end if



// command READ
// input:
// pointer to data structure
// number of octets expected to be read
// return: success, failure or EOF 
// retval2:
// (if success or EOF): number of octets read
// (if failure): error id

if (cmd == ISAL_CMD_READ) {
	if (!(init)) {
	// not initialised
		snprintf(retmsg,ISALRETMSGSIZE,"Error: SAL not initialised\n");
		*retval2=ISAL_ERR_NOTINIT;
		return(ISAL_RET_FAIL);
	}; // end

	if (!(methode)) {
	// not open
		snprintf(retmsg,ISALRETMSGSIZE,"Error: SAL input not opened\n");
		*retval2=ISAL_ERR_NOTOPEN;
		return(ISAL_RET_FAIL);
	}; // end


	// read from stdin or filein
	if ((methode == 1) || (methode == 2)) {
		size_t numread;
		numread=fread(data,1,len,filein);

		if ((int) numread < len) {
			// check for error or eof
			if (feof(filein)) {
				*retval2=len;
				return(ISAL_RET_READ_EOF);
			}; 

			if (ferror(filein)) {
				snprintf(retmsg,ISALRETMSGSIZE,"Error: read fails in SAL_READ\n");
				*retval2=ISAL_ERR_CMDFAIL;
				return(ISAL_RET_FAIL);
			}

			// no error of eof, process as success
		}; // end if

		// success
		*retval2=len;
		return(ISAL_RET_SUCCESS);
	}; // end if

	// read from tcp stream
	if (methode == 3) {
		int numread;
		// if not yet opened a stream, open one now
		if (!tcp_connected) {
			clientfd=accept(localfd,(struct sockaddr *)&clientsa,&clilen);
			tcp_connected=1;
		}; // end if

		// read data
		numread=recvfrom(clientfd,data,len,0,(struct sockaddr*)&clientsa,&clilen);

		if (numread < 0) {
			// error 
			snprintf(retmsg,ISALRETMSGSIZE,"Error: read failed reading TCP stream\n");
			*retval2=ISAL_ERR_CMDFAIL;
			return(ISAL_RET_FAIL);
		};

		// success
		*retval2=(int) numread;
		return(ISAL_RET_SUCCESS);

	}; // end if

	// read from udp stream
	if (methode == 4) {
		// UDP packets are received "at hoc", creating a buffer then can be used to
		// feed to s_inpup_sal "read" function
		int octetsprovided=0;
		unsigned char *dp;

		// init dp
		dp=(unsigned char*) data;

		// loop until we get the requested number of octets from the 
		// udp buffer
		while (octetsprovided < len) {

			// begin of read:
			// phase 1: do we still have some data in the buffer?
			if (udp_size) {
			// copy either what we have available or what is requested
				int tocopy;
				int needed;
				needed=len-octetsprovided;

				if (needed < udp_size) {
					tocopy=needed;
				} else {
					tocopy=udp_size;
				}; // end else - if

				//copy available data to buffer
				memcpy(&dp[octetsprovided],&udpbuffer[udp_begin],tocopy);

				// correct vars
				octetsprovided += tocopy;
				udp_begin += tocopy;
				udp_size -=  tocopy;				
			}; // end if (still data in buffer)

			// if we have enough data
			if (octetsprovided >= len) {
				// break out of while loop
				break;
			}; // end if

			// at this point, the udp buffer should be empty, so let's
			// let's get some more data
			// re-init (just to be sure)
			udp_begin=0;


			// read until read or error, but ignore "EINTR" errors
			while (FOREVER) {
				udp_size=recvfrom(udpsock, udpbuffer, 1500, 0, (struct sockaddr *)&localsa, &clilen);
				// no error: break out
				if (udp_size >= 0) {
					break;
				}; // end if

				// error: break out when not error EINTR
				if (errno != EINTR) {
					break;
				}; // end if
			}; // end while

			if (udp_size < 0) {
				// error condition

				// reiint udp_size 
				udp_size=0;

				snprintf(retmsg,ISALRETMSGSIZE,"Error: recvfrom failed reading UDP packet: error %d (%s)\n", errno,strerror(errno));
				*retval2=ISAL_ERR_CMDFAIL;
				return(ISAL_RET_FAIL);
				
			} else if (! udp_size) {
				// error condition with orda

				snprintf(retmsg,ISALRETMSGSIZE,"Error: recvfrom failed reading UDP packet: socket closed\n");
				*retval2=ISAL_ERR_CMDFAIL;
				return(ISAL_RET_FAIL);
			};// end if
		}; // end while (not enough provided)


		// success
		*retval2=(int) len;
		return(ISAL_RET_SUCCESS);
	}; // end if (methode 4/UDP)


	// catchall
	// no methode found to read
	snprintf(retmsg,ISALRETMSGSIZE,"Error: unknown methode for SAL_READ\n");
	*retval2=ISAL_ERR_CMDFAIL;
	return(1);

}; // end command READ

// command CLOSE
// no input, all data taken from local vars
// return: success, failure 
// retval2:
// (if success): REOPEN-YES or REOPEN-NO
// (if failure): error id

if (cmd == ISAL_CMD_CLOSE) {
	if (!(init)) {
	// not initialised
		snprintf(retmsg,ISALRETMSGSIZE,"Error: SAL not initialised\n");
		*retval2=ISAL_ERR_NOTINIT;
		return(ISAL_RET_FAIL);
	}; // end

	if (!(methode)) {
	// not open
		snprintf(retmsg,ISALRETMSGSIZE,"Error: SAL input not opened\n");
		*retval2=ISAL_ERR_NOTOPEN;
		return(ISAL_RET_FAIL);
	}; // end

	if (methode == 1) {
		// do not close stdin as this will cause an interruption in case of piping
		filein=NULL;

		// reset methode 
		methode=0;


		// reopen is possible (stdin is endless stream)
		*retval2=ISAL_INPUT_REOPENYES;
		has_been_closed=1;

		return(ISAL_RET_SUCCESS);
	}; // end if

	if (methode == 2) {
		// close file
		fclose(filein);
		filein=NULL;

		// reset methode
		methode=0;

		// reopen is not possible
		*retval2=ISAL_INPUT_REOPENNO;
		has_been_closed=2;

		return(ISAL_RET_SUCCESS);
	}; // end if

	if (methode == 3) {
		// close TCP socket
		close(clientfd);
		clientfd=-1;

		// reset methode
		methode=0;

		// reopen is possible
		*retval2=ISAL_INPUT_REOPENYES;
		has_been_closed=1;

		return(ISAL_RET_SUCCESS);
	}; // end if

	if (methode == 4) {
		// close UDP socket
		close(udpsock);
		udpsock=-1;

		// flush UDP buffer
		udp_size=0;

		// reset methode
		methode=0;

		// reopen is possible
		*retval2=ISAL_INPUT_REOPENYES;
		has_been_closed=1;

		return(ISAL_RET_SUCCESS);
	}; // end if

	// catch all
	// no methode found to close
	snprintf(retmsg,ISALRETMSGSIZE,"Error: unknown methode for SAL_CLOSE\n");
	*retval2=ISAL_ERR_CMDFAIL;
	return(ISAL_RET_FAIL);

}; // end command CLOSE

// catchall

snprintf(retmsg,ISALRETMSGSIZE,"Error: unknown command %d\n",cmd);
*retval2=ISAL_ERR_CMDFAIL;
return(ISAL_RET_FAIL);
}; // end function input_sal


// support functions:
// input_sal_readall: reads input until all data read
// returns ISAL_RET_SUCCESS, ISAL_RET_FAIL or ISAL_RET_READ_EOF
// if less then "len" octets are read due to EOF, complete with all 0

int input_sal_readall (void * bufferin, int len, int * retval2, char * retmsg) {
	int octetsread;

	int ret1;
	int ret2;

	char * buffer;
	buffer = (char *) bufferin;


	// init vars
	octetsread=0;

	while (octetsread < len) {
		ret1=input_sal(ISAL_CMD_READ, &buffer[octetsread], len-octetsread, &ret2, retmsg);

		// success
		if (ret1 == ISAL_RET_SUCCESS) {
			octetsread += ret2;
			continue;
		}; 

		// success but with EOF
		if (ret1 == ISAL_RET_READ_EOF) {
			// move up pointer
			octetsread += ret2;

			// is still not enough, complete will all 0 
			if (octetsread < len)  {
				int octetsleft;
				octetsleft=(len-1-octetsread);
				memset(&buffer[octetsread], 0, octetsleft);
				octetsread = len;

				// exit
				// store number of octets added in ret2
				*retval2=octetsleft;

				return(ISAL_RET_READ_EOF);
			};
		}; // end if - EOF

		if (ret1 == ISAL_RET_FAIL) {
			// error
			*retval2=ret2;
			return(ISAL_RET_FAIL);
		}; // end if

	}; // end while

	// all read
	// No EOF
	*retval2=0;

	// exit
	return(ISAL_RET_SUCCESS);
 
}; // end function input_sal_readall
