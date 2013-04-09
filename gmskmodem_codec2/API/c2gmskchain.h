//////////////////////
// API version of the GMSK modem for 10m / VHF / UHF communication
// using codec2
// version 0 (versionid 0x27f301): 4800 bps, 1/3 repetition code FEC



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


// C2GMSK "chaining" functions: used to create message chains

// Release information
// version 20130310 initial release
// Version 20130314: API c2gmsk version / bitrate control + versionid codes
// Version 20130324: convert into .so shared library


// functions:
// checksign_chain
// c2gmskchain_new
// c2gmskchain_destroy
// c2gmskchain_add
// c2gmskchain_reinit


// check chain signature
int checksign_chain (struct c2gmsk_msgchain * chain) {
// used for sanity checking

// does it point somewhere?
if (!chain) {
	return(C2GMSK_RET_NOVALIDCHAIN);
}; // end if

if (memcmp(chain->signature,CHAIN_SIGNATURE,4)) {
	return(C2GMSK_RET_NOVALIDCHAIN);
}; // end if

// OK
return(C2GMSK_RET_OK);

}; // end function check signature chain


////////////////////
///////////////////


// C2GMSK CHAIN
struct c2gmsk_msgchain * c2gmskchain_new (int size, int * ret) {
// create memory structure for chain

// before calling chain_new, the chain value must be set to zero. This is to
// avoid doing a "new" on a chain that alread exists
struct c2gmsk_msgchain * newchain;


// allocate memory for chain
newchain=malloc(sizeof (struct c2gmsk_msgchain));

if (!newchain) {
	// malloc failed
	*ret=C2GMSK_RET_NOMEMORY;
	return(NULL);
}; // end if

newchain->data=malloc(size);

if (!newchain->data) {
	// could not allocate memory
	// first free memory of chain already created, then exit
	free(newchain);
	*ret=C2GMSK_RET_NOMEMORY;
	return(NULL);
}; // end if

// success!
newchain->size=size;
newchain->numelem=0;
newchain->s_pos=0;
newchain->s_numelem=0;

memcpy(newchain->signature,CHAIN_SIGNATURE,4);

*ret=C2GMSK_RET_OK;
return(newchain);

}; // end function c2gmskchain_new


////////////////

int c2gmskchain_destroy (struct c2gmsk_msgchain * chain) {
// free memory of chain
int ret;

// sanitry check. Does not point somewhere?
ret=checksign_chain(chain);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// free data memory (if it exists)
if (chain->data) {
	free(chain->data);
}; // end if

// free memory of chain itself and return
free(chain);

return (C2GMSK_RET_OK);
}; // end function gc2gmskchain_destroy

///////////////////////

///// add data on replychain
int c2gmskchain_add (struct c2gmsk_msgchain * chain, void * data, int size) {
int ret;

// return if size is zero or less then 0
if (size < 0) {
	return(C2GMSK_RET_OK);
}; // end if

// sanitry check. Does chain point somewhere?
ret=checksign_chain(chain);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// Does data point somewhere?
// make sure it does not point to NULL
if (!data) {
	return(C2GMSK_RET_NOVALIDDATA);
}; // end if


// OK, let's add data to chain
// do we have sufficient place to store data?
if  ((size + chain->used) > chain->size) {
	uint8_t * newdataptr;

	// now, increase size of chain

	// sanity check; maximum size
	if ((size + chain->size) > C2GMSKCHAIN_MAXSIZE) {
		return(C2GMSK_RET_TOLARGE);
	}; // end if


	newdataptr=realloc(chain->data,chain->size + size);

	if (!newdataptr) {
		// could not add memory, realloc returns a NULL
		return(C2GMSK_RET_NOMEMORY);
	};

	// alloc has succeeded
	chain->size += size;
	chain->data = newdataptr;

}; // end if

// ok, we have place, copy data
memcpy(&chain->data[chain->used],data,size);

chain->used += size;
chain->numelem++; // increase number of elements in chain

return(C2GMSK_RET_OK);
}; // end function c2gmskchain add data


///////
// memory chain reinit
int c2gmskchain_reinit (struct c2gmsk_msgchain * chain, int size) {
int ret;

// sanitry check. Does not point somewhere?
ret=checksign_chain(chain);
if (ret != C2GMSK_RET_OK) {
	return(ret);
}; // end if

// reset buffer
chain->used=0;
chain->numelem=0;

chain->s_pos=0;
chain->s_numelem=0;

// if buffer is more then 4 times the initial size, reset back to default size
if (chain->size > (size << 2)) {
	uint8_t * newdataptr;

	// sanity check; maximum size
	if (size > C2GMSKCHAIN_MAXSIZE) {
		return(C2GMSK_RET_TOLARGE);
	}; // end if

	newdataptr=realloc(chain->data,size);

	if (!newdataptr) {
		// could not add memory, realloc returns a NULL
		return(C2GMSK_RET_NOMEMORY);
	};

	// alloc has succeeded
	chain->size = size;
	chain->data = newdataptr;
}; // end if

return(C2GMSK_RET_OK);
}


