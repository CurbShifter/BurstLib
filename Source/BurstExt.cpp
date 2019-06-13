/*
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

#include "BurstExt.h"
#include <boost/crc.hpp>
#include "crypto/blowfish.h"

BurstExt::BurstExt(String hostUrl) : BurstKit(hostUrl)
{
	nodeHopCount = 0;
	maxHopNodeRequests = 0;

	threadpoolCloud = nullptr;
}

BurstExt::~BurstExt()
{
	if (threadpoolCloud)
	{
		threadpoolCloud->removeAllJobs(true, 5000);
		threadpoolCloud = nullptr;
	}
}

void BurstExt::ThreadpoolCloud()
{
	if (threadpoolCloud == nullptr)
		threadpoolCloud = new ThreadPool();
}

// CloudBurst --------------------------------------------------------------------------------------------------------------
String BurstExt::CloudCancel()
{
	ThreadpoolCloud();
	int n = threadpoolCloud->getNumJobs();
	threadpoolCloud->removeAllJobs(true, 10000);
	return String(n) + " jobs cancelled";
}

bool BurstExt::CloudDownloadStart(String cloudID, String dlFolder)
{
	ThreadpoolCloud();
	BurstJob::downloadArgs args;
	args.cloudID = cloudID;
	args.dlFolder = dlFolder;
	
	BurstJob* newJob = new BurstJob(GetNode(), GetSecretPhraseString(), args);
	jobs.set(cloudID, newJob);
	threadpoolCloud->addJob(newJob, false);

	return newJob != nullptr;
}

bool BurstExt::CloudDownloadFinished(String cloudID, String &dlFilename, MemoryBlock &dlData, String &msg, uint64 &epochLast, float &progress)
{
	ThreadpoolCloud();
	BurstExt::BurstJob *job = jobs[cloudID];
	if (!threadpoolCloud->contains(job))
	{
		dlFilename = job->download.dlFilename;
		dlData = job->download.dlData;
		msg = job->download.msg;
		epochLast = job->download.epoch;

		if (job) delete job;

		jobs.remove(cloudID);

		progress = 1.f;
		return true;
	}
	progress = job->GetProgress();
	return false;
}

String BurstExt::CloudUploadStart(String message, File fileToUpload, uint64 deadline, uint64 stackSize, uint64 fee)
{
	ThreadpoolCloud();
	BurstJob::uploadArgs args;
	args.message = message;
	args.fileToUpload = fileToUpload;
	args.deadline = deadline;
	args.stackSize = stackSize;
	args.fee = fee;

	BurstJob* newJob = new BurstJob(GetNode(), GetSecretPhraseString(), args, true);
	String jobID = String((uint64)newJob);
	jobs.set(jobID, newJob);
	threadpoolCloud->addJob(newJob, false);

	return (newJob != nullptr) ? jobID : String::empty;
}

bool BurstExt::CloudUploadFinished(String jobID, String &cloudID, uint64 &txFeePlancks, uint64 &feePlancks, uint64 &burnPlancks, uint64 &confirmTime, float &progress)
{
	ThreadpoolCloud();
	BurstJob* job = jobs[jobID];
	if (!threadpoolCloud->contains(job))
	{
		cloudID = job->upload.cloudID;
		txFeePlancks = job->upload.txFeePlancks;
		feePlancks = job->upload.feePlancks;
		burnPlancks = job->upload.burnPlancks;
		confirmTime = job->upload.confirmTime;

		if (job) delete job;

		jobs.remove(jobID); // scoped pointer deletes the job

		progress = 1.f;
		return true;
	}
	progress = job->GetProgress();
	return false;
}

String BurstExt::CloudCalcCostsStart(String message, File fileToUpload, uint64 stackSize, uint64 fee)
{
	ThreadpoolCloud();
	BurstJob::uploadArgs args;
	args.message = message;
	args.fileToUpload = fileToUpload;
	args.stackSize = stackSize;
	args.fee = fee;

	BurstJob* newJob = new BurstJob(GetNode(), GetSecretPhraseString(), args, false);
	String jobID = String((uint64)newJob);
	jobs.set(jobID, newJob);
	threadpoolCloud->addJob(newJob, false);

	return (newJob != nullptr) ? jobID : String::empty;
}

int64 BurstExt::CloudCalcCostsFinished(String jobID, uint64 &txFeePlancks, uint64 &feePlancks, uint64 &burnPlancks, uint64 &confirmTime, float &progress)
{
	ThreadpoolCloud();
	BurstJob* job = jobs[jobID];
	if (!threadpoolCloud->contains(job))
	{
		txFeePlancks = job->upload.txFeePlancks;
		feePlancks = job->upload.feePlancks;
		burnPlancks = job->upload.burnPlancks;
		confirmTime = job->upload.confirmTime;

		if (job) delete job;

		jobs.remove(jobID); // scoped pointer deletes the job

		progress = 1.f;
		return true;
	}
	progress = job->GetProgress();
	return false;
}

// BurstJob ----------------------------------------------------------------------------------------------------------------
BurstExt::BurstJob::BurstJob(String hostUrl, String passPhrase, downloadArgs &download) 
	: BurstKit(hostUrl, passPhrase), ThreadPoolJob("BurstJob"), hostUrl(hostUrl), /*passPhrase(passPhrase),*/ progressFlt(0.f)
{
	upload.stackSize = 0;
	upload.fee = 0;
	upload.deadline = 0;
	upload.txFeePlancks = 0;
	upload.feePlancks = 0;
	upload.burnPlancks = 0;
	upload.confirmTime = 0;
	
	this->download = download;
	this->broadcast = false;

	jobType = 0;
}

