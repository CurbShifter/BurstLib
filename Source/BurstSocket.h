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

#ifndef BurstSocket_H_INCLUDED
#define BurstSocket_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "BurstKit.h"
#include <boost/crc.hpp>

// BurstSocket Asset definitions
#define SOCKET_ASSET_NAME "Socket"
#define SOCKET_ASSET_TOTAL_QUANTITY (181800)
#define SOCKET_ASSET_DECIMALS 8
#define SOCKET_ASSET_DESCRIPTION ("BurstSocket - sidechain and data transfer token")

#define SOCKET_ASSET_ID_MAINNET "10987192031618394393"
#define SOCKET_ASSET_ID_TESTNET "14100336206556672026"

#define DEVELOPER_ADDRESS "BURST-SCKT-GBVL-X3ZD-GQHNQ"

// the max slot height that can be blocked. max tx throughput is the used height times 360 each minute. ie. 10 * 360 / 60 = 600 tx/sec
#define SOCKET_MAX_SLOTBLOCK_HEIGHT 10
// the max possible amount of burstcoin-blocks on the chain to be blocked
#define SOCKET_MAX_SLOTBLOCK_LENGTH 5
// the base amount of tokens, denominated in burst, the user should hold (without user multipliers)
#define SOCKET_USER_TOKEN_HOLD_SIZE_IN_BURST_NQT (1 * std::pow(10L, 8L))
// the order quantity in plancks for each blocker transaction. 1 Burst in plancks
#define SOCKET_ASSET_BASE_BURST_QUANTITY_QNT (std::pow(10, 8))

// base amount of fill prices
#define SOCKET_ASSET_ORDERBOOK_MAX_STEPS 30
// the token start selling price
#define SOCKET_ASSET_ORDERBOOK_START_NQT (std::pow(10, 8))
// the token selling price step increment size upon the start price
#define SOCKET_ASSET_ORDERBOOK_STEP_NQT (std::pow(10, 8))
// a multiplier for the quantity of tokens for each step up from the start price. matching 360 blocks a day
#define SOCKET_ASSET_ORDERBOOK_STEP_QUANTITY_MULTIPLIER 36

// the max size of a file stream. (1 kb is one message)
#define SOCKET_MAX_SEND_KB 64
// the max amount of blocks that will be predicted from the mempool
#define SOCKET_MAX_PREDICT_BLOCK_DEPTH 360


class BurstSocket : public BurstKit
{
public:
	BurstSocket(String hostUrl = String::empty);
	~BurstSocket();

	void SetNode(String hostUrl);
	void SetSecretPhrase(String passphrase);
	void SetForceSSL_TSL(const bool forceSSLOn);
	void SetDonateMode(bool on);

	String GetLastError(int &errorCode);
	void SetError(int errorCode, String msg);
	
	class BurstSocketThread : public Thread, public BurstKit
	{
	public:
		BurstSocketThread();
		~BurstSocketThread();

		void Init();

		void SetNode(String hostUrl);
		void SetSecretPhrase(String passphrase);
		void SetForceSSL_TSL(const bool forceSSLOn);
		void SetDonateMode(bool on);

		uint64 syncSidechain();
		void run();
		
		void SetPause(const bool pause);
		void SetUseCustody(const bool useCustodyMode);
		bool GetUseCustody();
		void SetForceBlock(const bool force);

		void GetStatus(bool &blocking, int &slot, float &percent, uint64 &currActiveIn, uint64 &maxActiveIn);

		int GetMaxSlotHeight();
		void SetMaxSlotHeight(const int slotHeight);
		int GetBlockingLength();
		void SetBlockingLength(const int slotDepth);
		int GetHoldSize();
		void SetHoldSize(const int newHoldSize);

		void CreateBlock();

		bool Listen(const bool on, const String port);
		StringArray GetListenPorts();
		bool Ignore(bool on, String port);
		StringArray GetIgnorePorts();

