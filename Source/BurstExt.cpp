#include "BurstExt.h"

#include <boost/crc.hpp>
#include "crypto/blowfish.h"

BurstExt::BurstExt(String hostUrl) : BurstKit(hostUrl)
{
}

BurstExt::~BurstExt()
{
}

// CloudBurst DOWNLOAD ------------------------------------------------------------------------------------------------------------------------
bool CheckThreadShouldExit(const void* context, float /*progress*/)
{
	if (context)
	{
	//	((BurstExt*)context)->setProgress(jmin<float>(jmax<float>(progress, 0.f), 1.f));
	//	return ((BurstExt*)context)->threadShouldExit();
	}
	return false;
}

bool BurstExt::CloudDownload(String cloudID, String dlFolder, String &dlFilename, MemoryBlock &dlData, String &msg)
{
	Array<MemoryBlock> memoryBLocks;
	Array<String> timecodes;
	timecodes.clear();

	ScrapeAccountTransactions(memoryBLocks, timecodes, cloudID, &CheckThreadShouldExit, this);

	msg.clear();
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
			dlFilename = filename;
			dlData = mem;

			if (File(dlFolder).exists() && File(dlFolder).getChildFile(dlFilename).existsAsFile() == false && dlFilename.isNotEmpty())
				File(dlFolder).getChildFile(dlFilename).appendData(dlData.getData(), dlData.getSize());

			downloadOk = true;
		}
		else if (step <= 1)
			downloadOk = true;

		if (message.isNotEmpty())
		{
			if (timecodes[i].isNotEmpty())
			{ // 2014-08-11 04:00:00 genesis epoch
				int64 tc = 1407722400 + timecodes[i].getLargeIntValue();
				msg += Time(tc * 1000).toString(true, true) + " ";
			}
			msg += message;
		}
	}
	return downloadOk;
}

