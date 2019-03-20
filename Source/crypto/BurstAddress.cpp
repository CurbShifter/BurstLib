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

#include "BurstAddress.h"

BurstAddress::BurstAddress() : base_32_length(13), base_10_length(20)
{
	Initialize();
}

void BurstAddress::Initialize()
{
	initial_codeword_length = 17;
	memset(&initial_codeword[0], 0, sizeof(int) * initial_codeword_length);
	initial_codeword[0] = 1;

	// can be done const and neater with c++11
	const int gexp_t[4 * 8] = {
		1, 2, 4, 8,
		16, 5, 10, 20,
		13, 26, 17, 7,
		14, 28, 29, 31,
		27, 19, 3, 6,
		12, 24, 21, 15,
		30, 25, 23, 11,
		22, 9, 18, 1
	};
	memcpy(&gexp[0], &gexp_t[0], sizeof(int) * 4 * 8);
	const int glog_t[4 * 8] =
	{
		0, 0, 1, 18,
		2, 5, 19, 11,
		3, 29, 6, 27,
		20, 8, 12, 23,
		4, 10, 30, 17,
		7, 22, 28, 26,
		21, 25, 9, 16,
		13, 14, 24, 15
	};
	memcpy(&glog[0], &glog_t[0], sizeof(int) * 4 * 8);

	int codeword_map_t[17] = 
	{ 
		3, 2, 1, 0, 
		7, 6, 5, 4, 
		13, 14, 15, 16, 
		12, 8, 9, 10, 
		11 
	};
	memcpy(&codeword_map[0], &codeword_map_t[0], sizeof(int) * 17);
	alphabet = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";
}

BurstAddress::~BurstAddress()
{
}

String BurstAddress::encode(uint64 plain)
{
	return encode(String(plain));
}

String BurstAddress::encode(String plain)
{
	int length = plain.length();
	
	ScopedPointer<int> plain_string_10 = new int[base_10_length];
	for (int i = 0; i < length && i < base_10_length; i++)
		plain_string_10[i] = (int)plain[i] - (int)'0';

	int codeword_length = 0;
	ScopedPointer<int> codeword = new int[initial_codeword_length];
	memset(codeword, 0, sizeof(int) * initial_codeword_length);

	do {  // base 10 to base 32 conversion
		int new_length = 0;
		int digit_32 = 0;
		for (int i = 0; i < length; i++) 
		{
			digit_32 = digit_32 * 10 + plain_string_10[i];
			if (digit_32 >= 32)
			{
				plain_string_10[new_length] = digit_32 >> 5;
				digit_32 &= 31;
				new_length += 1;
			}
			else if (new_length > 0)
			{
				plain_string_10[new_length] = 0;
				new_length += 1;
			}
		}
		length = new_length;
		codeword[codeword_length] = digit_32;
		codeword_length += 1;
	} while (length > 0);

	//int p_t[4] = { 24, 30, 30, 30 };
	int p[4] = { 0, 0, 0, 0 };
	for (int i = base_32_length - 1; i >= 0; i--)
	{
		int fb = codeword[i] ^ p[3];
		p[3] = p[2] ^ gmult(30, fb);
		p[2] = p[1] ^ gmult(6, fb);
		p[1] = p[0] ^ gmult(9, fb);
		p[0] = gmult(17, fb);
	}
	int *codeword_p = &(codeword[base_32_length]);
	memcpy(codeword_p, &p[0], (initial_codeword_length - base_32_length) * sizeof(int));
//	const bool isValid = is_codeword_valid(codeword);

	String cypher_string_builder;
	for (int i = 0; i < 17; i++)
	{
		int codework_index = codeword_map[i];
		int alphabet_index = codeword[codework_index];		
		cypher_string_builder.append(alphabet.substring(alphabet_index, alphabet_index + 1), 1);

		if ((i &3) == 3 && i < 13)
			cypher_string_builder.append(String("-"), 1);
	}
//	const bool isValidStill = String(decode(cypher_string_builder)).compare(plain) == 0;

	return cypher_string_builder;
}

uint64 BurstAddress::decode(String cypher_string)
{
	cypher_string = cypher_string.removeCharacters("-");
	if (cypher_string.startsWithIgnoreCase("BURST") && cypher_string.length() > 17)
		cypher_string = cypher_string.substring(5);
	cypher_string = cypher_string.toUpperCase();

	ScopedPointer<int> codeword = new int[initial_codeword_length];
	memcpy(&codeword[0], &initial_codeword[0], initial_codeword_length * sizeof(int));

	int codeword_length = 0;
	for (int i = 0; i < cypher_string.length(); i++)
	{
		int position_in_alphabet = alphabet.indexOf(cypher_string.substring(i, i+1));

		if (position_in_alphabet <= -1 || position_in_alphabet > alphabet.length()) {
			continue;
		}

		if (codeword_length > 16) {
			return 0; // CodewordTooLong
		}

		int codework_index = codeword_map[codeword_length];
		codeword[codework_index] = position_in_alphabet;
		codeword_length += 1;
	}

	if ((codeword_length == 17 && !is_codeword_valid(codeword)) || codeword_length != 17)
		return 0; // CodewordInvalid

	int length = base_32_length;
	ScopedPointer<int> cypher_string_32 = new int[length];
	for (int i = 0; i < length; i++) {
		cypher_string_32[i] = codeword[length - i - 1];
	}

	String plain_string_builder;
	do { // base 32 to base 10 conversion
		int new_length = 0;
		int digit_10 = 0;

		for (int i = 0; i < length; i++)
		{
			digit_10 = digit_10 * 32 + cypher_string_32[i];

			if (digit_10 >= 10)
			{
				cypher_string_32[new_length] = digit_10 / 10;
				digit_10 %= 10;
				new_length += 1;
			}
			else if (new_length > 0)
			{
				cypher_string_32[new_length] = 0;
				new_length += 1;
			}
		}
		length = new_length;
		char c = (char)(digit_10 + (int)'0');
		plain_string_builder = String((char*)&c, 1) + plain_string_builder;
	} while (length > 0);

	return GetUINT64(plain_string_builder);
}

int BurstAddress::gmult(const int a, const int b)
{
	if (a == 0 || b == 0)
		return 0;
	int idx = (glog[a] + glog[b]) % 31;
	return gexp[idx];
}

bool BurstAddress::is_codeword_valid(const int *codeword)
{
	int sum = 0;
	for (int i = 1; i < 5; i++)
	{
		int t = 0;
		for (int j = 0; j < 31; j++)
		{
			if (j > 12 && j < 27)
			{
				continue;
			}
			int pos = j;
			if (j > 26)
			{
				pos -= 14;
			}
			t ^= gmult(codeword[pos], gexp[(i * j) % 31]);
		}
		sum |= t;
	}
	return sum == 0;
}

uint64 BurstAddress::GetUINT64(const String uint64Str)
{
	return *((uint64*)(GetUINT64MemoryBlock(uint64Str).getData()));
}

MemoryBlock BurstAddress::GetUINT64MemoryBlock(const String uint64Str)
{ // drop the use of BigInteger. but String only returns signed version of int 64 (max size should be 2^64)
	juce::BigInteger bigInt;
	bigInt.parseString(uint64Str, 10);
	MemoryBlock m(bigInt.toMemoryBlock());
	m.ensureSize(8, true);
	return m;
}