		struct txPacketOut
		{
			txPacketOut() :
				version(0),
				recipient(0),
				isText(false),
				encrypted(false),
				messageSize(0)
			{
				memset(message, 0, sizeof(char) * MAX_TXDATASIZE);
			}
			uint64 version;
			uint64 recipient;
			bool isText;
			bool encrypted;
			char message[MAX_TXDATASIZE];
			unsigned int messageSize;
		};
		struct txPacketIn
		{
			txPacketIn() : 
				version(0),
				txid(0),
				sender(0),
				recipient(0),
				feeNQT(0),
				amountNQT(0),
				timestamp(0),
				isText(false),
				encrypted(false),
				messageSize(0)
			{
				memset(message, 0, sizeof(char) * MAX_TXDATASIZE);
			}
			uint64 version;
			uint64 txid;
			uint64 sender;
			uint64 recipient;
			uint64 feeNQT;
			uint64 amountNQT;
			uint64 timestamp;

			bool isText;
			bool encrypted; // message is always decrypted. but bool used as toggle for fifo in or as indication for fifo out (that it was encrypted before).
			char message[MAX_TXDATASIZE];
			unsigned int messageSize;
		};

		static txPacketOut initPacketOut(txPacketOut *p = nullptr)
		{
			txPacketOut pt;
			if (p == nullptr)
				p = &pt;
			p->version = 0;
			p->recipient = 0;
			p->isText = false;
			p->encrypted = false;
			std::memset(&(p->message[0]), 0, MAX_TXDATASIZE);
			p->messageSize = 0;

			return *p;
		}
		static txPacketIn initPacketIn(txPacketIn *p = nullptr)
		{
			txPacketIn pt;
			if (p == nullptr)
				p = &pt;
			p->version = 0;
			p->txid = 0;
			p->sender = 0;
			p->recipient = 0;
			p->feeNQT = 0;
			p->amountNQT = 0;
			p->timestamp = 0;

			p->isText = false;
			p->encrypted = false;
			std::memset(&(p->message[0]), 0, MAX_TXDATASIZE);
			p->messageSize = 0;

			return *p;
		}

		template <class packetType>
		class PacketFifo
		{
		public:
			PacketFifo() : packetFifo(1024)
			{
			}
			int addToFifo(const packetType* someData, int numItems)
			{
				int start1, size1, start2, size2;
				packetFifo.prepareToWrite(numItems, start1, size1, start2, size2);
				if (size1 > 0)
					memcpy(&myBuffer[start1], someData, size1 * sizeof(packetType));
				if (size2 > 0)
					memcpy(&myBuffer[start2], &(someData[size1]), size2 * sizeof(packetType));
				packetFifo.finishedWrite(size1 + size2);
				return (size1 + size2);
			}
			int readFromFifo(packetType* someData, int numItems, bool peek = false)
			{
				int start1, size1, start2, size2;
				packetFifo.prepareToRead(numItems, start1, size1, start2, size2);
				if (size1 > 0 && someData)
					memcpy(someData, &myBuffer[start1], size1 * sizeof(packetType));
				if (size2 > 0 && someData)
					memcpy(&(someData[size1]), &myBuffer[start2], size2 * sizeof(packetType));
				if (!peek)
					packetFifo.finishedRead(size1 + size2);
				return (size1 + size2);
			}
			AbstractFifo packetFifo;
		private:
			packetType myBuffer[1024];
		};

		PacketFifo<txPacketOut>& getFifoOut() { return fifoOut; };
		PacketFifo<txPacketIn>& getFifoInData() { return fifoInData; };
		PacketFifo<txPacketIn>& getFifoInText() { return fifoInText; };

	private:
		void UpdateBalance();

		void GetActive(uint64 &currActiveIn, uint64 &maxActiveIn);
		float GetPressureIndicator(int &slot);

		int SendBlockerTransactions(const bool slot_consensus);
		int SendBlocker(const int slotNr, const int blocks);
		void SendFifo(const bool slot_consensus, const bool blocking, const int64 mempoolSize, const int64 activeStake);

		void SetCurrActive(uint64 currActiveIn);
		void SetMaxActive(uint64 maxActiveIn);

		void ProcessUTids(var unconfirmedTransactionIds, int &activeStake, int &prevNumUTs, Array<uint64> &ut_ids, Array<uint64> &ut_fees);
		void PredictSlotAllocation(Array<uint64> &ut_ids, Array<uint64> &ut_fees);
		void ForgeCheck(bool &slot_consensus);

		void CalculateSlotAllocation(Array<uint64> &ut_ids, Array<uint64> &ut_fees);
		void CalculateSlotPressures(const bool slot_consensus);
		void SetPressureIndicator(int slot, float pressure);
				