bool BurstExt::ScrapeAccountTransactions(Array<MemoryBlock> &attachmentDatas, Array<String> &timecodes, String downloadTransactionID, CheckThreadShouldExit_ptr CheckThreadShouldExit, const void* context)
{
	downloadTransactionID = downloadTransactionID.toUpperCase();
	if (downloadTransactionID.containsAnyOf("ABCDEFGHIJKLMNOPQRSTUVWXYZ")) // a reed solomon or numerical?
	{
		String converted(downloadTransactionID.replace(PREFIX_CB_RS "-", ""));
		if (converted.startsWith("BURST-") == false)
			converted = "BURST-" + converted;
		downloadTransactionID = GetJSONvalue(rsConvert(converted), "account");
	}
	float progress = 0.f;

	//bool cancel = CheckThreadShouldExit(context, progress);

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

	StringArray txidArray;
	var recipientsArray;
	StringArray timeStamps;
	progress += 0.01f;
	if (reedSolomonIn.isNotEmpty() && !CheckThreadShouldExit(context, progress))
	{
		// get all transactions from this account
		String timestamp = String::empty; // is the earliest block(in seconds since the genesis block) to retrieve(optional)
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
		while ((!stopLookingPost || !stopLookingPre) && !CheckThreadShouldExit(context, progress))
		{
			progress = (idxPre + idxPost) / numProgress;
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
							bigInt.parseString(v[0][0].toString(), 10);
							MemoryBlock header = bigInt.toMemoryBlock();

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
	}

	float progressPart2 = (1.f - progress) / 2.f;
	float progressPart3 = (1.f - progress) / 2.f;

	// sort and get data from recipients. reconstruct the data
	// we only collect 1 payload here but multiple payloads with different CRCs will be splitted also
	Array<unsigned int> attachmentCRCs;
	if (recipientsArray.isArray() && !CheckThreadShouldExit(context, progress))
	{
		int totalMultiOuts = recipientsArray.getArray()->size();
		Array<int> totalReadData;
		for (int i = 0; i < totalMultiOuts && !CheckThreadShouldExit(context, progress + ((progressPart2 / totalMultiOuts) * i)); i++)
		{
			int dataSize = 0;
			unsigned int crc16_full_match = 0;
			MemoryBlock data = GetAttachmentData(recipientsArray[i], dataSize, crc16_full_match);

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

		for (int i = attachmentDatas.size() - 1; i >= 0 && !CheckThreadShouldExit(context, progress + progressPart2 + ((progressPart3 / attachmentDatas.size()) * ((attachmentDatas.size() - 1) - i))); i--)
		{ // check 16 bit crc for combined data. and if expected data size matches
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
	if (CheckThreadShouldExit(context, 1.f))
	{
		attachmentDatas.clear();
		timecodes.clear();
	}
	return !CheckThreadShouldExit(context, 1.f);
}

MemoryBlock BurstExt::GetAttachmentData(var jsonStructure, int &length_match, unsigned int &crc16_full_match) // length_match to 0 get data without sequence check
{ // convert the numerical burst addresses from a multi out to arbitrary data
	MemoryBlock attachmentData;
	var recipientsVAR = jsonStructure;
	if (recipientsVAR.isArray())
	{
		if (recipientsVAR.size() > 0)
		{
			juce::BigInteger bigInt; // TODO drop the use of BigInteger. but String only returns signed version of int 64 (max size should be 2^64)
			bigInt.parseString(recipientsVAR[0].toString(), 10);
			MemoryBlock header = bigInt.toMemoryBlock();

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

				for (int i = 1; i < recipientsVAR.size(); i++)
				{
					juce::BigInteger bigInt;
					bigInt.parseString(recipientsVAR[i].toString(), 10);
					MemoryBlock part = bigInt.toMemoryBlock();
					part.ensureSize(8, true);
					attachmentData.append(part.getData(), 8);
				}

				length_match = jmax<int>(length - attachmentData.getSize(), 0);

				// check crc for this mulitout part
				boost::crc_16_type crcProcessor;
				crcProcessor.process_bytes(attachmentData.getData(), attachmentData.getSize()); // incl any padded zeroes
				unsigned int crc16_check = crcProcessor.checksum();

				if ((crc16) != crc16_check)
					attachmentData.reset();
				else if (length_match <= 0)
				{ // end of the data stream, check if we need to cut any padded zeroes
					attachmentData.setSize(length, true); // remove zero padded bytes only at the end, when length is shorter than the data we got
				}
			}
			else attachmentData.reset();
		}
	}
	return attachmentData;
}

// CloudBurst UPLOAD --------------------------------------------------------------------------------------------------------------------------
String BurstExt::CloudUpload(String message, File fileToUpload, uint64 stackSize, uint64 fee, Array<StringArray> &allAddresses, uint64 deadline, int64 &addressesNum, uint64 &txFeePlancks, uint64 &feePlancks, uint64 &burnPlancks)
{
	Array<MTX> uploadedMTX;
	unsigned int dropSize = 1;
	String balanceNQT = GetJSONvalue(getBalance(GetAccountRS(), String::empty, String::empty, String::empty, String::empty), "balanceNQT");

	// make memoryblock to save. message + file   MESSAGE\0FILENAME\0DATA
	String filename = fileToUpload.getFileName();

	MemoryBlock filenameData;
	MemoryBlock fileData;
	if (fileToUpload.existsAsFile())
	{
		if (fileToUpload.getSize() > 8 * 128 * 1020)
			return "file is too large!";
		filenameData.append(filename.toUTF8(), filename.length());
		fileToUpload.loadFileAsData(fileData);
	}

	MemoryBlock mbIn;
	char zeroByte = 0;
	mbIn.append(message.toUTF8(), message.length());
	mbIn.append(&zeroByte, 1);
	if (fileData.getSize() > 0 && fileToUpload.existsAsFile())
	{
		mbIn.append(filenameData.getData(), filenameData.getSize());
		mbIn.append(&zeroByte, 1);
		mbIn.append(fileData.getData(), fileData.getSize());
	}
	
	String uploadFinishMsg;
	// result is an array with multiple numerical addresses
	int64 planks = CloudCalcCosts(message, fileToUpload, stackSize, fee, allAddresses, addressesNum, txFeePlancks, feePlancks, burnPlancks);
	bool fullFail = false;
	if (planks < balanceNQT.getLargeIntValue())
	{
		String result;

		// all transactions
		int transactionNumber = 0;
		while (transactionNumber < allAddresses.size() && !fullFail) //&& !threadShouldExit())			
		{
			if (allAddresses[transactionNumber].size() > 0)
			{	// keep trying until 1 transaction doesnt fail
				int failed = 1;
				while (failed > 0 && failed < 32) // && !threadShouldExit())
				{
					// Send the transaction
					if (transactionNumber == 0)
					{
						StringArray amountsNQT;
						for (int i = 0; i < allAddresses[transactionNumber].size() - 1; i++)
							amountsNQT.add("1");
						amountsNQT.add(String(feePlancks));

						result = sendMoneyMulti(
							allAddresses[transactionNumber],
							amountsNQT,
							String(fee + (735000 * (transactionNumber % stackSize))),
							String(deadline));
					}
					else
					{
						result = sendMoneyMultiSame(
							allAddresses[transactionNumber],
							String(dropSize),
							String(fee + (735000 * (transactionNumber % stackSize))),
							String(deadline * 60));
					}

					String transactionID = GetJSONvalue(result, "transaction"); // the ID of the newly created transaction
					if (transactionID.isEmpty())
					{
						//ToLog("FAILED " + String(failed) + " to send: " + (allAddresses[transactionNumber].joinIntoString(",")));
						failed++;
						Time::waitForMillisecondCounter(Time::getApproximateMillisecondCounter() + 100);
					}
					else
					{
						//ToLog("send tx " + String(transactionNumber) + " id:" + transactionID);
						// store a copy of the transaction
						MTX mtx;
						mtx.id = transactionID;
						mtx.addresses = allAddresses[transactionNumber];
						mtx.amount = String(dropSize);
						mtx.fee = String(fee + (735000 * (transactionNumber % stackSize)));
						mtx.dl = String(deadline * 60);
						uploadedMTX.add(mtx);

						failed = 0;
						Time::waitForMillisecondCounter(Time::getApproximateMillisecondCounter() + 50);
					}
				}
				if (failed >= 32)
					fullFail = true;
			}

			transactionNumber++;
//			ThreadWithProgressWindow::setProgress((1.f / allAddresses.size()) * transactionNumber);
			Time::waitForMillisecondCounter(Time::getApproximateMillisecondCounter() + 100);
		}

	/*	if (threadShouldExit())
		{
			uploadFinishMsg = ("Transaction cancelled !");
		}
		else*/ 
		if (fullFail)
		{
			uploadFinishMsg = ("Transaction failed !");
		}
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
	//ToLog(uploadFinishMsg);
	/*	TEST UNPACK
	var attachmentVAR;
	for (int i = 0; i < sa.size(); i++)
	{
	var arryayTest;
	for (int j = 0; j < sa[i].size(); j++)
	arryayTest.append(sa[i][j]);
	attachmentVAR.append(arryayTest);
	}
	Array<MemoryBlock> arb = burstAPI.ScrapeAccountTransactions("", attachmentVAR);
	for (int i = 0; i < arb.size(); i++)
	{ // check if the data matches
	if (arb[i].matches(mbIn.getData(), mbIn.getSize()))
	match = true;
	}*/
	return uploadFinishMsg;
}

int64 BurstExt::CloudCalcCosts(String message, File fileToUpload, uint64 stackSize, uint64 fee, Array<StringArray> &allAddresses, int64 &addressesNum, uint64 &txFeePlancks, uint64 &feePlancks, uint64 &burnPlancks)
{
	unsigned int dropSize = 1;
	// make memoryblock to save. message + file   MESSAGE\0FILENAME\0DATA
	String filename = fileToUpload.getFileName();

	MemoryBlock filenameData;
	MemoryBlock fileData;
	if (fileToUpload.existsAsFile())
	{
		filenameData.append(filename.toUTF8(), filename.length());
		fileToUpload.loadFileAsData(fileData);
	}

	MemoryBlock mbIn;
	char zeroByte = 0;
	mbIn.append(message.toUTF8(), message.length());
	mbIn.append(&zeroByte, 1);
	if (fileData.getSize() > 0 && fileToUpload.existsAsFile())
	{
		mbIn.append(filenameData.getData(), filenameData.getSize());
		mbIn.append(&zeroByte, 1);
		mbIn.append(fileData.getData(), fileData.getSize());
	}

	allAddresses = SetAttachmentData(mbIn);

	addressesNum = 0;
	txFeePlancks = 0;
	feePlancks = 0;
	burnPlancks = 0;
	for (int i = 0; i < allAddresses.size(); i++)
	{
		//costsPlanks
		burnPlancks += dropSize * allAddresses[i].size();
		addressesNum += allAddresses[i].size();
		txFeePlancks += (fee + (735000 * (i % stackSize)));
	}

	burnPlancks -= 1; // we added one for the fee address
	feePlancks = (txFeePlancks / 100); // 1

	// The slot-based transaction fee system https://burstwiki.org/wiki/Slot-Based_Transaction_Fees
	return txFeePlancks + burnPlancks + feePlancks;
}

// 64 bits per address, 0.00000001 BURST burned
// HEADER = datasize 32 bits + crc 16 + crcfull 16
// PAYLOAD multiple of 8 bytes
Array<StringArray> BurstExt::SetAttachmentData(MemoryBlock data)
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
			headerPtr[1] = (char)(remaining_len & 0x0000FF00) >> 8;
			headerPtr[2] = (char)(remaining_len & 0x00FF0000) >> 16;
			headerPtr[3] = (char)(remaining_len & 0xFF000000) >> 24;	// 32 bit to store the length, might wanna repurpose the first byte. then max size would be 16777215 bytes (16.7 MB)
			headerPtr[4] = (char)(crc16 & 0x000000FF);
			headerPtr[5] = (char)(crc16 & 0x0000FF00) >> 8;
			headerPtr[6] = (char)(crc16_full & 0x000000FF);
			headerPtr[7] = (char)(crc16_full & 0x0000FF00) >> 8;

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
	return addressList;
}

// COUPONS --------------------------------------------------------------------------------------------------
String BurstExt::CreateCoupon(String txSignedHex, String password)
{
	MemoryBlock	pwBin(password.toRawUTF8(), password.getNumBytesAsUTF8());
	String shaPwHex = SHA256(pwBin).toHexString().removeCharacters(" ");
	BLOWFISH bf(shaPwHex.toStdString());

	int retry = 5;
	String base64coupon;
	while (txSignedHex.isNotEmpty() && base64coupon.isEmpty() && --retry > 0)
	{
		int length = txSignedHex.getNumBytesAsUTF8();
		byte* data = new byte[length];
		memcpy(data, txSignedHex.toRawUTF8(), length);

		int newlength = 0;
		byte *encBytes = bf.Encrypt_CBC(data, length, &newlength);

		base64coupon = toBase64Encoding(encBytes, newlength);

		bool valid = false;
		String details = ValidateCoupon(base64coupon, password, valid);

		if (!valid) // erase if code is not valid
			base64coupon.clear();

		delete[] encBytes;
	}
	return base64coupon;
}

String BurstExt::RedeemCoupon(String couponCode, String password)
{
	String transactionID;

	MemoryBlock	pwBin(password.toRawUTF8(), password.getNumBytesAsUTF8());
	String shaPwHex = SHA256(pwBin).toHexString().removeCharacters(" ");
	BLOWFISH bf(shaPwHex.toStdString());

	MemoryBlock	couponCodeBin = fromBase64EncodingToMB(couponCode.toStdString());
	int length = couponCodeBin.getSize();
	byte* data = new byte[length];
	memcpy(data, couponCodeBin.getData(), length);

	int newlength = 0;
	byte *decBytes = bf.Decrypt_CBC(data, length, &newlength);

	bool invalidChar = false;
	for (int i = 0; i < newlength && !invalidChar; i++)
	{
		invalidChar = invalidChar ? true : decBytes[i] < 48 || (decBytes[i] > 57 && decBytes[i] < 65) || (decBytes[i] > 70 && decBytes[i] < 97) || decBytes[i] > 102;
	}
	if (!invalidChar)
	{
		String coupondataDecypted((const char*)decBytes, newlength);
		//var result = broadcastTransaction(String::toHexString(coupondataDecypted.getData(), coupondataDecypted.getSize()).removeCharacters(" "));
		var result = broadcastTransaction(coupondataDecypted);
		transactionID = result["transaction"];
	}

	delete[] data;
	delete[] decBytes;

	return transactionID;
}

String BurstExt::ValidateCoupon(String couponCode, String password, bool &valid)
{
	MemoryBlock	pwBin(password.toRawUTF8(), password.getNumBytesAsUTF8());
	String shaPwHex = SHA256(pwBin).toHexString().removeCharacters(" ");
	BLOWFISH bf(shaPwHex.toStdString());

	MemoryBlock	couponCodeBin = fromBase64EncodingToMB(couponCode.toStdString());
	int length = couponCodeBin.getSize();
	byte* data = new byte[length];
	memcpy(data, couponCodeBin.getData(), length);

	int newlength = 0;
	byte *decBytes = bf.Decrypt_CBC(data, length, &newlength);

	valid = false;
	bool invalidChar = false;
	for (int i = 0; i < newlength && !invalidChar; i++)
	{ // check the data before we try to make a valid string. only expecting ascii hex-chars
		invalidChar = invalidChar ? true : decBytes[i] < 48 || (decBytes[i] > 57 && decBytes[i] < 65) || (decBytes[i] > 70 && decBytes[i] < 97) || decBytes[i] > 102;
	}
	String txDetail;
	if (!invalidChar)
	{
		String coupondataDecypted((const char*)decBytes, newlength);
		
		if (coupondataDecypted.isNotEmpty() && coupondataDecypted.containsOnly("ABCDEFabcdef0123456789"))
		{
			txDetail = parseTransaction(coupondataDecypted, "");
			String verify = GetJSONvalue(txDetail, "verify");
			valid = verify.getIntValue() > 0 || verify.compareIgnoreCase("true") == 0;
		}
	}

	delete[] data;
	delete[] decBytes;

	return txDetail;
}