BurstExt::BurstJob::BurstJob(String hostUrl, String passPhrase, uploadArgs &upload, const bool broadcast) 
	: BurstKit(hostUrl, passPhrase), ThreadPoolJob("BurstJob"), hostUrl(hostUrl), /*passPhrase(passPhrase),*/ progressFlt(0.f)
{
	upload.txFeePlancks = 0;
	upload.feePlancks = 0;
	upload.burnPlancks = 0;
	upload.confirmTime = 0;

	this->upload = upload;
	this->broadcast = broadcast;

	if (upload.deadline > 0 && broadcast)
		jobType = 1;
	else jobType = 2; // calc costs
}

BurstExt::BurstJob::~BurstJob()
{
}

ThreadPoolJob::JobStatus BurstExt::BurstJob::runJob()
{
	// must keep checking the shouldExit() method to see if something is trying to interrupt the AlbumJob.
	// If shouldExit() returns true, the runAlbumJob() method must return immediately.
	const ScopedTryLock tryLock(runLock);
	if (tryLock.isLocked() && !shouldExit())
	{
		SetProgress(0.f);

		if (jobType == 0)
			CloudDownload();
		else if (jobType == 1)
			upload.cloudID = CloudUpload();
		else if (jobType == 2)
			CloudCalcCosts(); //int64 costs = upload.txFeePlancks + upload.burnPlancks + upload.feePlancks;

		SetProgress(1.f);
		return jobHasFinished;
	}
	return jobNeedsRunningAgain;
}

bool BurstExt::BurstJob::CloudDownload()
{	
	Array<MemoryBlock> memoryBLocks;
	Array<String> timecodes;
	timecodes.clear();

	BurstJob::ScrapeAccountTransactions(memoryBLocks, timecodes, download.cloudID);

	download.msg.clear();
	download.epoch = 0;
	bool downloadOk = false;
	for (int i = 0; i < memoryBLocks.size(); i++)
	{
		MemoryBlock mem(memoryBLocks[i]);
		
		String message;
		String filename;
		char *p = (char*)mem.getData();
		int step = 0;
		for (size_t j = 0; j<mem.getSize() && step < 2; j++)
		{ // find first null
			if (p[j] != NULL)
			{
#if JUCE_WINDOWS
				juce_wchar character = juce::CharacterFunctions::getUnicodeCharFromWindows1252Codepage((uint8)p[j]); // ISO-8859-1 vs Windows-1252
#else
				juce_wchar character = p[j];
#endif
				if (step == 0)
					message += character;
				else if (step == 1)
					filename += character;
			}
			else
			{
				step++;
				if (step == 2)
					mem.removeSection(0, j + 1);
			}
		}

		if (mem.getSize() > 0 && filename.isNotEmpty())
		{
			download.dlFilename = filename;
			download.dlData = mem;

			if (File(download.dlFolder).exists() && File(download.dlFolder).getChildFile(download.dlFilename).existsAsFile() == false && download.dlFilename.isNotEmpty())
				File(download.dlFolder).getChildFile(download.dlFilename).appendData(download.dlData.getData(), download.dlData.getSize());

			downloadOk = true;
		}
		else if (step <= 1)
			downloadOk = true;

		if (timecodes[i].isNotEmpty() && (int64)download.epoch < BURSTCOIN_GENESIS_EPOCH + timecodes[i].getLargeIntValue())
		{ // 2014-08-11 04:00:00 genesis epoch
			uint64 tc = timecodes[i].getLargeIntValue();
			download.epoch = BURSTCOIN_GENESIS_EPOCH + tc;
			download.msg += "[" + Time(download.epoch * 1000).toString(true, true) + "] ";
		}
		if (message.isNotEmpty())
		{
			download.msg += message;
		}
	}
	return downloadOk;
}

