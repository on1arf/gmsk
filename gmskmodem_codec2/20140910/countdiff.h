/* countdiff.h */




/* Copyright (C) 2013 Kristoff Bonne ON1ARF

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 2.1, as
  published by the Free Software Foundation.  This program is
  distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
  License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.
*/



// function countdiff16, 32 or 64
// Part of the gmskmodem

// compaires 16, 32 or 64 bit patterns (up to "size" bits, and after applying a
// "mask" to one of the data fields) and returns 1 if the number of
// difference is less or equal then "maxdiff"; otherwize returns 0

// version 20111107: initial release
// version 20120607: add 32 bit version, add _frommsb versions
// version 20130310: API release: no change
// Version 20130314: API c2gmsk version / bitrate control + versionid codes
// Version 20130324: convert into .so shared library



int countdiff64_fromlsb(uint64_t data, uint64_t mask, int size, uint64_t target, int maxdiff) {
	int loop;
	int bit, lastbit=0;
	int penalty;

	uint64_t diff;

	// masked data
	diff = (data ^ target) & mask;

	// perhaps it is equal? if yes, return "true"
	if (diff == 0) {
		return(1);
	}; // end if

	// not equal, if maxdiff is zero, return "false"
	if (maxdiff == 0) {
		return(0);
	}; // end if

	penalty=0;

	// just to be sure
	if (size > 64) {
		size=64;
	}; // 


	for (loop=0;loop<size;loop++) {
		// get rightmost bit
		bit=diff & 0x0000000000000001;

		// move everything to the right for the next loop
		diff >>= 1;

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


int countdiff64_frommsb(uint64_t data, uint64_t mask, int size, uint64_t target, int maxdiff) {
	int loop;
	uint64_t bit;
	int lastbit=0;
	int penalty;

	uint64_t diff;

	// masked data
	diff = (data ^ target) & mask;

	// perhaps it is equal? if yes, return "true"
	if (diff == 0) {
		return(1);
	}; // end if

	// not equal, if maxdiff is zero, return "false"
	if (maxdiff == 0) {
		return(0);
	}; // end if

	penalty=0;

	// just to be sure
	if (size > 64) {
		size=64;
	}; // 


	for (loop=0;loop<size;loop++) {
		// get leftmost bit
		bit=diff & 0x8000000000000000ULL;

		// move everything to the right for the next loop
		diff >>= 1;

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



int countdiff32_fromlsb(uint32_t data, uint32_t mask, int size, uint32_t target, int maxdiff) {
	int loop;
	int bit, lastbit=0;
	int penalty;

	uint32_t diff;

	// masked data
	diff = (data ^ target) & mask;

	// perhaps it is equal? if yes, return "true"
	if (diff == 0) {
		return(1);
	}; // end if

	// not equal, if maxdiff is zero, return "false"
	if (maxdiff == 0) {
		return(0);
	}; // end if

	penalty=0;

	// just to be sure
	if (size > 32) {
		size=32;
	}; // 

	for (loop=0;loop<size;loop++) {
		// check mostright "diff" bit
		bit=diff & 0x00000001; 

		// move all bits to the right (for the next loop)
		diff >>= 1;

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
	}; // end for


	// all bits, check, penalty is still less then maxdiff, return "true"
	return(1);
	
}; // end function


int countdiff32_frommsb(uint32_t data, uint32_t mask, int size, uint32_t target, int maxdiff) {
	int loop;
	int bit, lastbit=0;
	int penalty;

	uint32_t diff;

	// masked data
	diff = (data & mask) ^ (target & mask);

	// perhaps it is equal? if yes, return "true"
	if (diff == 0) {
		return(1);
	}; // end if

	// not equal, if maxdiff is zero, return "false"
	if (maxdiff == 0) {
		return(0);
	}; // end if

	penalty=0;

	// just to be sure
	if (size > 32) {
		size=32;
	}; // 

	for (loop=0;loop<size;loop++) {
		// check mostright "diff" bit
		bit=diff & 0x80000000; 

		// move all bits to the right (for the next loop)
		diff <<= 1;

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
	}; // end for


	// all bits, check, penalty is still less then maxdiff, return "true"
	return(1);
	
}; // end function






int countdiff16_fromlsb(uint16_t data, uint16_t mask, int size, uint16_t target, int maxdiff) {
	int loop;
	int bit, lastbit=0;
	int penalty;

	uint16_t diff;

	// masked data
	diff = (data ^ target) & mask;

	// perhaps it is equal? if yes, return "true"
	if (diff == 0) {
		return(1);
	}; // end if

	// not equal, if maxdiff is zero, return "false"
	if (maxdiff == 0) {
		return(0);
	}; // end if

	penalty=0;

	// just to be sure
	if (size > 16) {
		size=16;
	}; // 

	for (loop=0;loop<size;loop++) {
		// check mostright "diff" bit
		bit=diff & 0x0001; 

		// move all bits to the right (for the next loop)
		diff >>= 1;

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
	}; // end for


	// all bits, check, penalty is still less then maxdiff, return "true"
	return(1);
	
}; // end function


int countdiff16_frommsb(uint16_t data, uint16_t mask, int size, uint16_t target, int maxdiff) {
	int loop;
	int bit, lastbit=0;
	int penalty;

	uint16_t diff;

	// masked data
	diff = (data & mask) ^ (target & mask);

	// perhaps it is equal? if yes, return "true"
	if (diff == 0) {
		return(1);
	}; // end if

	// not equal, if maxdiff is zero, return "false"
	if (maxdiff == 0) {
		return(0);
	}; // end if

	penalty=0;

	// just to be sure
	if (size > 16) {
		size=16;
	}; // 

	for (loop=0;loop<size;loop++) {
		// check mostright "diff" bit
		bit=diff & 0x8000; 

		// move all bits to the right (for the next loop)
		diff <<= 1;

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
	}; // end for


	// all bits, check, penalty is still less then maxdiff, return "true"
	return(1);
	
}; // end function




