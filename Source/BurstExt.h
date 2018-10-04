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

class BurstExt : public BurstKit
{
public:
	BurstExt(String hostUrl = "https://wallet.dev.burst-test.net:8125/"); // default on testnet
	~BurstExt();

	// CloudBurst non-blocking functions
	bool CloudDownloadStart(String cloudID, String dlFolder);
	bool CloudDownloadFinished(String cloudID, String &dlFilename, MemoryBlock &dlData, String &msg, uint64 &epochLast, float &progress);

	String CloudUploadStart(String message, File fileToUpload, uint64 deadline, uint64 stackSize, uint64 fee);
	bool CloudUploadFinished(String jobID, String &cloudID, uint64 &txFeePlancks, uint64 &feePlancks, uint64 &burnPlancks, uint64 &confirmTime, float &progress);

	String CloudCalcCostsStart(String message, File fileToUpload, uint64 stackSize, uint64 fee);
	int64 CloudCalcCostsFinished(String jobID, uint64 &txFeePlancks, uint64 &feePlancks, uint64 &burnPlancks, uint64 &confirmTime, float &progress);

	String CloudCancel();

	// Coupons
	String CreateCoupon(String txSignedHex, String password);
	String RedeemCoupon(String couponHex, String password);
	String ValidateCoupon(String couponCode, String password, bool &valid);
	
	class BurstJob : public ThreadPoolJob, public BurstKit
	{
	public:
		struct downloadArgs
		{
			// in
			String cloudID;
			String dlFolder;

			// out
			String dlFilename;
			MemoryBlock dlData;
			String msg;
			unsigned long long epoch;
		};
		struct uploadArgs
		{
			// in
			String message;
			File fileToUpload;
			uint64 deadline;
			uint64 stackSize;
			uint64 fee;

			// out
			String cloudID;
			uint64 confirmTime;
			Array<StringArray> allAddresses;
			uint64 txFeePlancks;
			uint64 feePlancks;
			uint64 burnPlancks;
		};

		BurstJob(String hostUrl, String passPhrase, downloadArgs &download);
		BurstJob(String hostUrl, String passPhrase, uploadArgs &upload, const bool broadcast);
		~BurstJob();

		ThreadPoolJob::JobStatus runJob();

		downloadArgs download;
		uploadArgs upload;

		void SetProgress(float p)
		{
			const ScopedLock lock(progressLock);
			progressFlt = jmin<float>(jmax<float>(p, 0.f), 1.f);
		}
		float GetProgress()
		{
			const ScopedLock lock(progressLock);
			return progressFlt;
		}

	private:
		bool CloudDownload();
		String CloudUpload();
		int64 CloudCalcCosts();

		bool ScrapeAccountTransactions(Array<MemoryBlock> &results, Array<String> &timecodes, String downloadTransactionID);
		Array<StringArray> SetAttachmentData(MemoryBlock data);
		MemoryBlock GetAttachmentData(StringArray recipientsArray, int &length_match, unsigned int &crc16_full_match, const bool header = false);		

		int jobType;
		bool broadcast;
		float progressFlt;
		String hostUrl;
		String passPhrase;
		CriticalSection progressLock;
		CriticalSection runLock;
	};

private:
	struct MTX
	{
		String id;
		StringArray addresses;
		String amount;
		String fee;
		String dl;
	};

	HashMap<String, BurstJob*> jobs;
	ScopedPointer<ThreadPool> threadpool;
};

#endif
