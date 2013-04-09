/* interleave_c2.h */

// CODEC2 octet interleaving for 4800 bps, changes order of 2nd and 3th octet in DV pattern
// Used to avoid repeating the same 7-octet char-pattern which results in harmonics and interference

const int interl2[7]= { 15, 14, 13, 12, 11, 10, 16}; // reverse order, shift one to the left, then add 7 (2nd group) + 3 (get past sync)
const int interl3[7]= { 18, 22, 17, 21, 23, 20, 19}; // repeat {+4 , +2}, move up if already used at that position in octet 1 or 2; then add 14 (3th octet) + 3 (get past sync)