bool BurstExt::BurstJob::ScrapeAccountTransactions(Array<MemoryBlock> &attachmentDatas, Array<String> &timecodes, String downloadTransactionID)
{
	downloadTransactionID = downloadTransactionID.toUpperCase();
	if (downloadTransactionID.containsAnyOf("ABCDEFGHIJKLMNOPQRSTUVWXYZ")) // a reed solomon or numerical?
	{
		String converted(downloadTransactionID.replace(PREFIX_CB_RS "-", ""));
		if (converted.startsWith("BURST-") == false)
			converted = "BURST-" + converted;
		downloadTransactionID = GetJSONvalue(rsConvert(converted), "account");
	}
	
	// get transaction with downloadTransactionID
	// read sender address
	String baseTransactionContents = getTransaction(downloadTransactionID, String::empty);
	String reedSolomonIn = GetJSONvalue(baseTransactionContents, "senderRS");

	// track the probable max or min timestamp of adjacent messages.
	// the range of all messages is always within 32767 minutes, the max deadline.
	// we use this to limit the search range and limit cpu and io usage
	const String timestamp = GetJSONvalue(baseTransactionContents, "timestamp");
	const uint64 tsBase = timestamp.getLargeIntValue();
	uint64 extendedSearchTimestampMax = 0;
	uint64 extendedSearchTimestampMin = 0xFFFFFFFFFFFFFFFF;
	if (tsBase - (SEARCH_RANGE_MINUTES * 60) < extendedSearchTimestampMin)
		extendedSearchTimestampMin = tsBase - (SEARCH_RANGE_MINUTES * 60);
	if (tsBase + (SEARCH_RANGE_MINUTES * 60) > extendedSearchTimestampMax)
		extendedSearchTimestampMax = tsBase + (SEARCH_RANGE_MINUTES * 60);

	float progress = 0.f;
	StringArray txidArray;
	var recipientsArray;
	StringArray timeStamps;
	if (reedSolomonIn.isNotEmpty() && !shouldExit())
	{
		// get all transactions from this account
		String timestamp = String(extendedSearchTimestampMin); // cap the tx count // is the earliest block(in seconds since the genesis block) to retrieve(optional)
		String type = String::empty; // is the type of transactions to retrieve(optional)
		String subtype = String::empty; // is the subtype of transactions to retrieve(optional)
		String firstIndex = String::empty; // is the a zero - based index to the first transaction ID to retrieve(optional)
		String lastIndex = String::empty; // is the a zero - based index to the last transaction ID to retrieve(optional)
		String numberOfConfirmations = String::empty; // is the required number of confirmations per transaction(optional)
		const String transactionsStr = getAccountTransactionIds(reedSolomonIn, timestamp, type, subtype, firstIndex, lastIndex, numberOfConfirmations);
		var transactions;
		Result r = JSON::parse(transactionsStr, transactions);

		Array<String> allTransactionsToCheckPre;
		Array<String> allTransactionsToCheckPost;
		float numProgress = 1.f;
		if (transactions["transactionIds"].isArray())
		{
			numProgress = (float)transactions["transactionIds"].size();
			bool copyFromHere = false;
			for (int i = 0; i < transactions["transactionIds"].size(); i++)
			{
				if (downloadTransactionID.compare(transactions["transactionIds"][i]) == 0)
					copyFromHere = true;

				if (copyFromHere)
					allTransactionsToCheckPost.add(transactions["transactionIds"][i]);
				else allTransactionsToCheckPre.insert(0, transactions["transactionIds"][i]); // reverse insert
			}
		}

		unsigned int matchThisCRC = 0;
		bool stopLookingPost = false;
		bool stopLookingPre = false;
		int idxPre = 0;
		int idxPost = 0;
		// check each transaction in front and back, and see if its a multi out same. and within time range
		// store each mtx that matches the CRC of the given tx id
		// and stop looking untill there is a multi out same with a different crc
		while ((!stopLookingPost || !stopLookingPre) && !shouldExit())
		{			
			SetProgress(progress + ((idxPre + idxPost) / numProgress));

			bool timeRange = false;
			String txid;
			if (!stopLookingPost)
				txid = allTransactionsToCheckPost[idxPost];
			else if (!stopLookingPre)
				txid = allTransactionsToCheckPre[idxPre];

			if (txid.isNotEmpty())
			{
				var txDetails;
				Result r = JSON::parse(getTransaction(txid, String::empty), txDetails);

				const String timestamp = txDetails["timestamp"];
				const uint64 ts = timestamp.getLargeIntValue();

				if (ts >= extendedSearchTimestampMin && ts <= extendedSearchTimestampMax)
				{	// get Multi Same {"version.MultiSameOutCreation":1,"recipients":["9223372036854775807","1"]}
					const var attachmentVAR = txDetails["attachment"];
					const var multiSameOutCreationVAR = attachmentVAR["version.MultiSameOutCreation"].toString();
					if ((int)multiSameOutCreationVAR == 1)
					{
						var	v = attachmentVAR["recipients"];
						// Check total CRC here so we can stop getting transaction details when it stops matching
						if (v.isArray())
						{
							juce::BigInteger bigInt; // convert string to 64 bit blob
							bigInt.parseString(v[0].toString(), 10);
							MemoryBlock header = bigInt.toMemoryBlock();
							header.ensureSize(sizeof(uint64), true);

							int length = 0;
							unsigned int crc16 = 0;
							memcpy(&length, header.getData(), sizeof(int));
							memcpy(&crc16, &((int*)header.getData())[1], sizeof(int));
							unsigned int crc16_full = (crc16 & 0xFFFF0000) >> 16;

							if (idxPost == 0) // first one, the transaction we pointed to
								matchThisCRC = crc16_full;

							if (matchThisCRC == crc16_full)
							{
								// collect all probale data that matches the crc. still 1:65535 chance its not part of the set
								txidArray.add(txid);
								recipientsArray.append(v);
								// narrow the range down from min and max 32767 minutes of the origin tx
								if (ts - (SEARCH_RANGE_MINUTES * 60) > extendedSearchTimestampMin) // if 32767 min before Minimum is higher than our current minimum
									extendedSearchTimestampMin = ts - (SEARCH_RANGE_MINUTES * 60);
								if (ts + (SEARCH_RANGE_MINUTES * 60) < extendedSearchTimestampMax) // if 32767 min after Maximum is lower
									extendedSearchTimestampMax = ts + (SEARCH_RANGE_MINUTES * 60);

								timeStamps.add(timestamp);
							}
						}
					}
					const var multiOutCreationVAR = attachmentVAR["version.MultiOutCreation"].toString();
					if ((int)multiOutCreationVAR == 1)
					{
						var	v = attachmentVAR["recipients"];
						// Check total CRC here so we can stop getting transaction details when it stops matching
						if (v.isArray())
						{
							juce::BigInteger bigInt; // convert string to 64 bit blob
							const String strv = v[0][0].toString();
							if (strv.isNotEmpty())
							{
								bigInt.parseString(strv, 10);
								MemoryBlock header = bigInt.toMemoryBlock();
								header.ensureSize(sizeof(uint64), true);///

								int length = 0;
								unsigned int crc16 = 0;
								memcpy(&length, header.getData(), sizeof(int));
								memcpy(&crc16, &((int*)header.getData())[1], sizeof(int));
								unsigned int crc16_full = (crc16 & 0xFFFF0000) >> 16;

								if (idxPost == 0) // first one, the transaction we pointed to
									matchThisCRC = crc16_full;

								if (matchThisCRC == crc16_full)
								{
									// collect all probale data that matches the crc. still 1:65535 chance its not part of the set
									txidArray.add(txid);

									// filter out the amounts. and remove the last TX
									var onlyAddresses;
									for (int a = 0; a < v.size() - 1; a++)
										onlyAddresses.append(v[a][0]);

									recipientsArray.append(onlyAddresses);
									// narrow the range down from min and max 32767 minutes of the origin tx
									if (ts - (SEARCH_RANGE_MINUTES * 60) > extendedSearchTimestampMin) // if 32767 min before Minimum is higher than our current minimum
										extendedSearchTimestampMin = ts - (SEARCH_RANGE_MINUTES * 60);
									if (ts + (SEARCH_RANGE_MINUTES * 60) < extendedSearchTimestampMax) // if 32767 min after Maximum is lower
										extendedSearchTimestampMax = ts + (SEARCH_RANGE_MINUTES * 60);

									timeStamps.add(timestamp);
								}
							}
						}
					}
				}
				else // exeeded the time range. stop searching
				{
					timeRange = true;
				}
			}

			if (!stopLookingPost)
				idxPost++;
			else idxPre++;

			if (!stopLookingPost)
				stopLookingPost = idxPost >= allTransactionsToCheckPost.size();

			if (!stopLookingPre)
				stopLookingPre = idxPre >= allTransactionsToCheckPre.size();

			if (timeRange)
			{
				if (!stopLookingPost)
					stopLookingPost = true;
				else stopLookingPre = true;
			}
		}
		progress = (idxPre + idxPost) / numProgress;
	}

	float progressPart2 = (1.f - progress) / 2.f;
	float progressPart3 = (1.f - progress) / 2.f;

	// sort and get data from recipients. reconstruct the data
	// we only collect 1 payload here but multiple payloads with different CRCs will be splitted also
	Array<unsigned int> attachmentCRCs;
	if (recipientsArray.isArray() && !shouldExit())
	{
		int totalMultiOuts = recipientsArray.getArray()->size();
		Array<int> totalReadData;
		for (int i = 0; i < totalMultiOuts && !shouldExit(); i++)
		{
			SetProgress(progress + ((progressPart2 / totalMultiOuts) * i));

			int dataSize = 0;
			unsigned int crc16_full_match = 0;

			StringArray recipients;
			for (int ri = 0; ri < recipientsArray[i].size(); ri++)
			{
				recipients.add(recipientsArray[i][ri]);
			}
			//StringArray recipients = StringArray::fromTokens(rStr, ",", "\"");
			MemoryBlock data = GetAttachmentData(recipients, dataSize, crc16_full_match);

			int attachmentIndex = attachmentCRCs.indexOf(crc16_full_match);
			int neededBytes = dataSize + data.getSize();
			if (attachmentIndex < 0)
			{ // new attachment
				attachmentDatas.add(MemoryBlock(data.getData(), data.getSize()));
				attachmentDatas.getReference(attachmentDatas.size() - 1).ensureSize(neededBytes, true);
				timecodes.add(timeStamps[i]);
				attachmentCRCs.addIfNotAlreadyThere(crc16_full_match);
				totalReadData.add(data.getSize());
			}
			else
			{
				totalReadData.getReference(attachmentIndex) += data.getSize();
				// determine the position to insert the data
				if (neededBytes > (int)attachmentDatas.getReference(attachmentIndex).getSize()) // check if we need to add more space
				{	// add the data in front of the data we already got
					int addedSize = (neededBytes)-attachmentDatas.getReference(attachmentIndex).getSize();
					data.ensureSize(addedSize, true);
					attachmentDatas.getReference(attachmentIndex).insert(data.getData(), data.getSize(), 0);
				}
				else
				{
					const int desitnationPos = attachmentDatas.getReference(attachmentIndex).getSize() - (neededBytes);
					attachmentDatas.getReference(attachmentIndex).copyFrom(data.getData(), desitnationPos, data.getSize());
				}
			}
		}

		for (int i = attachmentDatas.size() - 1; i >= 0 && !shouldExit(); i--)
		{ // check 16 bit crc for combined data. and if expected data size matches
			SetProgress(progress + progressPart2 + ((progressPart3 / attachmentDatas.size()) * ((attachmentDatas.size() - 1) - i)));

			int attachmentSize = attachmentDatas[i].getSize();
			// unzip
			MemoryInputStream srcStream(attachmentDatas.getReference(i), false);
			juce::MemoryBlock mb;
			GZIPDecompressorInputStream dezipper(&srcStream, false, GZIPDecompressorInputStream::gzipFormat);
			dezipper.readIntoMemoryBlock(mb);
			attachmentDatas.getReference(i) = mb;

			// check the 16 bits crc of the unzipped data
			boost::crc_16_type crcProcessor;
			crcProcessor.process_bytes(attachmentDatas[i].getData(), attachmentDatas[i].getSize());
			int crc16_full = crcProcessor.checksum();
			int crc16_expected = attachmentCRCs[i];
			int trd = totalReadData[i];

			if (trd < attachmentSize || // smaller than. bcz its possible a previous upload got aborted and we had double data
				crc16_expected != crc16_full)
			{
				attachmentDatas.remove(i);
			}
		}
	}

	SetProgress(1.f);

	if (shouldExit())
	{
		attachmentDatas.clear();
		timecodes.clear();
	}
	return !shouldExit();
}

