/*
Reed Solomon Encoding and Decoding for Burst. cpp version based on:
Version: 1.0, license: Public Domain, coder: NxtChg (admin@nxtchg.com)
Java Version: ChuckOne (ChuckOne@mail.de).

Copyright (C) 2018  CurbShifter

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef BurstAddress_H_INCLUDED
#define BurstAddress_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

class BurstAddress
{ // ReedSolomon
public:
	BurstAddress();
	~BurstAddress();
	void Initialize();

	String encode(String plain);
	String encode(uint64 plain);
	int64 decode(String cypher_string);
private:
	int gmult(const int a, const int b);
	bool is_codeword_valid(const int *codeword);
	uint64 GetUINT64(const String uint64Str);
	MemoryBlock GetUINT64MemoryBlock(const String uint64Str);
	
	int initial_codeword_length;
	int initial_codeword[17];
	int gexp[4 * 8];
	int glog[4 * 8];
	int codeword_map[17];
	String alphabet;
	int base_32_length;
	int base_10_length;
};

#endif //BurstAddress_H_INCLUDED