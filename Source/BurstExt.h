/*
BurstCoupon DAPP
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

#ifndef BurstExt_H_INCLUDED
#define BurstExt_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "BurstKit.h"

// Search range should be 32767 as absolute max. but 1 day should be enough
#define SEARCH_RANGE_MINUTES (24*60)
#define PREFIX_CB_RS "CLOUD"
#define PERCENT_FEE 1

typedef bool(*CheckThreadShouldExit_ptr)(const void* context, float progress);

class BurstExt : public BurstKit
{
public:
	BurstExt(String hostUrl = "https://wallet.dev.burst-test.net:8125/"); // default on testnet
	~BurstExt();

	// CloudBurst
	bool CloudDownload(String cloudID, String dlFolder, String &dlFilename, MemoryBlock &dlData, String &msg);
	String CloudUpload(String message, File fileToUpload, uint64 stackSize, uint64 fee, Array<StringArray> &allAddresses, uint64 deadline, int64 &addressesNum, uint64 &txFeePlancks, uint64 &feePlancks, uint64 &burnPlancks);
	int64 CloudCalcCosts(String message, File fileToUpload, uint64 stackSize, uint64 fee, Array<StringArray> &allAddresses, int64 &addressesNum, uint64 &txFeePlancks, uint64 &feePlancks, uint64 &burnPlancks);

	// Coupons
	String CreateCoupon(String txSignedHex, String password);
	String RedeemCoupon(String couponHex, String password);
	String ValidateCoupon(String couponCode, String password, bool &valid);

private:
	struct MTX
	{
		String id;
		StringArray addresses;
		String amount;
		String fee;
		String dl;
	};

	bool ScrapeAccountTransactions(Array<MemoryBlock> &results, Array<String> &timecodes, String downloadTransactionID, CheckThreadShouldExit_ptr CheckThreadShouldExit, const void* context);
	Array<StringArray> SetAttachmentData(MemoryBlock data);
	MemoryBlock GetAttachmentData(var jsonStructure, int &length_match, unsigned int &crc16_full_match);
};

#endif