MemoryBlock BurstExt::BurstJob::GetAttachmentData(StringArray recipientsArray, int &length_match, unsigned int &crc16_full_match, const bool mainHeader) // length_match to 0 get data without sequence check
{ // convert the numerical burst addresses from a multi out to arbitrary data
	MemoryBlock attachmentData;

	juce::BigInteger bigInt; // TODO drop the use of BigInteger. but String only returns signed version of int 64 (max size should be 2^64)
	bigInt.parseString(recipientsArray[0], 10);
	MemoryBlock header = bigInt.toMemoryBlock();
	header.ensureSize(sizeof(uint64), true);

	int length = 0;
	unsigned int crc16 = 0;
	memcpy(&length, header.getData(), sizeof(int));
	memcpy(&crc16, &((int*)header.getData())[1], sizeof(int));
	unsigned int crc16_full = (crc16 & 0xFFFF0000) >> 16;
	crc16 &= 0x0000FFFF;

	if (length < 0xFFFFFFFF && // 1000 * 1000
		length > 0 && (length == length_match || length_match == 0) && (crc16_full == crc16_full_match || length_match == 0))
	{
		if (length_match == 0)
			crc16_full_match = crc16_full;

		for (int i = 1; i < recipientsArray.size() - (mainHeader ? 1 : 0); i++) // mainHeader contains cloud wallet address at end
		{
			juce::BigInteger bigInt;
			bigInt.parseString(recipientsArray[i], 10);
			MemoryBlock part = bigInt.toMemoryBlock();
			part.ensureSize(8, true);
			attachmentData.append(part.getData(), 8);
		}

		length_match = jmax<int>(length - attachmentData.getSize(), 0);

		// check crc for this mulitout part
		boost::crc_16_type crcProcessor;
		crcProcessor.process_bytes(attachmentData.getData(), attachmentData.getSize()); // incl any padded zeroes
		unsigned int crc16_check = crcProcessor.checksum();

		if (crc16 != crc16_check)
			attachmentData.reset();
		else if (length_match <= 0)
		{ // end of the data stream, check if we need to cut any padded zeroes
			attachmentData.setSize(length, true); // remove zero padded bytes only at the end, when length is shorter than the data we got
		}
	}
	else attachmentData.reset();

	return attachmentData;
}

