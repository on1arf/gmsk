/* countdiff.h */

// function countdiff64

// compaires 64 bit patterns (up to "size" bits, and after applying a
// "mask" to one of the data fields) and returns 1 if the number of
// difference is less or equal then "maxdiff"; otherwize returns 0

// version 20111107: initial release

/*
 *      Copyright (C) 2011 by Kristoff Bonne, ON1ARF
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



int countdiff64(uint64_t data, uint64_t mask, int size, uint64_t target, int maxdiff) {
	int loop;
	int bit, lastbit;
	int penalty;

	uint64_t diff;

	// masked data
	diff = (data & mask) ^ target;

	// perhaps it is equal? if yes, return "true"
	if (diff == 0) {
		return(1);
	}; // end if

	// not equal, if maxdiff is zero, return "false"
	if (maxdiff == 0) {
		return(0);
	}; // end if

	penalty=0;

	for (loop=0;loop<size;loop++) {
		diff >>= 1;
		bit=diff & 0x01;

		if (bit) {
			// difference in bit found

			// add 2 "diff-points" if bit by itself
			// add 1 "diff-point" if bit more to the right was also 1

			if (lastbit) {
				penalty++;
			} else {
				penalty+=2;
			}; // end if
		}; // end if

		lastbit=bit;

		if (penalty > maxdiff) {
		// we already have to much penalty points (over "maxdiff") -> return false
			return(0);
		}; // end if
	}; // end if


	// all bits, check, penalty is still less then maxdiff, return "true"
	return(1);
	
}