		Array<txPacketIn> sendEncryptedMessages;
		PacketFifo<txPacketOut> fifoOut;
		PacketFifo<txPacketIn> fifoInData;
		PacketFifo<txPacketIn> fifoInText;

		CriticalSection runLock;
		CriticalSection settingLock;

		StringArray messageTxIds;
		StringArray listenPorts;
		StringArray ignorePorts;
		
		HashMap<int64, String> txMap;
		int64 prevUTtc;
		uint64 currActive;
		uint64 maxActive;
		uint64 balance;
		uint64 poll_interval_ms;

		String error;
		String errorDescription;
		String newBlockContentsHex;

		bool pause;
		bool forceBlock;
		bool nodeSupportsFullHashStore;
		int socketErrorCount;
		int hold_multiplier;

		bool justDonateToDev;

		// block mode
		CriticalSection pressureLock;
		StringArray blockerTxIds;
		Array<uint64> slotUT_id[MAX_FEE_SLOTS];
		Array<uint64> slotUT_fee[MAX_FEE_SLOTS];
		bool useCustodyMode;
		bool lowFunds;
		uint64 blockFeesNQT;
		int currBlockSlot;
		int slotIsblocked[MAX_FEE_SLOTS];
		int slotQueue[MAX_FEE_SLOTS];
		float pressure;
		int pressureSlot;
		int currBlockingLength;
		int maxSlotHeight;
	};

	String CreateAsset();
	String FillOrderBook(uint64 step);

	void OpenSocket();
	void CloseSocket();
	bool isSocketOpen() { return socketThread.isThreadRunning();  };
	
	void SetPause(const bool pause);
	bool SetBlock(const int maxSlotHeight = 10, const int slotHeight = 3);
	void SetForceBlock(const bool force);

	void SetUseCustody(const bool useCustodyMode);
	bool GetUseCustody();

	void GetStatus(bool &blocking, int &slot, float &percent, uint64 &currActiveIn, uint64 &maxActiveIn);

	int GetMaxSlotHeight();
	void SetMaxSlotHeight(const int slotHeight);
	int GetBlockingLength();
	void SetBlockingLength(const int slotDepth);
	int GetHoldSize();
	void SetHoldSize(const int newHoldMultiplier);

	bool Listen(const bool on, String port);
	StringArray GetListenPorts();
	bool Ignore(const bool on, String port);
	StringArray GetIgnorePorts();

	void SocketSendMessage(const String address, const String txt, const bool encrypted);
	void SocketStreamData(const String address, const char type, const String nameOfFileOrStream, const void *data, const int bytes, const bool encrypted);
	void SocketSendFile(const String address, const File f, const bool encrypted);

	bool SocketPollMessage(BurstSocketThread::txPacketIn &p);
	void GetDataStreamPacket(MemoryBlock hash, char &type, BurstSocketThread::txPacketIn &tx, String &name, MemoryBlock &data);
	
private:
	BurstSocketThread socketThread;

	struct MTX
	{
		String id;
		StringArray addresses;
		String amount;
		String fee;
		String dl;
	};

	// socket data
	static bool GetHeader(MemoryBlock header, char &type, uint32 &offset, uint32 &part_size, uint32 &total_size, MemoryBlock &hash, String &name, MemoryBlock &rawData);
	MemoryBlock MakeHeader(char type, uint32 offset, uint32 part_size, uint32 total_size, MemoryBlock hash, String name = String::empty);
	bool ProcessDataTx(BurstSocketThread::txPacketIn &tx, char &type, uint32 &offset, uint32 &part_size, uint32 &total_size, MemoryBlock &hash, String &name, MemoryBlock &rawData);

	bool SocketPollData(BurstSocketThread::txPacketIn &p);
	void AddDataStreamCache(MemoryBlock hash, const char type, const BurstSocketThread::txPacketIn tx, const String name, const MemoryBlock data);
	void ClearDataStreamCache(MemoryBlock hash = MemoryBlock());
	
	Array<MemoryBlock> socketDataTimeline;
	HashMap<MemoryBlock, char> socketDataType;
	HashMap<MemoryBlock, BurstSocketThread::txPacketIn> socketDataTx;
	HashMap<MemoryBlock, String> socketDataNames;
	HashMap<MemoryBlock, MemoryBlock> socketDatas;	
};

#endif // BurstSocket