// CloudBurst UPLOAD --------------------------------------------------------------------------------------------------------------------------
String BurstExt::BurstJob::CloudUpload()
{
	Array<MTX> uploadedMTX;
	unsigned int dropSize = 1;
	String balanceNQT = GetJSONvalue(getBalance(GetAccountRS()), "balanceNQT");

	// make memoryblock to save. message + file   MESSAGE\0FILENAME\0DATA
	String filename = upload.fileToUpload.getFileName();

	MemoryBlock filenameData;
	MemoryBlock fileData;
	if (upload.fileToUpload.existsAsFile())
	{
		if (upload.fileToUpload.getSize() > 8 * 128 * MAX_FEE_SLOTS)
			return "file is too large!";
		filenameData.append(filename.toUTF8(), filename.length());
		upload.fileToUpload.loadFileAsData(fileData);
	}

/*	MemoryBlock mbIn;
	char zeroByte = 0;
	mbIn.append(upload.message.toUTF8(), upload.message.length());
	mbIn.append(&zeroByte, 1);
	if (fileData.getSize() > 0 && upload.fileToUpload.existsAsFile())
	{
		mbIn.append(filenameData.getData(), filenameData.getSize());
		mbIn.append(&zeroByte, 1);
		mbIn.append(fileData.getData(), fileData.getSize());
	}*/
	
	String uploadFinishMsg;
	// result is an array with multiple numerical addresses
	int64 planks = CloudCalcCosts();
	bool fullFail = false;
	if (planks < balanceNQT.getLargeIntValue())
	{
		String result;
		// all transactions
		int transactionNumber = 0;
		while (transactionNumber < upload.allAddresses.size() && !fullFail && !shouldExit())
		{
			if (upload.allAddresses[transactionNumber].size() > 0)
			{	// keep trying until 1 transaction doesnt fail
				int failed = 1;
				while (failed > 0 && failed < 32 && !shouldExit())
				{
					// Send the transaction
					if (transactionNumber == 0)
					{
						StringArray amountsNQT;
						for (int i = 0; i < upload.allAddresses[transactionNumber].size() - 1; i++)
							amountsNQT.add("1");
						amountsNQT.add(String(upload.feePlancks));

						result = sendMoneyMulti(
							upload.allAddresses[transactionNumber],
							amountsNQT,
							String(upload.fee + (FEE_QUANT * (transactionNumber % upload.stackSize))),
							String(upload.deadline));
					}
					else
					{
						result = sendMoneyMultiSame(
							upload.allAddresses[transactionNumber],
							String(dropSize),
							String(upload.fee + (FEE_QUANT * (transactionNumber % upload.stackSize))),
							String(upload.deadline));
					}

					String transactionID = GetJSONvalue(result, "transaction"); // the ID of the newly created transaction
					if (transactionID.isEmpty())
					{ //ToLog("FAILED " + String(failed) + " to send: " + (allAddresses[transactionNumber].joinIntoString(",")));
						failed++;
						Time::waitForMillisecondCounter(Time::getApproximateMillisecondCounter() + 100);
					}
					else
					{ //ToLog("send tx " + String(transactionNumber) + " id:" + transactionID);
						// store a copy of the transaction
						MTX mtx;
						mtx.id = transactionID;
						mtx.addresses = upload.allAddresses[transactionNumber];
						mtx.amount = String(dropSize);
						mtx.fee = String(upload.fee + (FEE_QUANT * (transactionNumber % upload.stackSize)));
						mtx.dl = String(upload.deadline * 60);
						uploadedMTX.add(mtx);

						failed = 0;
						Time::waitForMillisecondCounter(Time::getApproximateMillisecondCounter() + 50);
					}
				}
				if (failed >= 32)
					fullFail = true;
			}

			transactionNumber++;
			Time::waitForMillisecondCounter(Time::getApproximateMillisecondCounter() + 100);
		}

		if (shouldExit())
			uploadFinishMsg = ("Transaction cancelled !");
		else if (fullFail)
			uploadFinishMsg = ("Transaction failed !");
		else
		{
			String txid = uploadedMTX.getLast().id;
			String txidRS = PREFIX_CB_RS "-" + GetJSONvalue(rsConvert(txid), "accountRS").fromFirstOccurrenceOf("BURST-", false, true);
			
			return txidRS;
		}
	}
	else
	{
		if (balanceNQT.getLargeIntValue() <= 0)
			uploadFinishMsg = ("Wallet balance for " + GetAccountRS() + " is zero !\nPlease check your connection and your passphrase for any errors.");
		else
			uploadFinishMsg = ("Wallet balance is too low !");
	}

	return uploadFinishMsg;
}

