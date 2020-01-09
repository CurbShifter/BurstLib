/*
Copyright (C) 2019  CurbShifter

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

#include "BurstSocket.h"

BurstSocket::BurstSocket(String hostUrl) : BurstKit(hostUrl)
{
}

BurstSocket::~BurstSocket()
{
}

void BurstSocket::SetNode(String hostUrl)
{
	BurstKit::SetNode(hostUrl);
	socketThread.SetNode(hostUrl);
}

void BurstSocket::SetDonateMode(bool on)
{
	socketThread.SetDonateMode(on);
}

void BurstSocket::SetSecretPhrase(String passphrase)
{
	BurstKit::SetSecretPhrase(passphrase);
	socketThread.SetSecretPhrase(passphrase);
}

void BurstSocket::SetForceSSL_TSL(const bool forceSSLOn)
{
	BurstKit::SetForceSSL_TSL(forceSSLOn);
	socketThread.SetForceSSL_TSL(forceSSLOn);
}

String BurstSocket::GetLastError(int &errorCode)
{	
	return BurstKit::GetLastError(errorCode);
}

void BurstSocket::SetError(int errorCode, String msg)
{	
	BurstKit::SetError(errorCode, msg);
}

String BurstSocket::CreateAsset()
{ // run once
#ifdef _DEBUG
	String name(SOCKET_ASSET_NAME);
	String description(SOCKET_ASSET_DESCRIPTION);
	uint64 quantityQNT(SOCKET_ASSET_TOTAL_QUANTITY * std::pow(10L, SOCKET_ASSET_DECIMALS));
	uint64 decimals(SOCKET_ASSET_DECIMALS);
	String result = issueAsset(name, description, String(quantityQNT), String(decimals), "100000000000", "1440", true, 0);
	String transactionID = GetJSONvalue(result, "transaction");
	if (transactionID.isNotEmpty())
		return transactionID;
	else
	{
		String errorDescription = GetJSONvalue(result, "errorDescription");
		return errorDescription;
	}
#endif
}

String BurstSocket::FillOrderBook(uint64 step)
{ // run once
	String r;
#ifdef _DEBUG
	//for ( = 0; step < SOCKET_ASSET_ORDERBOOK_MAX_STEPS; step++)
	{
		String askQuantityQNT = String((int64)(((uint64)SOCKET_ASSET_ORDERBOOK_STEP_QUANTITY_MULTIPLIER * (step + 1)) * std::pow(10L, SOCKET_ASSET_DECIMALS)));
		String askPriceNQT = String((int64)(SOCKET_ASSET_ORDERBOOK_START_NQT + (step * SOCKET_ASSET_ORDERBOOK_STEP_NQT)) / std::pow(10L, 8));

		String result = placeAskOrder(IsOnTestNet() ? SOCKET_ASSET_ID_TESTNET : SOCKET_ASSET_ID_MAINNET, askQuantityQNT, askPriceNQT, "normal", "1440", String::empty, false, true);

		if (result.isNotEmpty())
		{
			String txId = GetJSONvalue(result, "transaction");
			r = txId;
			if (txId.isEmpty())
			{
				String errorCode = GetJSONvalue(result, "errorCode");
				String error = GetJSONvalue(result, "error");
				String errorDescription = GetJSONvalue(result, "errorDescription");

				r = errorCode + " " + error + " " + errorDescription;
			}
		}
	}
#endif
	return r;
}

void BurstSocket::OpenSocket()
{
	CloseSocket();
	SetBlock();
	socketThread.startThread();
}

void BurstSocket::CloseSocket()
{
	socketThread.stopThread(10 * 1000);
}

void BurstSocket::SetForceBlock(const bool force)
{
	socketThread.SetForceBlock(force);
}

void BurstSocket::SetPause(const bool pause)
{
	socketThread.SetPause(pause);
}

bool BurstSocket::SetBlock(const int maxSlotHeight, const int slotHeight)
{
	socketThread.SetMaxSlotHeight(maxSlotHeight);
	socketThread.SetBlockingLength(slotHeight);
	return true;
}

void BurstSocket::SetUseCustody(const bool useCustodyMode)
{
	socketThread.SetUseCustody(useCustodyMode);
}

bool BurstSocket::GetUseCustody()
{
	return socketThread.GetUseCustody();
}

void BurstSocket::GetStatus(bool &blocking, int &slot, float &percent, uint64 &currActiveIn, uint64 &maxActiveIn)
{
	socketThread.GetStatus(blocking, slot, percent, currActiveIn, maxActiveIn);
}

int BurstSocket::GetMaxSlotHeight()
{
	return socketThread.GetMaxSlotHeight();
}

void BurstSocket::SetMaxSlotHeight(const int slotHeight)
{
	return socketThread.SetMaxSlotHeight(slotHeight);
}

int BurstSocket::GetBlockingLength()
{
	return socketThread.GetBlockingLength();
}

void BurstSocket::SetBlockingLength(const int length)
{
	return socketThread.SetBlockingLength(length);
}

int BurstSocket::GetHoldSize()
{
	return socketThread.GetHoldSize();
}

void BurstSocket::SetHoldSize(const int newHoldSize)
{
	return socketThread.SetHoldSize(newHoldSize);
}

bool BurstSocket::Listen(const bool on, String port)
{
	socketThread.Listen(on, port);
	return true;
}

StringArray BurstSocket::GetListenPorts()
{
	return socketThread.GetListenPorts();	
}

bool BurstSocket::Ignore(const bool on, String port)
{
	socketThread.Ignore(on, port);
	return true;
}

StringArray BurstSocket::GetIgnorePorts()
{
	return socketThread.GetIgnorePorts();
}

void BurstSocket::SocketSendMessage(const String address, const String txt, const bool encrypted)
{
	BurstSocket::BurstSocketThread::txPacketOut p = BurstSocket::BurstSocketThread::initPacketOut();
	p.version = GetBurstKitVersionNumber();
	p.isText = true;
	p.encrypted = encrypted;

	String aid = BurstKit::convertToAccountID(address);// convert port ID or RS to uint64
	GetUINT64(aid);
	p.recipient = GetUINT64(aid);
		
	txt.copyToUTF8(&(p.message[0]), MAX_TXDATASIZE);
	p.messageSize = txt.getNumBytesAsUTF8();
	
	socketThread.getFifoOut().addToFifo(&p, 1);
}

bool BurstSocket::GetHeader(MemoryBlock header, char &type, uint32 &offset, uint32 &part_size, uint32 &total_size, MemoryBlock &hash, String &name, MemoryBlock &rawData)
{
	int minimumSize = sizeof(char) + (sizeof(uint32) * 3) + (256/8) + 1;
	if (header.getSize() < minimumSize)
		return false;
	int headerOffset = 0;
	char *data = (char *)header.getData();
	type = data[headerOffset];
	headerOffset += sizeof(char);
	offset = *((uint32*)(&data[headerOffset]));
	headerOffset += sizeof(uint32);
	part_size = *((uint32*)(&data[headerOffset]));
	headerOffset += sizeof(uint32);
	total_size = *((uint32*)(&data[headerOffset]));
	headerOffset += sizeof(uint32);
	hash.ensureSize(256 / 8);
	hash.copyFrom(&(data[headerOffset]), 0, 256 / 8);
	headerOffset += 256 / 8;

	int nameEndIdx = headerOffset;
	while (data[nameEndIdx] != 0 && nameEndIdx < header.getSize())
		nameEndIdx++;

	if (nameEndIdx < header.getSize()) // expect at leat 1 byte of raw data
	{
		if ((nameEndIdx - headerOffset) > 0)
			name = MemoryBlock(&(data[headerOffset]), nameEndIdx - headerOffset).toString();
		else name = String::empty;
		
		if (nameEndIdx + 1 < header.getSize())
			rawData = MemoryBlock(&(data[nameEndIdx + 1]), header.getSize() - (nameEndIdx + 1));
		else rawData.reset();

		return true;
	}
	return false;
}

MemoryBlock BurstSocket::MakeHeader(char type, uint32 offset, uint32 part_size, uint32 total_size, MemoryBlock hash, String name)
{
	MemoryBlock header;
	int zero = 0;
	header.append(&type, sizeof(char));
	header.append(&offset, sizeof(uint32));
	header.append(&part_size, sizeof(uint32));
	header.append(&total_size, sizeof(uint32));
	header.append(hash.getData(), jmin<int>(256 / 8, hash.getSize()));
	header.append(name.toUTF8(), name.getNumBytesAsUTF8());
	header.append(&zero, 1);
	return header;
}

void BurstSocket::SocketStreamData(const String address, const char type, const String nameOfFileOrStream, const void *data, const int bytes, const bool encrypted)
{
	BurstSocket::BurstSocketThread::txPacketOut p = BurstSocket::BurstSocketThread::initPacketOut();
	p.version = GetBurstKitVersionNumber();
	p.isText = false;
	p.encrypted = encrypted;
	
	// convert port ID or RS to uint64
	String aid = BurstKit::convertToAccountID(address);
	GetUINT64(aid);
	p.recipient = GetUINT64(aid);
	
	// compress the data
	MemoryBlock dataMb(data, bytes);
	MemoryOutputStream destStream(0);
	juce::GZIPCompressorOutputStream zipper(&destStream, 9, false, juce::GZIPCompressorOutputStream::windowBitsGZIP);
	zipper.write(dataMb.getData(), dataMb.getSize());
	zipper.flush();
	MemoryBlock dataMbZip = destStream.getMemoryBlock();

	// get the hash
	juce::SHA256 shaHash(dataMbZip);
	MemoryBlock shaHashMb = shaHash.getRawData();

	uint64 offset = 0;
	const uint32 sizeZipped = dataMbZip.getSize();
	while (offset < sizeZipped)
	{
		int minimumSize = sizeof(char) + (sizeof(uint32) * 3) + (256 / 8) + 1;
		uint32 headersize = minimumSize + nameOfFileOrStream.length();
		uint32 availableSpace = (1000 - 100) - headersize; // -100 because double compression. (whole stream and default per packet). cause to bloat the packet with window flags and encryption after
		uint32 part_size = jmin<int>(availableSpace, sizeZipped - offset);
		
		MemoryBlock header = MakeHeader(type, offset, part_size, sizeZipped, shaHashMb, nameOfFileOrStream);// type: 0 stream, 1 file
		p.messageSize = juce::jmin<int>(header.getSize() + part_size, MAX_TXDATASIZE);
		memcpy(&(p.message[0]), header.getData(), header.getSize());
		memcpy(&(p.message[header.getSize()]), &(((char*)dataMbZip.getData())[offset]), p.messageSize - header.getSize());
		socketThread.getFifoOut().addToFifo(&p, 1);

		offset += part_size;
	}
}

void BurstSocket::SocketSendFile(const String address, const File f, const bool encrypted)
{
	MemoryBlock dataMb;
	if (f.loadFileAsData(dataMb))
	{
		void *data = dataMb.getData();
		const int bytes = dataMb.getSize();
		char type = 1;
		SocketStreamData(address, type, f.getFileName(), data, bytes, encrypted);
	}
}

bool BurstSocket::SocketPollMessage(BurstSocket::BurstSocketThread::txPacketIn &p)
{
	// check plain messages
	if (socketThread.getFifoInText().readFromFifo(&p, 1) > 0)
		return true;
	// if no plain messages, check for finished data streams
	BurstSocket::BurstSocketThread::txPacketIn txd = BurstSocket::BurstSocketThread::initPacketIn();
	while (SocketPollData(txd))
	{
		char type = 0;
		uint32 offset = 0;
		uint32 part_size = 0;
		uint32 total_size = 0;
		MemoryBlock hash;
		String name;
		MemoryBlock rawData;

		if (ProcessDataTx(txd, type, offset, part_size, total_size, hash, name, rawData))
		{ // add a message if there is a finished file stream
			String msg(name + " [" + String::toHexString(hash.getData(), hash.getSize(), 0) + "]");
			msg.copyToUTF8(&(txd.message[0]), MAX_TXDATASIZE);
			txd.messageSize = msg.length();
			txd.isText = false;
			p = txd;
			
			return true;
		}
	}
	return false;
}

bool BurstSocket::SocketPollData(BurstSocket::BurstSocketThread::txPacketIn &p)
{
	if (socketThread.getFifoInData().readFromFifo(&p, 1) > 0)
		return true;
	return false;
}

bool BurstSocket::ProcessDataTx(BurstSocket::BurstSocketThread::txPacketIn &tx, char &type, uint32 &offset, uint32 &part_size, uint32 &total_size, MemoryBlock &hash, String &name, MemoryBlock &rawData)
{
	MemoryBlock msgData(&(tx.message[0]), tx.messageSize);
	if (GetHeader(msgData, type, offset, part_size, total_size, hash, name, rawData))
	{
		MemoryBlock data = socketDatas[hash]; // get the previous data
		data.ensureSize(total_size, true); // and set the new data
		data.copyFrom(rawData.getData(), offset, part_size);

		BurstSocket::BurstSocketThread::txPacketIn txCache = socketDataTx[hash];
		if (tx.sender == txCache.sender || txCache.sender == 0)
		{ // keep a cache of collected stream data
			AddDataStreamCache(hash, type, tx, name, data);

			juce::SHA256 shaTest(data);
			MemoryBlock realSha = shaTest.getRawData();
			if (hash.matches(realSha.getData(), realSha.getSize()))
			{
				MemoryInputStream srcStream(data, false); // unzip
				juce::MemoryBlock mb;
				GZIPDecompressorInputStream dezipper(&srcStream, false, GZIPDecompressorInputStream::gzipFormat);
				dezipper.readIntoMemoryBlock(mb);
				socketDatas.set(hash, mb);

				return true; // returns true if we have a finished data stream
			}
		}
	}
	return false;
}

void BurstSocket::GetDataStreamPacket(MemoryBlock hash, char &type, BurstSocket::BurstSocketThread::txPacketIn &tx, String &name, MemoryBlock &data)
{
	type = socketDataType[hash];
	tx = socketDataTx[hash];
	name = socketDataNames[hash];
	data = socketDatas[hash];
}

void BurstSocket::AddDataStreamCache(MemoryBlock hash, const char type, const BurstSocket::BurstSocketThread::txPacketIn tx, const String name, const MemoryBlock data)
{
	socketDataTimeline.add(hash);
	socketDataType.set(hash, type);
	socketDataTx.set(hash, tx);
	socketDataNames.set(hash, name);
	socketDatas.set(hash, data);

	if (socketDataTimeline.size() > 360)
	{ // cache size limits, remove the first
		ClearDataStreamCache(socketDataTimeline[0]);
	}
}

void BurstSocket::ClearDataStreamCache(MemoryBlock hash)
{
	if (hash.getSize() > 0)
	{
		socketDataTimeline.removeAllInstancesOf(hash);
		socketDataType.remove(hash);
		socketDataTx.remove(hash);
		socketDataNames.remove(hash);
		socketDatas.remove(hash);
	}
	else
	{
		socketDataTimeline.clear();
		socketDataType.clear();
		socketDataTx.clear();
		socketDataNames.clear();
		socketDatas.clear();
	}
}

// BurstSocket Thread ------------------------------------------------------------------------------------------------
BurstSocket::BurstSocketThread::BurstSocketThread()
	: Thread("BurstSocketThread")
{
	Init();
}

void BurstSocket::BurstSocketThread::Init()
{
	poll_interval_ms = 10000;
	prevUTtc = -1;
	currActive = 0;
	maxActive = 0;
	balance = 0;
	nodeSupportsFullHashStore = true;	
	pause = true;
	forceBlock = false;
	useCustodyMode = false;
	lowFunds = false;
	blockFeesNQT = 0;
	memset(&slotIsblocked[0], 0, sizeof(int) * MAX_FEE_SLOTS);
	memset(&slotQueue[0], 0, sizeof(int) * MAX_FEE_SLOTS);
	pressure = 0.f;
	pressureSlot = 0;
	currBlockingLength = 3;
	maxSlotHeight = 10;
	currBlockSlot = 0;
	hold_multiplier = 1;
	justDonateToDev = false;
}

BurstSocket::BurstSocketThread::~BurstSocketThread()
{
}

void BurstSocket::BurstSocketThread::SetNode(String hostUrl)
{
	SetPause(true);
	BurstKit::SetNode(hostUrl);
	SetPause(false);
}

void BurstSocket::BurstSocketThread::SetDonateMode(bool on)
{
	SetPause(true);
	justDonateToDev = on;
	SetPause(false);
}

void BurstSocket::BurstSocketThread::SetSecretPhrase(String passphrase)
{
	SetPause(true);
	Init(); // reset socket status
	BurstKit::SetSecretPhrase(passphrase);
	SetPause(false);
}

void BurstSocket::BurstSocketThread::SetForceSSL_TSL(const bool forceSSLOn)
{
	SetPause(true);
	BurstKit::SetForceSSL_TSL(forceSSLOn);
	SetPause(false);
}

int BurstSocket::BurstSocketThread::GetMaxSlotHeight()
{
	const ScopedLock lock(settingLock);
	return maxSlotHeight;
}

void BurstSocket::BurstSocketThread::SetMaxSlotHeight(const int slotHeight)
{
	const ScopedLock lock(settingLock);
	maxSlotHeight = jmin<int>(jmax<int>(slotHeight, 1), SOCKET_MAX_SLOTBLOCK_HEIGHT);
}

int BurstSocket::BurstSocketThread::GetBlockingLength()
{
	const ScopedLock lock(settingLock);
	return currBlockingLength;
}

void BurstSocket::BurstSocketThread::SetBlockingLength(const int length)
{
	const ScopedLock lock(settingLock);
	currBlockingLength = jmin<int>(jmax<int>(length, 1), SOCKET_MAX_SLOTBLOCK_LENGTH);
}

int BurstSocket::BurstSocketThread::GetHoldSize()
{
	const ScopedLock lock(settingLock);
	return hold_multiplier;
}

void BurstSocket::BurstSocketThread::SetHoldSize(const int newHoldSize)
{
	const ScopedLock lock(settingLock);
	hold_multiplier = newHoldSize;
}

void BurstSocket::BurstSocketThread::GetStatus(bool &blocking, int &slot, float &percent, uint64 &currActiveIn, uint64 &maxActiveIn)
{
	blocking = (GetUseCustody() == false);
	currActiveIn = 0;
	slot = 0;
	percent = 0.f;

	GetActive(currActiveIn, maxActiveIn);
	percent = GetPressureIndicator(slot);
}

void BurstSocket::BurstSocketThread::GetActive(uint64 &currActiveIn, uint64 &maxActiveIn)
{
	const ScopedLock lock(settingLock);
	maxActiveIn = maxActive;
	currActiveIn = currActive;
}

float BurstSocket::BurstSocketThread::GetPressureIndicator(int &slot)
{
	const ScopedLock lock(pressureLock);
	slot = pressureSlot;
	return pressure;
}

void BurstSocket::BurstSocketThread::SetPressureIndicator(int slot, float pressure)
{
	const ScopedLock lock(pressureLock);
	pressureSlot = slot;
	this->pressure = pressure;
}

void BurstSocket::BurstSocketThread::SetCurrActive(uint64 currActiveIn)
{
	const ScopedLock lock(settingLock);
	currActive = currActiveIn;
}

void BurstSocket::BurstSocketThread::SetMaxActive(uint64 maxActiveIn)
{
	const ScopedLock lock(settingLock);
	maxActive = maxActiveIn;
}

bool BurstSocket::BurstSocketThread::Listen(const bool on, const String port)
{
	const ScopedLock lock(settingLock);
	if (on)
		listenPorts.add(convertToReedSolomon(port));
	else listenPorts.removeString(port);
	return true;
}

StringArray BurstSocket::BurstSocketThread::GetListenPorts()
{
	const ScopedLock lock(settingLock);
	return listenPorts;
}

bool BurstSocket::BurstSocketThread::Ignore(bool on, String port)
{
	const ScopedLock lock(settingLock);
	if (on)
		ignorePorts.add(convertToReedSolomon(port));
	else ignorePorts.removeString(port);
	return true;
}

StringArray BurstSocket::BurstSocketThread::GetIgnorePorts()
{
	const ScopedLock lock(settingLock);
	return ignorePorts;
}

void BurstSocket::BurstSocketThread::SetUseCustody(const bool useCustodyMode)
{
	const ScopedLock lock(settingLock);
	this->useCustodyMode = useCustodyMode;
}

bool BurstSocket::BurstSocketThread::GetUseCustody()
{
	const ScopedLock lock(settingLock);
	return useCustodyMode;
}

void BurstSocket::BurstSocketThread::SetPause(const bool pause)
{
	const ScopedLock lock(settingLock);
	this->pause = pause;
}

void BurstSocket::BurstSocketThread::SetForceBlock(const bool force)
{
	const ScopedLock lock(settingLock);
	this->forceBlock = force;
}

void BurstSocket::BurstSocketThread::SendBlockerTransactions(const bool slot_consensus)
{
	if (messageTxIds.size() > 0 || (slot_consensus && fifoOut.packetFifo.getNumReady() > 0) || forceBlock) // if there are active ETx or upcoming ones or forceBlock is on
	{ // add new blockers if needed
		const int blockingLength = GetBlockingLength();
		const int blockers_needed = (blockingLength - slotIsblocked[currBlockSlot]) * (currBlockSlot + 1);

		// create new blocker txs if the amount of blocked slots is lower than requierd for multiple upcoming blocks and the slots below it
		if (blockers_needed > 0) 
		{
			SendBlocker(currBlockSlot, blockers_needed); // add new blockers, always on 'currBlockSlot' otherwise our messages can fall outside the blocking scope
		}
	}
}

void BurstSocket::BurstSocketThread::SendBlocker(const int slotNr, const int blocks) // slotNr: 0-1020
{
	// get current balance
	String accountStr = getAccount(GetAccountRS());
	var accountJson;
	Result r = JSON::parse(accountStr, accountJson);
	uint64 balance_root_NQT = 0;
	uint64 balance_socket_asset_QNT = 0;
	if (r.wasOk())
	{ // use unconfiremd as the funds can be locked up in orders of the exchange
		balance_root_NQT = GetUINT64(accountJson["unconfirmedBalanceNQT"].toString());
		if (accountJson["unconfirmedAssetBalances"].isArray())
		{
			for (int i = 0; i < accountJson["unconfirmedAssetBalances"].size(); i++)
			{
				const String assetID = accountJson["unconfirmedAssetBalances"][i]["asset"];
				if (assetID.compare(IsOnTestNet() ? SOCKET_ASSET_ID_TESTNET : SOCKET_ASSET_ID_MAINNET) == 0)
				{
					const String balanceQNT = accountJson["unconfirmedAssetBalances"][i]["unconfirmedBalanceQNT"];
					balance_socket_asset_QNT = GetUINT64(balanceQNT);
				}
			}
		}
	}

	uint64 baseNQT = (uint64)(FEE_QUANT * (slotNr + 1));
	if (lowFunds)
	{ // check the balance
		String balanceNQT = String(balance_root_NQT);
		if (balanceNQT.getLargeIntValue() > baseNQT * blocks)
			lowFunds = false;
	}
	else
	{
		uint64 feeNQTint = baseNQT + 1;
		String feeNQT(feeNQTint);

		if (justDonateToDev)
		{ // support for non asset trading blockers
			String result;
			for (int i = 0; i < blocks; i++)
			{
				String deadlineMinutes(8 + i); // 8 min deadline + blocker num to space out deadlines
				
				const String amountNQT = String::formatted("%lu", std::pow(10, 7));
				result = sendMoney(DEVELOPER_ADDRESS, amountNQT, feeNQT, deadlineMinutes, String::empty, true);
				
				String errorCode = GetJSONvalue(result, "errorCode");
				error = GetJSONvalue(result, "error");
				errorDescription = GetJSONvalue(result, "errorDescription");
				if (result.isNotEmpty() && error.isEmpty() && errorDescription.isEmpty())
				{
					String txId = GetJSONvalue(result, "transaction");
					if (txId.isNotEmpty())
					{
						blockerTxIds.add(txId); // remember to check if the tx was mined
						blockFeesNQT += (feeNQTint); // total tx fees paid
					}
				}
				else
				{ // ensure we are not spamming
					if (errorCode.isNotEmpty() ||
						error.compareIgnoreCase("Insufficient funds") == 0 ||
						errorDescription.compareIgnoreCase("Not enough funds") == 0)
						lowFunds = true;
				}
			}
		}
		else
		{
			// by using the network the user holds max SOCKET_USER_TOKEN_HOLD_SIZE_IN_BURST_NQT tokens. 
			// a bid or ask order for 0.1 burst in tokens for each blocker tx.
			// get the market price for the asset. getAskOrders and buy or sell at market price, depending on the holdings
			String asset((IsOnTestNet() ? SOCKET_ASSET_ID_TESTNET : SOCKET_ASSET_ID_MAINNET));

			uint64 marketPriceNQT = 0;
			int maxOrders = 1;
			int orderIdx = 0;
			bool noOrdersFound = false;

			// calculate the ask and bid price against current market
			// our bid matches the lowset current ask
			// our ask is one SOCKET_ASSET_BID_STEP_NQT above our bid
			while (marketPriceNQT == 0 && !noOrdersFound)
			{
				String allOrdersStr = getAskOrders(asset, String(orderIdx), String(orderIdx + (maxOrders - 1)));
				orderIdx += maxOrders;
				var allOrdersJSON;
				if (JSON::parse(allOrdersStr, allOrdersJSON).wasOk() && !threadShouldExit())
				{
					var ordersArray = allOrdersJSON.getProperty("askOrders", String::empty);
					if (ordersArray.isArray())
					{
						for (int i = 0; i < ordersArray.size() && !threadShouldExit(); i++)
						{
							String accountRS = ordersArray[i]["accountRS"].toString();
							uint64 quantityQNT = GetUINT64(ordersArray[i]["quantityQNT"].toString());

							// save ask the order that has at least the minimum needed. and ensure its not the users own order
							if (quantityQNT >= pow(10, SOCKET_ASSET_DECIMALS) && // quantity must be 1 token
								accountRS.compareIgnoreCase(GetAccountRS()) != 0) // ignore own orders
							{
								marketPriceNQT = ordersArray[i]["priceNQT"].toString().getLargeIntValue() * (std::pow(10L, SOCKET_ASSET_DECIMALS));
							}
						}
						if (ordersArray.size() <= 0)
							noOrdersFound = true;
					}
					else noOrdersFound = true;
				}
				else noOrdersFound = true;
			}

			if (noOrdersFound) // some server issue..
				return;

			CreateBlock(); // make the newBlockContentsHex of the block to post

			// the bid / ask price(in NQT)
			const uint64 bidPriceNQT((marketPriceNQT + SOCKET_ASSET_ORDERBOOK_STEP_NQT)); // we add a order step amount here, because the chain will match the orders and not take more burst than needed. and this ensures we have enough orders to fill the quantity needed
			const uint64 askPriceNQT((marketPriceNQT + (SOCKET_ASSET_ORDERBOOK_STEP_NQT * ((uint64)slotNr + 1)))); // we add a order step amount here times the slot height (to offset tx fees), as this will increase the possibility for a return upon using and supporting the network with blocker txs
			// calc the amount of plancks per QNT of the asset
			const String bidPriceCQTstr(jmax<uint64>(1, bidPriceNQT / (std::pow(10L, SOCKET_ASSET_DECIMALS)))); // divide by 10^SOCKET_ASSET_DECIMALS. needed to match API QNT to NQT conversion
			const String askPriceCQTstr(jmax<uint64>(1, askPriceNQT / (std::pow(10L, SOCKET_ASSET_DECIMALS)))); // max decimals of the price is the 8 decimals of burst minus the decimals of the asset

			const uint64 bidQuantityQNT = (jmin<uint64>(balance_root_NQT, SOCKET_ASSET_BASE_BURST_QUANTITY_QNT) * (std::pow(10L, 8L)) / (marketPriceNQT)); // the ask quantity in QNT of the asset is SOCKET_ASSET_BASE_BURST_QUANTITY_QNT divided by the bid price
			const String bidQuantityQNTStr = String::formatted("%lu", bidQuantityQNT);
			const uint64 askQuantityQNT = (jmin<uint64>(balance_socket_asset_QNT, (SOCKET_ASSET_BASE_BURST_QUANTITY_QNT * std::pow(10L, 8L)) / (marketPriceNQT))); // the ask quantity in QNT of the asset is SOCKET_ASSET_BASE_BURST_QUANTITY_QNT divided by the ask price. but at most the amount the account has
			const String askQuantityQNTStr = String::formatted("%lu", askQuantityQNT);

			for (int i = 0; i < blocks; i++)
			{
				uint64 needed_balance_in_burst_NQT = SOCKET_USER_TOKEN_HOLD_SIZE_IN_BURST_NQT * GetHoldSize();
				uint64 needed_balance_socket_asset_QNT = (needed_balance_in_burst_NQT * std::pow(10L, 8L)) / marketPriceNQT; // the total amount the user should hold in Burst divided by the market price of the token
				const bool askOrder = (balance_socket_asset_QNT >= needed_balance_socket_asset_QNT);
	
				// add blocks i here to space the deadlines out. less risk of having a moment without any blockers
				String askDeadlineMinutes(16 + i); // lowered this deadline. if you just sold an asset and posted a ask order just before. it can get stuck in the mempool (for deadline amount of time)
				String bidDeadlineMinutes(8 + i); // 8 min deadline. as the message contents for the bid orders needs to be as recent as possible

				String result;
				if (askOrder) // bid or ask depending on token holdings
					result = placeAskOrder(asset, askQuantityQNTStr, askPriceCQTstr, feeNQT, askDeadlineMinutes, String::empty, false, true);
				else result = placeBidOrder(asset, bidQuantityQNTStr, bidPriceCQTstr, feeNQT, bidDeadlineMinutes, newBlockContentsHex, false, true); // add the newBlockContentsHex only in the bid orders, so it mines faster (not having to wait for market bid as ask order)

				String errorCode = GetJSONvalue(result, "errorCode");
				error = GetJSONvalue(result, "error");
				errorDescription = GetJSONvalue(result, "errorDescription");
				if (result.isNotEmpty() && error.isEmpty() && errorDescription.isEmpty())
				{
					String txId = GetJSONvalue(result, "transaction");
					if (txId.isNotEmpty())
					{
						blockerTxIds.add(txId); // remember to check if the tx was mined
						blockFeesNQT += (feeNQTint); // total tx fees paid
					}
				}
				else
				{ // ensure we are not spamming
					if (errorCode.isNotEmpty() ||
						error.compareIgnoreCase("Insufficient funds") == 0 ||
						errorDescription.compareIgnoreCase("Not enough funds") == 0)
						lowFunds = true;
				}
			}
		}
	}
}

void BurstSocket::BurstSocketThread::SendFifo(const bool slot_consensus, const bool useCustody, const int64 mempoolSize, const int64 activeStake)
{
	if (fifoOut.packetFifo.getNumReady() <= 0)
		return;
	uint64 nqt = 0; // fee amount
	if (useCustody == false)
	{
		const int blockingLength = GetBlockingLength();
		if (slot_consensus && slotIsblocked[currBlockSlot] >= blockingLength)
		{
			uint64 bestSlot = 0; // use the slot that has the least UTx
			int lowestQueueSize = MAX_TX_PER_SLOT;
			for (int s = 0; s < currBlockSlot + 1; s++)
			{
				if (lowestQueueSize > slotQueue[s])
				{
					lowestQueueSize = slotQueue[s];
					bestSlot = s;
				}
			}
			if (lowFunds)
			{ // check the balance
				String balanceNQT = String(balance);// GetJSONvalue(getBalance(GetAccountRS(), String::empty, String::empty, String::empty, String::empty), "balanceNQT");
				if (balanceNQT.getLargeIntValue() > ((uint64)FEE_QUANT * (bestSlot + 1L)))
					lowFunds = false;
			}
			nqt = ((uint64)FEE_QUANT * (bestSlot + 1L));
			if (lowFunds)
				return;
		}
		else return;
	}
	else
	{
		uint64 availableBalanceNQT = balance - (activeStake * 10000000000);
		nqt = jmax<uint64>(((mempoolSize / 360) + 1) * FEE_QUANT, 200000000); // ensure the fee is high enough to be included in the mempool

		if (availableBalanceNQT <= nqt + (10000000000)) // need at least 102 BURST available
			return;
	}

	{
		BurstSocket::BurstSocketThread::txPacketOut p = BurstSocket::BurstSocketThread::initPacketOut();
		if (fifoOut.readFromFifo(&p, 1, true) > 0)
		{
			String recipient = String(p.recipient);
			if (recipient.isNotEmpty())
			{
				String message;
				String messageIsText;
				String encryptedMessageData;
				String encryptedMessageNonce;
				String encryptToSelfMessageData;
				String encryptToSelfMessageNonce;
				String messageToEncryptIsText;

				if (p.isText)
				{
					messageIsText = "true";
					if (p.encrypted == false)
						message = MemoryBlock(&(p.message[0]), p.messageSize).toString();
					else
					{
						String messageToEncrypt = MemoryBlock(&(p.message[0]), p.messageSize).toString(); // is either UTF - 8 text or a string of hex digits to be compressed and converted into a 1000 byte maximum bytecode then encrypted using AES
						messageToEncryptIsText = ("true"); // is false if the message to encrypt is a hex string, true if the encrypted message is text
						encryptedMessageData = encryptTo( // Encrypt a message using AES without sending it.
							encryptedMessageNonce,
							recipient, // is the account ID of the recipient.
							messageToEncrypt, // is either UTF - 8 text or a string of hex digits to be compressed and converted into a 1000 byte maximum bytecode then encrypted using AES
							messageToEncryptIsText); // is false if the message to encrypt is a hex string, true if the encrypted message is text
					}
				}
				else
				{ // is Data
					messageIsText = "false";
					if (p.encrypted == false)
						message = String::toHexString((&(p.message[0])), p.messageSize, 0);
					else
					{
						String messageToEncrypt = String::toHexString((&(p.message[0])), p.messageSize, 0); // is either UTF - 8 text or a string of hex digits to be compressed and converted into a 1000 byte maximum bytecode then encrypted using AES
						messageToEncryptIsText = ("false"); // is false if the message to encrypt is a hex string, true if the encrypted message is text
						encryptedMessageData = encryptTo( // Encrypt a message using AES without sending it.
							encryptedMessageNonce,
							recipient, // is the account ID of the recipient.
							messageToEncrypt, // is either UTF - 8 text or a string of hex digits to be compressed and converted into a 1000 byte maximum bytecode then encrypted using AES
							messageToEncryptIsText); // is false if the message to encrypt is a hex string, true if the encrypted message is text
					}
				}

				String recipientPublicKey; // used to set pubkey of account
				String messageToEncrypt;
				String messageToEncryptToSelf;
				String messageToEncryptToSelfIsText;				
				String feeNQT(nqt);
				String deadlineMinutes("1");
				bool broadcast = true;

				String txRes("stub");
				String referencedTransactionFullHash;
				if (useCustody)
				{
					Random rander;
					while (txRes.isNotEmpty() &&
						txRes.contains("errorCode") == false) // it should give a error
					{ // RANDOM reference ensure it does not exist on the chain. if this while loop ever runs twice buy a lottery ticket ! NAOW!
						MemoryBlock rmb;
						for (int j = 0; j < 4; j++)
						{
							uint64 ranInt = rander.nextInt64() * rander.nextInt64();
							rmb.append(&ranInt, sizeof(uint64));
						}
						referencedTransactionFullHash = SHA256(rmb).toHexString();
						txRes = getTransaction("", referencedTransactionFullHash);
					}
				}

				String sendMessageResponse = sendMessage(recipient,
					message, // is either UTF - 8 text or a string of hex digits(perhaps previously encoded using an arbitrary algorithm) to be converted into a bytecode with a maximum length of one kilobyte(optional)
					messageIsText, // is false if the message is a hex string, otherwise the message is text (optional)
					messageToEncrypt, // is either UTF-8 text or a string of hex digits to be compressed and converted into a bytecode with a maximum length of one kilobyte, then encrypted using AES (optional)
					messageToEncryptIsText, // is false if the message to encrypt is a hex string, otherwise the message to encrypt is text (optional)
					encryptedMessageData, // is already encrypted data which overrides messageToEncrypt if provided (optional)
					encryptedMessageNonce, // is a unique 32-byte number which cannot be reused (optional unless encryptedMessageData is provided)
					messageToEncryptToSelf, // is either UTF-8 text or a string of hex digits to be compressed and converted into a one kilobyte maximum bytecode then encrypted with AES, then sent to the sending account (optional)
					messageToEncryptToSelfIsText, // is false if the message to self-encrypt is a hex string, otherwise the message to encrypt is text (optional)
					encryptToSelfMessageData, // is already encrypted data which overrides messageToEncryptToSelf if provided (optional)
					encryptToSelfMessageNonce, // is a unique 32-byte number which cannot be reused (optional unless encryptToSelfMessageData is provided)
					recipientPublicKey, // is the public key of the receiving account (optional, enhances security of a new account) // another account can announce the public key of the new account to the blockchain Any type of transaction in which the recipient is the new account will do.The sender needs to specify the new account public key as the "recipientPublicKey" parameter for the transaction API or using the wallet "Recipient Public Key" field.Most exchanges which support NXT, already supports this public key announcement function.
					referencedTransactionFullHash, // 32byte HEX  2 BURST for transactions that make use of referencedTransactionFullHash property when creating a new transaction.
					feeNQT,
					deadlineMinutes,
					broadcast);

				String errorCode = GetJSONvalue(sendMessageResponse, "errorCode");
				String error = GetJSONvalue(sendMessageResponse, "error");
				String errorDescription = GetJSONvalue(sendMessageResponse, "errorDescription");
				if (error.isEmpty() && errorDescription.isEmpty())
				{
					bool success = false;
					String txId = GetJSONvalue(sendMessageResponse, "transaction");
					if (nodeSupportsFullHashStore)
					{
						if (txId.isNotEmpty())
						{ // confirm tx is actually there
							Time::waitForMillisecondCounter(Time::getApproximateMillisecondCounter() + 500);
							String txRes = getTransaction(txId);
							if (txRes.contains("errorCode") == false)
								success = true;
							// else something went wrong, maybe the slots are full?
						}
					}
					else success = true;
					
					if (success == true)
					{ // remove it from the fifo after the messge really has been send. and cache the txid
						messageTxIds.add(txId);
						fifoOut.readFromFifo(0, 1); 

						if (p.encrypted)
						{ // keep a list of send encrypted messages we send. because we cannot read the contents back from UTstore
							BurstSocket::BurstSocketThread::txPacketIn pIn = BurstSocket::BurstSocketThread::initPacketIn();

							pIn.recipient = p.recipient;
							pIn.isText = p.isText;
							pIn.encrypted = p.encrypted;
							memcpy(&(pIn.message[0]), &(p.message[0]), MAX_TXDATASIZE);
							pIn.messageSize = p.messageSize;

							pIn.txid = GetUINT64(txId);
							pIn.feeNQT = nqt;
							pIn.amountNQT = 0;

							sendEncryptedMessages.add(pIn);
						}
					}
				}
				else
				{
					if (errorCode.isNotEmpty() ||
						error.compareIgnoreCase("Insufficient funds") == 0 ||
						errorDescription.compareIgnoreCase("Not enough funds") == 0)
						lowFunds = true;

					if (socketErrorCount++ > 3)
					{ // ensure we are not spamming. this would be PACKET LOSS !
						SetError(1, "PACKET LOSS: " + error + " - " + errorDescription);
						fifoOut.readFromFifo(0, 1);
						socketErrorCount = 0;
					}
				}
			}
		}
	}
}

void BurstSocket::BurstSocketThread::CalculateSlotAllocation(Array<uint64> &ut_ids, Array<uint64> &ut_fees)
{
	// sort UTs by fee
	for (int i = 0; i < ut_ids.size() - 1; i++)
	{
		for (int j = i + 1; j < ut_ids.size(); j++)
		{
			if (ut_fees[i] < ut_fees[j])
			{
				ut_ids.swap(i, j);
				ut_fees.swap(i, j);
			}
		}
	}
	// create 2 dim arrays and predict the txs that are filled for each slot and upcoming blocks
	// dim 1: 1020 slots (MAX_FEE_SLOTS)
	// dim 2: dynamic array with the tx data for coming blocks sorted from current to future
	for (int i = 0; i < MAX_FEE_SLOTS; i++)
	{
		slotUT_id[i].clearQuick();
		slotUT_fee[i].clearQuick();
	}
	// slotQueueNew saves the amount of tx in each slot
	int slotQueueNew[MAX_FEE_SLOTS];
	memset(&slotQueueNew[0], 0, sizeof(int) * MAX_FEE_SLOTS);

	Array<uint64> excludedIds; // to ensure to skip known txs. as we iterate the array multiple times to fill all slots for multiple blocks

	for (int block_nr = 0; block_nr < SOCKET_MAX_PREDICT_BLOCK_DEPTH; block_nr++) // determine the slots that are used for the upcoming blocks
	{ // fill the slots with the sorted TXs with matching fee.
		uint64 slotIter = 0;
		for (int txi = 0; txi < ut_ids.size() && excludedIds.size() < ut_ids.size(); txi++)
		{
			const uint64 id = ut_ids[txi];
			if (excludedIds.contains(id) == false)
			{
				// find the slot for this tx to be saved in
				const uint64 fee = ut_fees[txi];				
				bool filledSlot = false;
				while (slotIter < MAX_FEE_SLOTS && !filledSlot) // go backwards from highest to lowest fee
				{ // take the txi with ascending fees
					if (fee >= ((uint64)MAX_FEE_SLOTS - slotIter) * FEE_QUANT) // compare the fee of this tx, with the fee of the current slot index
					{ // fee is equal or larger
						
						int oldSize = slotUT_id[(MAX_FEE_SLOTS - 1) - slotIter].size();
						int oldSizeFee = slotUT_fee[(MAX_FEE_SLOTS - 1) - slotIter].size();
						
						// resize the arrays
						if (oldSize < block_nr + 1)
						{ // resize the slotUT_id result array and clear
							slotUT_id[(MAX_FEE_SLOTS - 1) - slotIter].resize(block_nr + 1);
							for (int d_new = oldSize; d_new < block_nr + 1; d_new++)
								slotUT_id[(MAX_FEE_SLOTS - 1) - slotIter].getReference(d_new) = 0;
						}
						if (oldSizeFee < block_nr + 1)
						{ // resize the slotUTfee result array and clear
							slotUT_fee[(MAX_FEE_SLOTS - 1) - slotIter].resize(block_nr + 1);
							for (int d_new = oldSizeFee; d_new < block_nr + 1; d_new++)
								slotUT_fee[(MAX_FEE_SLOTS - 1) - slotIter].getReference(d_new) = 0;
						}

						// inserting the tx in the most expensive slot possible
						if (block_nr < slotUT_id[(MAX_FEE_SLOTS - 1) - slotIter].size())
							slotUT_id[(MAX_FEE_SLOTS - 1) - slotIter].getReference(block_nr) = id;
						if (block_nr < slotUT_fee[(MAX_FEE_SLOTS - 1) - slotIter].size())
							slotUT_fee[(MAX_FEE_SLOTS - 1) - slotIter].getReference(block_nr) = fee;

						slotQueueNew[(MAX_FEE_SLOTS - 1) - slotIter] += 1; // the amount of future blocks that will be probably be filled for each slot in the array

						filledSlot = true; // done here. move on to next slot
						excludedIds.add(id);
					}
					slotIter++; // either the slot got filled or not. we move to the next slot one fee step down.
				}
			}
		}
	}
	// update the array
	memcpy(&slotQueue[0], slotQueueNew, sizeof(int) * MAX_FEE_SLOTS);
}

void BurstSocket::BurstSocketThread::CalculateSlotPressures(const bool slot_consensus)
{
	if (slot_consensus)
	{ 
		// calculate the slots that are blocked. 1 NQT above minimum
		for (uint64 l = 0; l < SOCKET_MAX_SLOTBLOCK_HEIGHT; l++)
		{
			slotIsblocked[l] = 0;	// reset blocking count
			uint64 nqt = ((uint64)FEE_QUANT * (l + 1));
			// the slot is blocked. if the tx has a fee BIGGER than the nqt. as our messages will use the fee of nqt itself
			for (int d = 0; d < jmin<int>(slotUT_id[l].size(), slotUT_fee[l].size()); d++)
				slotIsblocked[l] += (slotUT_id[l][d] > 0 && slotUT_fee[l][d] > nqt ? 1 : 0);
		}

		// check the slot tx pressure
		// each slot up to currBlockSlot can hold (360 tx * the slot heigth)						
		int switchLane = 0;
		int seachingHeight = GetMaxSlotHeight();
		int freeSpace = 0;
		do { // search downward from max slot height. for the best blocking height without obstructing the network
			freeSpace = 0; // we add up the amount of free space.
			int newSlotHeight = (currBlockSlot + switchLane + 1);
			for (int s = 0; s < newSlotHeight; s++)
				freeSpace += ((MAX_TX_PER_SLOT * (s + 1)) - slotQueue[s]); // add the max slotsize, minus the amount of tx that are loaded up in that slot for future blocks
						
			if (freeSpace < (MAX_TX_PER_SLOT / 2) && // if freespace is less than half a slot space
				newSlotHeight < GetMaxSlotHeight()) // and not beyond max height
				switchLane += 1; // we need to go up a lane
			else if (freeSpace > MAX_TX_PER_SLOT &&  // if there is more than 360 free
				(currBlockSlot + switchLane) > 0) // and if we are able to go down
				switchLane -= 1; // can go down a lane
			else seachingHeight = 0; // done
		} while (seachingHeight-- > 0);

		currBlockSlot += switchLane; // switch BT slot if needed

		SetPressureIndicator(currBlockSlot, ((float)MAX_TX_PER_SLOT - freeSpace) / (float)MAX_TX_PER_SLOT);
	}
}

void BurstSocket::BurstSocketThread::ForgeCheck(bool &slot_consensus)
{
	if (messageTxIds.size() > 0)
	{ // check if our messages are being forged. if this is the case the thread will end with a warning
		StringArray removeTxIds;
		for (int midx = 0; midx < jmin<int>(messageTxIds.size(), 3); midx++)
		{
			const String txId = messageTxIds[midx];
			const uint64 uTXid = GetUINT64(txId);
			const String txData = txMap[uTXid];
			if (txId.isNotEmpty() && txData.isNotEmpty())
			{
				// get the dealine of the tx
				const String timestampStr = GetJSONvalue(txData, "timestamp");
				const String deadlineStr = GetJSONvalue(txData, "deadline");
				uint64 deadline = GetUINT64(deadlineStr) + 1;
				uint64 timestamp = GetUINT64(timestampStr) + BURSTCOIN_GENESIS_EPOCH + (deadline * 60);

				if ((Time::currentTimeMillis() / 1000) > timestamp) // if the deadline has expired
				{ // check if its really gone
					const String txRes = getTransaction(txId);
					const String height = GetJSONvalue(txRes, "height");
					const String errorCode = GetJSONvalue(txRes, "errorCode");
					if (height.isNotEmpty()) // this tx should be off-chain. if there is a block height, 'something' went wrong. fat blocks?
						slot_consensus = false;
					if (errorCode.isNotEmpty()) // the tx probably doesnt exist anymore, as it should be {"errorDescription":"Unknown transaction","errorCode":5}
						removeTxIds.add(txId);
				}
			}
		}
		for (int r = 0; r < removeTxIds.size(); r++)
			messageTxIds.removeString(removeTxIds[r]); // only keep message ids that are 'alive'
	}
}

void BurstSocket::BurstSocketThread::ProcessUTids(var unconfirmedTransactionIds, int &activeStake, int &prevNumUTs, Array<uint64> &ut_ids, Array<uint64> &ut_fees)
{
	// collect the data for each TX id in txMap
	int numUTs = unconfirmedTransactionIds["unconfirmedTransactionIds"].size();
	if (prevNumUTs >= 0 && numUTs < prevNumUTs / 2) // if there is a drasctic drop in UTs. 50% to prev. then wait a round to confirm that
		numUTs = 0;
	prevNumUTs = numUTs;

	StringArray lps = GetListenPorts();
	StringArray ips = GetIgnorePorts();

	for (int i = 0; i < numUTs; i++)
	{
		String uTXidstr = unconfirmedTransactionIds["unconfirmedTransactionIds"][i].toString();
		uint64 uTXid = GetUINT64(uTXidstr);

		if (uTXid > 0)
		{
			var uTXdata;
			String uTXdataStr;
			if (!txMap.contains(uTXid))
			{
				uTXdataStr = getTransaction(uTXidstr);
				Result r2 = JSON::parse(uTXdataStr, uTXdata);

				String senderRS = uTXdata["senderRS"].toString();
				String recipientRS = uTXdata["recipientRS"].toString();
				if (senderRS.isNotEmpty() &&
					(lps.size() <= 0 || lps.contains(senderRS)) && // if receiver is same as listen port, or no ports defined
					(ips.size() <= 0 || ips.contains(senderRS) == false))
				{
					txMap.set(uTXid, uTXdataStr);
					const var attachment = uTXdata["attachment"];
					if ((int)(attachment["version.Message"]) == 1)
					{ // unencrypted msg
						BurstSocket::BurstSocketThread::txPacketIn p = BurstSocket::BurstSocketThread::initPacketIn();
						p.encrypted = false;
						p.isText = (bool)(attachment["messageIsText"]);

						p.txid = GetUINT64(uTXdata["transaction"].toString());
						p.sender = GetUINT64(uTXdata["sender"].toString());
						p.recipient = GetUINT64(uTXdata["recipient"].toString());
						p.feeNQT = GetUINT64(uTXdata["feeNQT"].toString());
						p.amountNQT = GetUINT64(uTXdata["amountNQT"].toString());
						p.timestamp = GetUINT64(uTXdata["timestamp"].toString()) + BURSTCOIN_GENESIS_EPOCH;

						String message(attachment["message"].toString());
						if (p.isText)
						{
							p.messageSize = jmin<int>(MAX_TXDATASIZE, message.getNumBytesAsUTF8());
							message.copyToUTF8(&(p.message[0]), MAX_TXDATASIZE);
							fifoInText.addToFifo(&p, 1);
						}
						else
						{
							MemoryBlock m;
							m.loadFromHexString(message);
							p.messageSize = jmin<int>(MAX_TXDATASIZE, m.getSize());
							m.copyTo(&(p.message[0]), 0, p.messageSize);

							fifoInData.addToFifo(&p, 1);
						}
					}
					if ((int)(attachment["version.EncryptedMessage"]) == 1 && senderRS.compareIgnoreCase(GetAccountRS()) == 0)
					{ // a check if our own send encrypted messages are visible in the mempool
						// we cannot read the messages we send (unless its a boomerang). so use the previous send data list.
						uint64 txid = GetUINT64(uTXdata["transaction"].toString());
						const var encryptedMessage = attachment["encryptedMessage"];
						bool isText = (bool)(encryptedMessage["isText"]);
						for (int si = 0; si < sendEncryptedMessages.size(); si++)
						{
							if (sendEncryptedMessages[si].txid == txid)
							{ // copy and remove the data from our send cache
								BurstSocket::BurstSocketThread::txPacketIn p = sendEncryptedMessages[si];
								p.sender = GetUINT64(uTXdata["sender"].toString());;
								p.timestamp = GetUINT64(uTXdata["timestamp"].toString()) + BURSTCOIN_GENESIS_EPOCH;

								if (isText)
									fifoInText.addToFifo(&p, 1);
								else fifoInData.addToFifo(&p, 1);

								sendEncryptedMessages.remove(si);
								si = sendEncryptedMessages.size() + 1;
							}
						}
					}
					if ((int)(attachment["version.EncryptedMessage"]) == 1 && recipientRS.compareIgnoreCase(GetAccountRS()) == 0)
					{ // encryptedMessage
						const var encryptedMessage = attachment["encryptedMessage"];

						BurstSocket::BurstSocketThread::txPacketIn p = BurstSocket::BurstSocketThread::initPacketIn();
						p.encrypted = true;
						p.isText = (bool)(encryptedMessage["isText"]);

						p.txid = GetUINT64(uTXdata["transaction"].toString());
						p.sender = GetUINT64(uTXdata["sender"].toString());
						p.recipient = GetUINT64(uTXdata["recipient"].toString());
						p.feeNQT = GetUINT64(uTXdata["feeNQT"].toString());
						p.amountNQT = GetUINT64(uTXdata["amountNQT"].toString());
						p.timestamp = GetUINT64(uTXdata["timestamp"].toString()) + BURSTCOIN_GENESIS_EPOCH;

						String data(encryptedMessage["data"].toString()); // is the AES-encrypted data
						String nonce(encryptedMessage["nonce"].toString()); // is the unique nonce associated with the encrypted data
						String decryptedMessageIsText(p.isText ? "true" : "false"); // is false if the decrypted message is a hex string, true if the decrypted message is text
						String decryptedMessage = decryptFrom(senderRS, data, nonce, decryptedMessageIsText);

						if (p.isText)
						{
							p.messageSize = jmin<int>(MAX_TXDATASIZE, decryptedMessage.getNumBytesAsUTF8());
							decryptedMessage.copyToUTF8(&(p.message[0]), MAX_TXDATASIZE);
							fifoInText.addToFifo(&p, 1);
						}
						else
						{
							MemoryBlock decryptedMessageMb;
							decryptedMessageMb.loadFromHexString(decryptedMessage);
							decryptedMessageMb.copyTo(&(p.message[0]), 0, MAX_TXDATASIZE);
							p.messageSize = jmin<int>(MAX_TXDATASIZE, decryptedMessageMb.getSize());
							fifoInData.addToFifo(&p, 1);
						}
					}
				}
			}
			else
			{
				uTXdataStr = txMap[uTXid];
				Result r2 = JSON::parse(uTXdataStr, uTXdata);
			}
			const String deadline = uTXdata["deadline"].toString(); // in minutes
			const uint64 feeNQT = uTXdata["feeNQT"].toString().getLargeIntValue();

			activeStake += messageTxIds.contains(uTXidstr) ? 1 : 0;

			ut_ids.add(uTXid);
			ut_fees.add(feeNQT);
		}
	}
}

void BurstSocket::BurstSocketThread::CreateBlock()
{
	uint64 satsPerBurst = 0;
	{
		URL cpURL("https://api.coinpaprika.com/v1/coins/BURST-BURST/markets?quotes=BTC");
		double price_final = 0.;
		const String cpOutput = cpURL.readEntireTextStream();
		var cpOutputJSON;
		if (JSON::parse(cpOutput, cpOutputJSON).wasOk())
		{
			if (cpOutputJSON.isArray())
			{
				Array<double> prices;
				for (int i = 0; i < cpOutputJSON.size(); i++)
				{
					const var item = cpOutputJSON[i];
					{
						double price_exchange = item["quotes"]["BTC"]["price"];

						if (price_final == 0. || price_exchange < price_final)
							price_final = price_exchange;

						prices.add(price_exchange);
					}
				}

				prices.sort();
				if (prices.size() > 1) // use the middle price as truth. as some quotes are way too low of high
					price_final = prices[prices.size() / 2];

				if (price_final > 0.)
					satsPerBurst = price_final * std::pow(10L, 8L);
			}
		}
	}

	const int16 version = 0x0001;

	MemoryBlock newBlockContents;
	newBlockContents.append(&version, sizeof(version));
	newBlockContents.append(&satsPerBurst, sizeof(satsPerBurst));

	newBlockContentsHex = String::toHexString(newBlockContents.getData(), newBlockContents.getSize(), 0); // LSB
}

uint64 BurstSocket::BurstSocketThread::syncSidechain()
{
	// fetch the txids of the trades of the last 10 blocks
	String asset((IsOnTestNet() ? SOCKET_ASSET_ID_TESTNET : SOCKET_ASSET_ID_MAINNET));
	int firstIndex = 0, lastIndex = 1;
	
	while (firstIndex != lastIndex)
	{
		const String trades = getTrades(asset, String::empty, String(firstIndex), String(lastIndex), "false");
		var tradesJSON;
		Result r = JSON::parse(trades, tradesJSON);
		if (r.wasOk() && tradesJSON["trades"].isArray())
		{
			for (int i = 0; i < tradesJSON["trades"].size(); i++)
			{
				const String transactionId = tradesJSON["trades"][i]["bidOrder"];
				var transactionJSON;
				const String transaction = getTransaction(transactionId);
				Result r2 = JSON::parse(transaction, transactionJSON);
				if (r.wasOk() && 
					transactionJSON["type"] == var(2) &&
					transactionJSON["subtype"] == var(3) &&
					((int)transactionJSON["attachment"]["version.Message"] == 1) &&
					((bool)transactionJSON["attachment"]["messageIsText"] == false))
				{
/*	dependent on consesus algo determine if this data is valid. options/notes;
	reference to the previous blok (fullhashref) get the longest chain with the price that is closest to the average (bell curve).
	proof of work to determine the data value/rarity. more leading zeros in the hash is better. with a time limit.
	signed via tx
	keep local a copy of all balances. sync up on connect
*/
					transactionJSON["attachment"]["message"];

					transactionJSON["amountNQT"];
					transactionJSON["feeNQT"];

					transactionJSON["height"];
					tradesJSON["height"];
				}
			}
		}
	}
	return 0;
}

