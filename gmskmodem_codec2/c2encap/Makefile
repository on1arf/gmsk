# Make file for c2encap

# Copyright (C) 2013 Kristoff Bonne ON1ARF

#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License version 2.1, as
#  published by the Free Software Foundation.  This program is
#  distributed in the hope that it will be useful, but WITHOUT ANY
#  WARRANTY; without even the implied warranty of MERCHANTABILITY or
#  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
#  License for more details.

#  You should have received a copy of the GNU Lesser General Public License
#  along with this program; if not, see <http://www.gnu.org/licenses/>.

# version 20121011: initial release
# version 20130606: license changed from GPL to LPLG



all: c2enc_add c2enc_strip

c2enc_add:  Makefile c2encap.h c2enc_add.c
	gcc -O3 -Wall -o c2enc_add c2enc_add.c

c2enc_strip:  Makefile c2encap.h c2enc_strip.c
	gcc -O3 -Wall -o c2enc_strip c2enc_strip.c

install:
	install -o root -g root -m 644 c2encap.h /usr/local/include
	install -o root -g root -m 755 c2enc_add /usr/local/bin
	install -o root -g root -m 755 c2enc_strip /usr/local/bin