int64 BurstExt::BurstJob::CloudCalcCosts()//String message, File fileToUpload, uint64 stackSize, uint64 fee, Array<StringArray> &allAddresses, int64 &addressesNum, uint64 &txFeePlancks, uint64 &feePlancks, uint64 &burnPlancks)
{
	unsigned int dropSize = 1;
	// make memoryblock to save. message + file   MESSAGE\0FILENAME\0DATA
	String filename = upload.fileToUpload.getFileName();

	MemoryBlock filenameData;
	MemoryBlock fileData;
	if (upload.fileToUpload.existsAsFile())
	{
		filenameData.append(filename.toUTF8(), filename.length());
		upload.fileToUpload.loadFileAsData(fileData);
	}

	MemoryBlock mbIn;
	char zeroByte = 0;
	mbIn.append(upload.message.toUTF8(), upload.message.length());
	mbIn.append(&zeroByte, 1);
	if (fileData.getSize() > 0 && upload.fileToUpload.existsAsFile())
	{
		mbIn.append(filenameData.getData(), filenameData.getSize());
		mbIn.append(&zeroByte, 1);
		mbIn.append(fileData.getData(), fileData.getSize());
	}

	upload.allAddresses = SetAttachmentData(mbIn);
	upload.txFeePlancks = 0;
	upload.feePlancks = 0;
	upload.burnPlancks = 0;
	for (int i = 0; i < upload.allAddresses.size(); i++)
	{
		upload.burnPlancks += dropSize * upload.allAddresses[i].size();
		upload.txFeePlancks += (upload.fee + (FEE_QUANT * (i % upload.stackSize)));
	}

	upload.burnPlancks -= (upload.burnPlancks > 0 ? 1 : 0); // we added one for the fee address
	upload.feePlancks = (upload.txFeePlancks / 100); // 1
	upload.confirmTime = uint64 (((upload.allAddresses.size() / (float)upload.stackSize) + 1) * (4 * 60));

	// The slot-based transaction fee system https://burstwiki.org/wiki/Slot-Based_Transaction_Fees
	return upload.txFeePlancks + upload.burnPlancks + upload.feePlancks;
}