void BurstSocket::BurstSocketThread::run()
{
	const ScopedTryLock tryLock(runLock);
	if (tryLock.isLocked() && !threadShouldExit())
	{
		// check if the asset exists. else default back to SetDonateMode. useful for private chains
		String result = getAsset(IsOnTestNet() ? SOCKET_ASSET_ID_TESTNET : SOCKET_ASSET_ID_MAINNET);
		if(result.contains(IsOnTestNet() ? SOCKET_ASSET_ID_TESTNET : SOCKET_ASSET_ID_MAINNET) == false)
			SetDonateMode(true);

		Random random;
		uint64 BTtc = 0;
		bool slot_consensus = true;
		int prevNumUTs = -1;
		int activeStake = 0;
		while (!threadShouldExit() && slot_consensus)
		{
			while (!threadShouldExit() && pause)
				Time::waitForMillisecondCounter(Time::getApproximateMillisecondCounter() + 500);

			if (pause == false)
			{
				bool useCustody = GetUseCustody();
			/*	// auto toggle off custody mode. 1020 minimum BURST for custody mode
				if (useCustody)
				{
					blocking = ((uint64)(balance / (102. * 100000000.))) < 10; // 102 burst per msg. 10 msg per min minimaal
					SetBlock(blocking);
				}*/

				uint32 endMs = Time::getApproximateMillisecondCounter() + poll_interval_ms + (random.nextInt() % 100); // randomize in regards to detection and distribution			
				String getUnconfirmedTransactionsIdsString = getUnconfirmedTransactionsIds();// get all UTx ids
				var unconfirmedTransactionIds;
				Result r = JSON::parse(getUnconfirmedTransactionsIdsString, unconfirmedTransactionIds);
				String lastUnconfirmedTransactionTimestamp = unconfirmedTransactionIds["lastUnconfirmedTransactionTimestamp"].toString();
				int64 uTtc = lastUnconfirmedTransactionTimestamp.getLargeIntValue();
				if ((prevUTtc < uTtc && uTtc > 0) || // ignore if the Timestamp is not newer. (if there is a Timestamp)
					(getUnconfirmedTransactionsIdsString.contains("unconfirmedTransactionIds") && lastUnconfirmedTransactionTimestamp.isEmpty()))
				{
					activeStake = 0;
					if (uTtc > 0)
						prevUTtc = uTtc;
					Array<uint64> ut_ids;
					Array<uint64> ut_fees;

					ProcessUTids(unconfirmedTransactionIds, activeStake, prevNumUTs, ut_ids, ut_fees);
					if (ut_ids.size() > 0)
					{ // if there are no UTs there is nothing to update. the Burst API might give a faulty/clipped/empty list
						CalculateSlotAllocation(ut_ids, ut_fees); // calculate the slots that are blocked
						CalculateSlotPressures(slot_consensus);
					}
					if (useCustody == false)
						SendBlockerTransactions(slot_consensus);  // send the blocker txs if needed
				}
				ForgeCheck(slot_consensus); // check if msgs are being forged/mined

				if (useCustody)
				{ // custody mode
					SetMaxActive((uint64)(balance / (102. * 100000000.))); // 102 burst per msg
					SetCurrActive(activeStake);
				}

				SendFifo(slot_consensus, useCustody, prevNumUTs, activeStake);

				do {
					Time::waitForMillisecondCounter(Time::getApproximateMillisecondCounter() + 500); // suspend, prevent too many server requests
				} while (Time::getApproximateMillisecondCounter() < endMs && fifoOut.packetFifo.getNumReady() <= 0 && !threadShouldExit());
			}
		}
	}
}
