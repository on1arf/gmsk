// c2encap.h


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

// version 20121011: initial release
// version 20130606: license changed from GPL to LPLG



// structures used to transport raw codec2 streams over pipes, TCP or UDP

// C2ENCAP VERSION
#define C2ENCAP_VERSION 20121011

// c2encap-frames at the first 3 octets 0x36f162
// 4th octet is type of data, divided in 2 times 4 bits
// 0x1x = group 1: markers
// 0x11: begin (followed by "BEG")
// 0x12: end (followed by "END")

// 0x2x = group 2: data
// 0x21: 40 ms codec2 voice @ 1200 bps (6 octets)
// 0x22: 40 ms codec2 voice @ 1400 bps (7 octets)
// 0x23: 20 ms codec2 voice @ 2400 bps (6 octets)

#define C2ENCAP_MARK_BEGIN 0x11
#define C2ENCAP_MARK_END 0x12
#define C2ENCAP_DATA_VOICE1200 0x21
#define C2ENCAP_DATA_VOICE1400 0x22
#define C2ENCAP_DATA_VOICE2400 0x23

#define C2ENCAP_SIZE_MARK 7 // total size
#define C2ENCAP_SIZE_VOICE1200 10 // total size
#define C2ENCAP_SIZE_VOICE1400 11 // total size
#define C2ENCAP_SIZE_VOICE2400 10 // total size

unsigned char C2ENCAP_HEAD[] = {0x36, 0xf1, 0x62};

typedef union {
	char c2data_text3[3];
	unsigned char c2data_data6[6];
	unsigned char c2data_data7[7];
	} c2data_t;

typedef struct {
	unsigned char header[4];

	c2data_t c2data;
} c2encap;