// 64 bits per address, 0.00000001 BURST burned
// HEADER = datasize 32 bits + crc 16 + crcfull 16
// PAYLOAD multiple of 8 bytes
Array<StringArray> BurstExt::BurstJob::SetAttachmentData(MemoryBlock data)
{ // convert arbitraty data to numerical burst addresses
	Array<StringArray> addressList;
	boost::crc_16_type crcProcessor;
	crcProcessor.process_bytes(data.getData(), data.getSize());
	int crc16_full = crcProcessor.checksum();

	// compress the data
	MemoryOutputStream destStream(0);
	juce::GZIPCompressorOutputStream zipper(&destStream, 9, false, juce::GZIPCompressorOutputStream::windowBitsGZIP);
	zipper.write(data.getData(), data.getSize());
	zipper.flush();
	data = destStream.getMemoryBlock();

	// zero padding
	juce::MemoryBlock paddedData = data;
	unsigned int original_len = paddedData.getSize();

	if (original_len % 8 > 0) // resize to multiple of 8 bytes
		paddedData.ensureSize(original_len + (8 - (original_len % 8)), true);
	uint64 header = 0;
	char *headerPtr = (char*)&header;

	Array<Array<uint64>> array64;
	Array<uint64> sa;
	unsigned int remaining_len = original_len;
	MemoryBlock partCRC;

	// first is always only 64 addresses. muli out
	// 1 header
	// payload
	// donation address
	for (unsigned int i = 0; i < paddedData.getSize(); i += 8)
	{
		// PAYLOAD
		uint64 address = 0;
		paddedData.copyTo(&address, i, sizeof(uint64));

		bool alreadyHasAddress = sa.contains(address);
		if (!alreadyHasAddress)
		{
			sa.add(address);
			partCRC.append(&address, sizeof(uint64));
		}

		const int s1 = array64.size(); // num multi outs
		const int s2 = sa.size(); // num addresses

		if (alreadyHasAddress ||
			(s1 != 0 && s2 >= (128 - 1)) ||	// multi out same
			(s1 == 0 && s2 >= (64 - 2)) ||	// multi out - fee
			(i + 8) == paddedData.getSize())
		{
			// HEADER
			// new multi out header for each 128 rows
			boost::crc_16_type crcProcessor;
			crcProcessor.process_bytes(partCRC.getData(), partCRC.getSize());
			int crc16 = crcProcessor.checksum();
			partCRC.reset();
			headerPtr[0] = (char)(remaining_len & 0x000000FF); // <- repurpose?
			headerPtr[1] = (char)((remaining_len & 0x0000FF00) >> 8);
			headerPtr[2] = (char)((remaining_len & 0x00FF0000) >> 16);
			headerPtr[3] = (char)((remaining_len & 0xFF000000) >> 24);	// 32 bit to store the length, might wanna repurpose the first byte. then max size would be 16777215 bytes (16.7 MB)
			headerPtr[4] = (char)(crc16 & 0x000000FF);
			headerPtr[5] = (char)((crc16 & 0x0000FF00) >> 8);
			headerPtr[6] = (char)(crc16_full & 0x000000FF);
			headerPtr[7] = (char)((crc16_full & 0x0000FF00) >> 8);

			remaining_len -= jmin<int>(sa.size() * 8, remaining_len); // subtract the bytes. which gives us an ID for searching the next multiout
			sa.insert(0, header);

			if (array64.size() == 0)
			{ // add the fee at the end of the first batch
				// If you change this or any other fee related code. then please consider buying me a beer : BURST-72X9-E6F3-YSM2-CLQUD
				sa.add(11955599475299484583);
			}

			// add 1 full new multi out with unique addresses
			array64.add(sa);

			sa.clear();
			if (alreadyHasAddress)
			{
				sa.add(address);
				partCRC.append(&address, sizeof(uint64));
			}
		}
	}

	// convert to array of multiouts with each a string arrays of addresses
	for (int mtx = 0; mtx < array64.size(); mtx++)
	{
		StringArray addresses;
		for (int address = 0; address < array64[mtx].size(); address++)
		{
			addresses.add(String(array64[mtx][address]));
		}
		addressList.add(addresses);
	}

	// test addresses for unpacking
	MemoryBlock attachmentData;
	int length = 0;
	unsigned int crc16_expected = 0;
	for (int i = 0; i < addressList.size(); i++)
	{
		MemoryBlock mem = GetAttachmentData(addressList[i], length, crc16_expected, i == 0 && addressList[i].size() <= 64);
		attachmentData.append(mem.getData(), mem.getSize());
	}	
	MemoryInputStream srcStream(attachmentData, false);
	juce::MemoryBlock unzippedData;
	GZIPDecompressorInputStream dezipper(&srcStream, false, GZIPDecompressorInputStream::gzipFormat);
	dezipper.readIntoMemoryBlock(unzippedData);	// unzip
	boost::crc_16_type crc16Processor;
	crc16Processor.process_bytes(unzippedData.getData(), unzippedData.getSize());
	unsigned int crc16_full_check = crc16Processor.checksum();
	if (crc16_expected != crc16_full_check)// check the 16 bits crc of the unzipped data
	{ // error we cannot reverse the data from the addresses !
		addressList.clear();
	}

	return addressList;
}

// COUPONS --------------------------------------------------------------------------------------------------
String BurstExt::CreateCoupon(String txSignedHex, String password)
{
	MemoryBlock	pwBin(password.toRawUTF8(), password.getNumBytesAsUTF8());
	String shaPwHex = SHA256(pwBin).toHexString().removeCharacters(" ");
	BLOWFISH bf(shaPwHex.toStdString());

	int retry = 10;
	String base64coupon;
	while (txSignedHex.length() >= (176 * 2) && base64coupon.isEmpty() && --retry > 0)
	{
		MemoryBlock txSigned;
		txSigned.loadFromHexString(txSignedHex);
				
		int length = 64;
		int newlength = 0; // 80
		byte *encBytes = bf.Encrypt_CBC(&((byte *)txSigned.getData())[96], length, &newlength);// only encrypt the 64 signature bytes. 96 to 160

		// cut out signature
		MemoryBlock txSignedEncrypted(&((char *)txSigned.getData())[0], 96); // pre sign
		txSignedEncrypted.append(&((char *)txSigned.getData())[160], txSigned.getSize() - 160); // post sign
		// add encrypted signature at back
		txSignedEncrypted.append(encBytes, newlength);

		base64coupon = toBase64Encoding(txSignedEncrypted);

	//	bool plainTestInvalid = true; // should fail
	//	String plainTest = ValidateCoupon(base64coupon, "WRONG_PASSWORD", plainTestInvalid);

		bool valid = false;
		String details = ValidateCoupon(base64coupon, password, valid);

		if (!valid) // erase if code is not valid
		{
			Time::waitForMillisecondCounter(Time::getApproximateMillisecondCounter() + 500);
			base64coupon.clear();
		}

		delete[] encBytes;
	}
	return base64coupon;
}

