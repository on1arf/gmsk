/* initinfile.h */

int init_udpsocket (char * retmsg) {
struct addrinfo hint;
struct addrinfo *info;
int warning;
char warningtext[160];

int ret;


// init vars
warning=1;


if (!(global.udphost)) {
	snprintf(retmsg,160,"Error: UDP host missing!\n");
	return(-1);
}; // end if

if ((global.udpport <= 0) || (global.udpport > 65535)) {
	snprintf(retmsg,160,"Error: UDP port should be between 1 and 65535, got %d\n",global.udpport);
	return(-1);
}; // end if

if ((global.ipv4only) & (global.ipv6only)) {
	snprintf(retmsg,160,"Error: IPv4-only and ipv6-only options are mutually exclusive\n");
	return(-1);
}; // end if

// clear hint
memset(&hint,0,sizeof(struct addrinfo));


// init "hint"
hint.ai_socktype = SOCK_DGRAM;

// resolve hostname, use function "getaddrinfo"
// set address family in hint if ipv4only or ipv6ony
if (global.ipv4only) {
	hint.ai_family = AF_INET;
} else if (global.ipv6only) {
	hint.ai_family = AF_INET6;
} else {
	hint.ai_family = AF_UNSPEC;
}; // end if

// do DNS-query, use getaddrinfo for both ipv4 and ipv6 support
ret=getaddrinfo(global.udphost, NULL, &hint, &info);

if (ret != 0) {
	snprintf(retmsg,160,"Error: resolving hostname: (%s)\n",gai_strerror(ret));
	exit(-1);
}; // end if

// getaddrinfo can return multiple results, we only use the first one

// give warning is more then one result found.
// Data is returned in info as a linked list
// If the "next" pointer is not NULL, there is more then one
// element in the chain

if ((info->ai_next != NULL) || global.verboselevel >= 1) {
	char ipaddrtxt[INET6_ADDRSTRLEN];


	// get ip-address in numeric form
	if (info->ai_family == AF_INET) {
		// ipv4
		// for some reason, we neem to shift the address-information with 2 positions
		// to get the correct string returned
		inet_ntop(AF_INET,&info->ai_addr->sa_data[2],ipaddrtxt,INET6_ADDRSTRLEN);
	} else {
		// ipv6
		// for some reason, we neem to shift the address-information with 6 positions
		// to get the correct string returned
		inet_ntop(AF_INET6,&info->ai_addr->sa_data[6],ipaddrtxt,INET6_ADDRSTRLEN);
	}; // end else - if

	// store warning text tempory in tempory buffer. If no other errors pop up, we will print out this warning
	if (info->ai_next != NULL) {
		snprintf(warningtext,160,"Warning. getaddrinfo returned multiple entries. Using %s\n",ipaddrtxt);
	} else {
		snprintf(warningtext,160,"Sending DV-stream to ip-address %s\n",ipaddrtxt);
	}; // end if
	warning=1;
}; // end if

// store returned DNS info in socket address
global.udpsockaddr=(struct sockaddr_in6 *) info->ai_addr;

// fill in UDP port
global.udpsockaddr->sin6_port=htons((unsigned short int) global.udpport);


// all OK
if (warning) {
	// return warning message
	strncpy(retmsg,warningtext,160);
	return(1);
} else {
	// nothing to say. All OK, just return
	return(0);
}; // end if

}; // end function init_udpsocket