String BurstExt::RedeemCoupon(String couponCode, String password)
{
	String txHex = DecryptCoupon(couponCode, password);
	if (txHex.length() >= 176 * 2 && txHex.containsOnly("ABCDEFabcdef0123456789"))
	{
		var result = broadcastTransaction(txHex);
		String transactionID = result["transaction"];
		return transactionID;
	}
	return String::empty;
}

String BurstExt::ValidateCoupon(String couponCode, String password, bool &valid)
{
	String txHex = DecryptCoupon(couponCode, password, true);
	if (txHex.length() >= 176 * 2 && txHex.containsOnly("ABCDEFabcdef0123456789"))
	{
		String txDetail = parseTransaction(txHex, "");
		String verify = GetJSONvalue(txDetail, "verify");
		valid = verify.getIntValue() > 0 || verify.compareIgnoreCase("true") == 0;
		return txDetail;
	}
	return String::empty;
}

String BurstExt::DecryptCoupon(String couponCode, String password, bool validate)
{
	String txHex;
	MemoryBlock	pwBin(password.toRawUTF8(), password.getNumBytesAsUTF8());
	String shaPwHex = SHA256(pwBin).toHexString().removeCharacters(" ");
	BLOWFISH bf(shaPwHex.toStdString());

	MemoryBlock	couponCodeBin = fromBase64Encoding(couponCode.toStdString());
	MemoryBlock	txBin(couponCodeBin.getData(), couponCodeBin.getSize() - 80);
	if (couponCodeBin.getSize() >= 176)
	{
		int length = 80;// couponCodeBin.getSize() - (176 - 64);
		int newlength = 0;
		byte *decBytes = bf.Decrypt_CBC(&((byte *)couponCodeBin.getData())[couponCodeBin.getSize() - 80], length, &newlength);

		if (newlength >= 64) // size of signature
		{
			txBin.insert(decBytes, 64, 96); // insert decrypted signature
			txHex = String::toHexString(txBin.getData(), txBin.getSize(), 0);
		}
		else if (!validate)
		{ // for data validation w/o sign
			MemoryBlock emptySign(64, true);
			txBin.insert(emptySign.getData(), 64, 96);
			txHex = String::toHexString(txBin.getData(), txBin.getSize(), 0);
		}
		delete[] decBytes;
	}
	return txHex;
}

//------------------------------------------------------------------------------------------------
String BurstExt::broadcastTransaction(String signedTransactionBytesStrHex)
{
	NodeHop(false);
	return BurstKit::broadcastTransaction(signedTransactionBytesStrHex);
}

String BurstExt::GetUrlStr(const String url)
{
	NodeHop(false);
	return BurstKit::GetUrlStr(url);
}

void BurstExt::EnableNodeHop(unsigned int maxHopNodeRequests)
{
	this->maxHopNodeRequests = maxHopNodeRequests;
}

unsigned int BurstExt::GetNodeCount()
{
	return api_peers_whitelist.size();
}

String BurstExt::NodeHop(const bool force)
{
	if (maxHopNodeRequests > 0 || force)
	{
		int port = 8125;
		String hopNode;
		if (nodeHopCount >= maxHopNodeRequests)
		{
			nodeHopCount = 0;
			
			String root_peers = URL(GetNode() + "burst?requestType=getPeers&state=CONNECTED").readEntireTextStream();
			var root_peersJson;
			Result r = JSON::parse(root_peers, root_peersJson);

			String hostStripped = GetNode();
			hostStripped = hostStripped.substring(0, hostStripped.length() - 1);

			if (hostStripped.getTrailingIntValue() == 6876) // check if testnet
				port = 6876;
			
			// if it has an response with peers, pick a random peer and check if it is API accessible
			if (root_peersJson["peers"].isArray())
			{
				Array<int> jumble;
				for (int i = 0; i < root_peersJson["peers"].size(); i++)
				{ // randomize the read index
					Random random;
					int index = random.nextInt(jumble.size() + 1);
					jumble.insert(index, i);
				}
				for (int i = 0; i < jumble.size() && hopNode.isEmpty(); i++)
				{
					String peer = root_peersJson["peers"][jumble[i]].toString() + ":" + String(port);
					if (api_peers_whitelist.contains(peer) == false &&
						api_peers_blacklist.contains(peer) == false)
					{
						String url((GetForceSSL_TSL() ? "https://" : "http://") + peer + "/burst?requestType=getPeers&state=CONNECTED");
						int64 startTimeMs = Time::currentTimeMillis();
						String peers = URL(url).readEntireTextStream();
						if (Time::currentTimeMillis() - startTimeMs < 100) // at least 0.1 sec response time
						{
							var peersJson;
							Result r = JSON::parse(peers, peersJson);
							if (peersJson["peers"].isArray() && 
								peersJson["peers"].size() > 2)
							{ // make sure its not a dead end and has more peers
								api_peers_whitelist.add(peer);
								hopNode = peer;
							}
							else api_peers_blacklist.add(peer);
						}
						else api_peers_blacklist.add(peer);
					}
				}
				if (hopNode.isEmpty() && api_peers_whitelist.size() > 0)
				{ // no peer found. get one from our api_peers_whitelist
					Random random;
					int index = random.nextInt(api_peers_whitelist.size() + 1);
					hopNode = api_peers_whitelist[index];
				}
			}
		}
		else nodeHopCount++;

		if (hopNode.isNotEmpty())
		{
			String r((GetForceSSL_TSL() ? "https://" : "http://") + hopNode + "/");
			SetNode(r);
			return r;
		}
	}
	return String::empty;
}