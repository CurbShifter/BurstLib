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

#ifndef BurstKit_H_INCLUDED
#define BurstKit_H_INCLUDED

#include "./crypto/Crypto.hpp"
#include "./crypto/BurstAddress.h"
#if BURSTKIT_SHABAL == 1
#include "./crypto/hash/simd_shabal.h"
#include "./crypto/hash/sph_shabal.h"
#endif
#include "../JuceLibraryCode/JuceHeader.h"

using namespace juce;

#define US_NET "wallet.burst-team.us:2083"

#define FEE_QUANT 735000
#define BURSTCOIN_GENESIS_EPOCH 1407722400
#define MAX_TXDATASIZE (1000)
#define MAX_FEE_SLOTS (1020)
#define MAX_TX_PER_SLOT 360

class BurstKit
{
public:
	BurstKit(String hostUrl = String::empty, String passPhrase = String::empty);
	~BurstKit();

	bool IsOnTestNet();

	int GetBurstKitVersionNumber();
	const char *GetBurstKitVersionString();

	String GetLastError(int &errorCode);
	void SetError(int errorCode, String msg);

	void SetNode(const String hostUrl, const bool allowNonSSL = true);
	String GetNode();
	void SetForceSSL_TSL(const bool force);
	bool GetForceSSL_TSL();

	void SetSecretPhrase(const String passphrase, const unsigned int index = 0);
	void GetWallet(unsigned int index, String &pubKey_hex, String &pubKey_b64, String &reedSolomon, String &addressID);

	uint64 GetBalance(const unsigned int index = -1);
	uint64 GetWalletBalance(const unsigned int index);
	String GetRecipientPublicKey(const String recipient);
	uint64 GetAssetBalance(const String assetID, const unsigned int index = -1);

	void SetAccount(const String account);
	void SetAccountRS(const String reedSolomonIn);
	void SetAccountID(const String accountID);
	String GetAccountPublicKey(const unsigned int index = 0);
	String GetAccountRS(const unsigned int index = 0);
	String GetAccountID(const unsigned int index = 0);
	virtual String GetJSONvalue(const String json, const String key);

	// API
	virtual String broadcastTransaction( // Broadcast a transaction to the network. POST only.
		String signedTransactionBytesStr); // is the bytecode of a signed transaction
	String calculateFullHash(
		String unsignedTransactionBytes, // is the unsigned bytes of a transaction
		String signatureHash); // is the SHA-256 hash of the transaction signature
	String decryptFrom( // Decrypt an AES-encrypted message. 
		String account, // is the account ID of the recipient
		String data, // is the AES-encrypted data
		String nonce, // is the unique nonce associated with the encrypted data
		String decryptedMessageIsText, // is false if the decrypted message is a hex string, true if the decrypted message is text
		String secretPhrase = String::empty, // is the secret passphrase of the recipient
		const unsigned int index = 0); // is private chain index
	String encryptTo( // Encrypt a message using AES without sending it.
		String &nonce,
		String recipient, // is the account ID of the recipient.
		String messageToEncrypt, // is either UTF - 8 text or a string of hex digits to be compressed and converted into a 1000 byte maximum bytecode then encrypted using AES
		String messageToEncryptIsText, // is false if the message to encrypt is a hex string, true if the encrypted message is text
		String secretPhrase = String::empty, // is the secret passphrase of the recipient
		const unsigned int index = 0); // is private chain index
	String getAccount( // Get account information given an account ID.
		String account_RS_or_ID); // is the account ID (or ReedSolomon, detected by BURST- Prefix and non numerical chars)
	String getAccountBlockIds( // Get the block IDs of all blocks forged (generated) by an account in reverse block height order. 
		String account, // is the account ID of the recipient
		String timestamp = String::empty, // is the earliest block(in seconds since the genesis block) to retrieve(optional)
		String firstIndex = String::empty, // is the zero-based index to the first block to retrieve (optional)
		String lastIndex = String::empty); // is the zero-based index to the last block to retrieve (optional)
	String getAccountBlocks( // Get all blocks forged (generated) by an account in reverse block height order. 
		String account, // is the account ID of the recipient
		String timestamp = String::empty, // is the earliest block(in seconds since the genesis block) to retrieve (optional)
		String firstIndex = String::empty, // is the zero - based index to the first block to retrieve (optional)
		String lastIndex = String::empty, // is the zero - based index to the last block to retrieve (optional)
		String includeTransactions = String::empty); // is the true to retrieve transaction details, otherwise only transaction IDs are retrieved (optional)

	String getAccountId( // Get an account ID given public key. (or a secret passphrase (POST only))
		String pubKey_64HEX); // is the public key of the account
	String getAccountTransactionIds( // Get the transaction IDs associated with an account in reverse block timestamp order. Note: Refer to Get Constants for definitions of types and subtypes
		String account, // is the account ID
		String timestamp = String::empty, // is the earliest block(in seconds since the genesis block) to retrieve(optional)
		String type = String::empty, // is the type of transactions to retrieve(optional)
		String subtype = String::empty, // is the subtype of transactions to retrieve(optional)
		String firstIndex = String::empty, // is the a zero - based index to the first transaction ID to retrieve(optional)
		String lastIndex = String::empty, // is the a zero - based index to the last transaction ID to retrieve(optional)
		String numberOfConfirmations = String::empty, // is the required number of confirmations per transaction(optional)
		bool includeIndirect = false); // includeIndirect, to include the received multiouts

	String getAccountPublicKey( // Get the public key associated with an account ID. 
		String account); // is the account ID
	String getAccountTransactions( // Get the transactions associated with an account in reverse block timestamp order. 
		String account, // is the account ID
		String timestamp = String::empty, // is the earliest block(in seconds since the genesis block) to retrieve(optional)
		String type = String::empty, // is the type of transactions to retrieve(optional)
		String subtype = String::empty, // is the subtype of transactions to retrieve(optional)
		String firstIndex = String::empty, // is the a zero - based index to the first transaction ID to retrieve(optional)
		String lastIndex = String::empty, // is the a zero - based index to the last transaction ID to retrieve(optional)
		String numberOfConfirmations = String::empty, // is the required number of confirmations per transaction(optional)
		bool includeIndirect = false); // includeIndirect, to include the received multiouts

	String setAccountInfo( // Set account information. POST only. Refer to Create Transaction Request for common parameters. 
		String name, // is the name to associate with the account
		String description, // is the description to associate with the account
		String feeNQT,
		String deadlineMinutes,
		bool broadcast = true,
		const unsigned int index = 0);

	String getAlias( // Get information about a given alias. 
		String alias, // is the alias ID (optional)
		String aliasName = String::empty); // is the name of the alias (optional if alias provided)
	String setAlias( // Create and/or assign an alias. POST only. Refer to Create Transaction Request for common parameters. 
		String aliasName, // is the alias name
		String aliasURI, // is the alias URI(e.g.http://www.google.com/)
		String feeNQT,
		String deadlineMinutes,
		bool broadcast = true,
		const unsigned int index = 0);
	String getAliases( // Get information on aliases owned by a given account in alias name order. 
		String account, // is the ID of the account that owns the aliases
		String timestamp = String::empty, // is the earliest creation time(in seconds since the genesis block) of the aliases(optional)
		String firstIndex = String::empty, // is the zero - based index to the first alias to retrieve(optional)
		String lastIndex = String::empty); // is the zero - based index to the last alias to retrieve(optional)
	String buyAlias( // Buy an alias. POST only. Refer to Create Transaction Request for common parameters. 
		String alias, // is the ID of the alias (optional)
		String aliasName, // is the alias name (optional if alias provided)
		String amountNQT, // is the amount (in NQT) offered for an alias for sale (buyAlias only)
		String recipient, // is the account ID of the recipient (only required for sellAlias and only if there is a designated recipient)
		String feeNQT,
		String deadlineMinutes,
		bool broadcast = true,
		const unsigned int index = 0);
	String sellAlias( // Sell an alias. POST only. Refer to Create Transaction Request for common parameters. 
		String alias, // is the ID of the alias(optional)
		String aliasName, // is the alias name(optional if alias provided)
		String priceNQT, // is the asking price(in NQT) of the alias(sellAlias only)
		String recipient, // is the account ID of the recipient(only required for sellAlias and only if there is a designated recipient)
		String feeNQT,
		String deadlineMinutes,
		bool broadcast = true,
		const unsigned int index = 0);

	String getBalance( // Get the balance of an account. 
		String account, // is the account ID
		//	String includeEffectiveBalance = String::empty, // is true to include effectiveBalanceNXT and guaranteedBalanceNQT(optional
		String height = String::empty, // is the height to retrieve account balance for, if still available(optional)
		String requireBlock = String::empty, // is the block ID of a block that must be present in the blockchain during execution(optional)
		String requireLastBlock = String::empty); // is the block ID of a block that must be last in the blockchain during execution(optional)
	String getGuaranteedBalance( // Get the balance of an account that is confirmed at least a specified number of times. 
		String account, // is the account ID
		String numberOfConfirmations); // is the minimum number of confirmations for a transaction to be included in the guaranteed balance(optional, if omitted or zero then minimally confirmed transactions are included)

	String getTransaction( // Get a transaction object given a transaction ID. 
		String transactionID, // a transaction ID. 
		String fullHash = String::empty); //  is the full hash of the transaction (optional if transaction ID is provided)	
	String getEscrowTransaction( // Get a transaction object given a transaction ID. 
		String transactionID); // a transaction ID. 
	String getUnconfirmedTransactionsIds( // Get a list of unconfirmed transaction IDs associated with an account. 
		String account = String::empty); // is the account ID(optional)
	String getUnconfirmedTransactions( // Get a list of unconfirmed transactions associated with an account. 
		String account = String::empty); // is the account ID(optional)		
	String parseTransaction( // Get a transaction object given a (signed or unsigned) transaction bytecode, or re-parse a transaction object. Verify the signature.
		String transactionBytes, // is the signed or unsigned bytecode of the transaction(optional)
		String transactionJSON = String::empty); // is the transaction object(optional if transactionBytes is included)		
	String getTransactionBytes( // Get the bytecode of a transaction. 
		String transaction); // is the transaction ID		

	String BurstKit::createMessageArgs(
		const String message,
		const bool isText = true, // is false if the message to encrypt is a hex string, true if the encrypted message is text
		const bool encrpyted = false,
		const String recipient = String::empty);

	String sendMoney( // Send individual amounts of BURST to up to 64 recipients. POST only. Refer to Create Transaction Request for common parameters. 
		String recipient,
		String amountNQT,
		String feeNQT,
		String deadlineMinutes,
		String referencedTransactionFullHash,
		bool broadcast = true,
		const unsigned int index = 0);
	String sendMoneyWithMessage(
		const String recipient,
		const String amountNQT,
		const String feeNQT,
		const String deadlineMinutes,
		const String message = String::empty,
		const bool messageIsText = true,
		const bool encrpyted = false,
		String referencedTransactionFullHash = String::empty,
		bool broadcast = true,
		const unsigned int index = 0);
	String sendMoneyMulti( // Send the same amount of BURST to up to 128 recipients. POST only. Refer to Create Transaction Request for common parameters. 
		StringArray recipients, // is the account ID of the recipients. The recipients string is <numid1>:<amount1>;<numid2>:<amount2>;<numidN>:<amountN>
		StringArray amountsNQT,
		String feeNQT,
		String deadlineMinutes,
		bool broadcast = true,
		const unsigned int index = 0);
	String sendMoneyMultiSame( // Send the same amount of BURST to up to 128 recipients. POST only. Refer to Create Transaction Request for common parameters. 
		StringArray recipients, // is the account ID of the recipients. The recipients string is <numid1>;<numid2>;<numidN>
		String amountNQT,
		String feeNQT,
		String deadlineMinutes,
		bool broadcast = true,
		const unsigned int index = 0);

	String readMessage( // Get a message given a transaction ID.
		String transaction); // is the transaction ID of the message	
	String sendMessage( // Create an Arbitrary Message transaction. POST only. Refer to Create Transaction Request for common parameters. 
		// Note: Any combination (including none or all) of the three options plain message, messageToEncrypt, and messageToEncryptToSelf will be included in the transaction. 
		// However, one and only one prunable message may be included in a single transaction if there is not already a message of the same type (either plain or encrypted). 
		String recipient, // is the account ID of the recipient.
		String message, // is either UTF - 8 text or a string of hex digits(perhaps previously encoded using an arbitrary algorithm) to be converted into a bytecode with a maximum length of one kilobyte(optional)
		String messageIsText, // is false if the message is a hex string, otherwise the message is text (optional)
		String messageToEncrypt, // is either UTF-8 text or a string of hex digits to be compressed and converted into a bytecode with a maximum length of one kilobyte, then encrypted using AES (optional)
		String messageToEncryptIsText, // is alse if the message to encrypt is a hex string, otherwise the message to encrypt is text (optional)
		String encryptedMessageData, // is already encrypted data which overrides messageToEncrypt if provided (optional)
		String encryptedMessageNonce, // is a unique 32-byte number which cannot be reused (optional unless encryptedMessageData is provided)
		String messageToEncryptToSelf, // is either UTF-8 text or a string of hex digits to be compressed and converted into a one kilobyte maximum bytecode then encrypted with AES, then sent to the sending account (optional)
		String messageToEncryptToSelfIsText, // is false if the message to self-encrypt is a hex string, otherwise the message to encrypt is text (optional)
		String encryptToSelfMessageData, // is already encrypted data which overrides messageToEncryptToSelf if provided (optional)
		String encryptToSelfMessageNonce, // is a unique 32-byte number which cannot be reused (optional unless encryptToSelfMessageData is provided)
		String recipientPublicKey, // is the public key of the receiving account (optional, enhances security of a new account)
		String referencedTransactionFullHash, // optional referencedTransactionFullHash parameter which creates a chained transaction, meaning that the new transaction cannot be confirmed unless the referenced transaction is also confirmed. This feature allows a simple way of transaction escrow. 2 BURST for transactions that make use of referencedTransactionFullHash property when creating a new transaction.
		String feeNQT,
		String deadlineMinutes,
		bool broadcast = true,
		const unsigned int index = 0);

	String suggestFee(); // Get a cheap, standard, and priority fee. 

	String getRewardRecipient(
		String account); // is the account ID.
	String setRewardRecipient( // Set Reward recipient is used to set the reward recipient of a given account. 
		String recipient, // is the account ID of the recipient.
		String feeNQT,
		String deadlineMinutes,
		bool broadcast = true,
		const unsigned int index = 0);

	String getBlock( // Note: block overrides height which overrides timestamp.
		String block, // is the block ID(optional)
		String height = String::empty, // is the block height(optional if block provided)
		String timestamp = String::empty, // is the timestamp(in seconds since the genesis block) of the block(optional if height provided)
		String includeTransactions = String::empty); // is true to include transaction details(optional)

	String getBlockId( // Get a block ID given a block height.
		String height); // is the block height

	String getBlockchainStatus(); // Get the blockchain status. 

	String getBlocks( // Get blocks from the blockchain in reverse block height order.
		String firstIndex, // is the first block to retrieve(optional, default is zero or the last block on the blockchain)
		String lastIndex, // is the last block to retrieve(optional, default is firstIndex + 99)
		String includeTransactions); // is true to include transaction details(optional)
	String getConstants(); // Get all defined constants. 
	String getECBlock( // Get Economic Cluster block data.  Note: If timestamp is more than 15 seconds before the timestamp of the last block on the blockchain, errorCode 4 is returned.
		String timestamp); // is the timestamp(in seconds since the genesis block) of the EC block(optional, default (or zero) is the current timestamp)

	String getMiningInfo(); // Get Mining Info
	String getMyInfo(); // Get hostname and address of the requesting node. 
	String getPeer( // Get information about a given peer. 
		String peer); // is the IP address or domain name of the peer(plus optional port)
	String getPeers( // Get a list of peer IP addresses.  Note: If neither active nor state is specified, all known peers are retrieved. 
		const String active = String::empty, // is true for active(not NON_CONNECTED) peers only(optional, if true overrides state)
		const String state = String::empty); // is the state of the peers, one of NON_CONNECTED, CONNECTED, or DISCONNECTED(optional)
	String getTime(); // Get the current time. 
	String getState( // Get the state of the server node and network. 
		const String includeCounts); // is true if the fields beginning with numberOf... are to be included(optional); password protected like the Debug Operations if true.

	String getAsset( // Get asset information given an asset ID. 
		const String asset); // is the ID of the asset

	String transferAsset( // Get the state of the server node and network. 
		const String recipient, // is the recipient account ID
		const String asset, // is the ID of the asset being transferred
		const String quantityQNT, // is the amount(in QNT) of the asset being transferred
		const String feeNQT,
		const String deadlineMinutes,
		const bool broadcast,
		const unsigned int index);

	String transferAsset( // Get the state of the server node and network. 
		const String recipient, // is the recipient account ID
		const String asset, // is the ID of the asset being transferred
		const String quantityQNT, // is the amount(in QNT) of the asset being transferred,
		const String feeNQT,
		const String deadlineMinutes,
		const String message,
		const bool encrypted,
		const String referencedTransactionFullHash,
		const bool broadcast,
		const unsigned int index);

	String getAllAssets(
		const String firstIndex = String::empty, // is a zero - based index to the first asset to retrieve(optional)
		const String lastIndex = String::empty, // is a zero - based index to the last asset to retrieve(optional)
		const String includeCounts = String::empty, // is true if the fields beginning with numberOf... are to be included(optional)
		const String requireBlock = String::empty, // is the block ID of a block that must be present in the blockchain during execution(optional)
		const String requireLastBlock = String::empty); // is the block ID of a block that must be last in the blockchain during execution(optional)

	String getAssets(
		const String assets, // is one the multiple asset IDs, comma separated
		const String includeCounts = String::empty, // is true if the fields beginning with numberOf... are to be included(optional)
		const String requireBlock = String::empty, // is the block ID of a block that must be present in the blockchain during execution(optional)
		const String requireLastBlock = String::empty); // is the block ID of a block that must be last in the blockchain during execution(optional)

	String getAssetTransfers(
		const String asset,// is the asset ID(optional),
		const String account = String::empty,// is the account ID(optional if asset provided),
		const String timestamp = String::empty,// is the earliest transfer(in seconds since the genesis block) to retrieve(optional, does not apply to expected transfers),
		const String firstIndex = String::empty,// is a zero - based index to the first transfer to retrieve(optional, does not apply to expected transfers),
		const String lastIndex = String::empty,// is a zero - based index to the last transfer to retrieve(optional, does not apply to expected transfers),
		const String includeAssetInfo = String::empty,// is true if the decimals and name fields are to be included(optional, does not apply to expected transfers),
		const String requireBlock = String::empty,// is the block ID of a block that must be present in the blockchain during execution(optional),
		const String requireLastBlock = String::empty);// is the block ID of a block that must be last in the blockchain during execution(optional),


	String issueAsset( // Create an asset on the exchange
		const String name, // is the name of the asset
		const String description, // is the url - encoded description of the asset in UTF - 8 with a maximum length of 1000 bytes(optional)
		const String quantityQNT, // is the total amount(in QNT) of the asset in existence
		const String decimals, // is the number of decimal places used by the asset(optional, zero default)
		const String feeNQT,
		const String deadlineMinutes,
		bool broadcast,
		const unsigned int index);

	String getAssetAccounts( // Get trades associated with a given asset and/or account in reverse block height order. 
		const String asset, // is the asset ID
		const String height = String::empty, // is the height of the blockchain to determine the accounts(optional, default is last block)
		const String firstIndex = String::empty, // is a zero - based index to the first account to retrieve(optional)
		const String lastIndex = String::empty); // is a zero - based index to the last account to retrieve(optional)

	String getAssetsByIssuer( // Get trades associated with a given asset and/or account in reverse block height order. 
		const String account, // is the account ID
		const String firstIndex = String::empty, // is a zero - based index to the first trade to retrieve(optional)
		const String lastIndex = String::empty); // is a zero - based index to the last trade to retrieve(optional)

	String getTrades( // Get trades associated with a given asset and/or account in reverse block height order. 
		const String asset, // is the asset ID
		const String account = String::empty, // is the account ID(optional if asset provided)
		const String firstIndex = String::empty, // is a zero - based index to the first trade to retrieve(optional)
		const String lastIndex = String::empty, // is a zero - based index to the last trade to retrieve(optional)
		const String includeAssetInfo = String::empty); // is true if the decimals and name fields are to be included(optional)

	String placeAskOrder( // Place an asset order
		const String asset, // is the asset ID of the asset being ordered
		const String quantityQNT, // is the amount(in QNT) of the asset being ordered
		const String priceNQT, // is the bid / ask price(in NQT)
		const String feeNQT,
		const String deadlineMinutes = "1440",
		const String message = String::empty,
		const bool messageIsText = true,
		bool broadcast = true,
		const unsigned int index = 0);

	String placeBidOrder( // Place an asset order
		const String asset, // is the asset ID of the asset being ordered
		const String quantityQNT, // is the amount(in QNT) of the asset being ordered
		const String priceNQT, // is the bid / ask price(in NQT)
		const String feeNQT,
		const String deadlineMinutes = "1440",
		const String message = String::empty,
		const bool messageIsText = true,
		bool broadcast = true,
		const unsigned int index = 0);

	String getAskOrder(const String order);
	String getAskOrderIds(
		const String asset,
		const String firstIndex = String::empty,
		const String lastIndex = String::empty);
	String getAskOrders(
		const String asset,
		const String firstIndex = String::empty,
		const String lastIndex = String::empty);
	String getAccountCurrentAskOrderIds(
		const String account,
		const String asset = String::empty,
		const String firstIndex = String::empty,
		const String lastIndex = String::empty);
	String getAccountCurrentAskOrders(
		const String account,
		const String asset = String::empty,
		const String firstIndex = String::empty,
		const String lastIndex = String::empty);
	String getAllOpenAskOrders(
		const String firstIndex = String::empty,
		const String lastIndex = String::empty);
	String cancelAskOrder(
		const String order,
		const String feeNQT,
		const String deadlineMinutes,
		bool broadcast,
		const unsigned int index);

	String getBidOrder(const String order);
	String getBidOrderIds(
		const String asset,
		const String firstIndex = String::empty,
		const String lastIndex = String::empty);
	String getBidOrders(
		const String asset,
		const String firstIndex = String::empty,
		const String lastIndex = String::empty);
	String getAccountCurrentBidOrderIds(
		const String account,
		const String asset = String::empty,
		const String firstIndex = String::empty,
		const String lastIndex = String::empty);
	String getAccountCurrentBidOrders(
		const String account,
		const String asset = String::empty,
		const String firstIndex = String::empty,
		const String lastIndex = String::empty);
	String getAllOpenBidOrders(
		const String firstIndex = String::empty,
		const String lastIndex = String::empty);
	String cancelBidOrder(
		const String order,
		const String feeNQT,
		const String deadlineMinutes,
		bool broadcast,
		const unsigned int index);

	String longConvert( // Converts an ID to the signed long integer representation used internally. 
		String id); // is the numerical ID, in decimal form but equivalent to an 8-byte unsigned integer as produced by SHA-256 hashing
	String rsConvert( // Get both the Reed-Solomon account address and the account number given an account ID. 
		String account); // is the account ID (either RS address or number)

	String createATProgram(
		const String name,
		const String description,
		const String creationBytes,
		const String code,
		const String data,
		const String dpages,
		const String cspages,
		const String uspages,
		const String minActivationAmountNQT,
		const String feeNQT,
		const String deadlineMinutes,
		bool broadcast,
		const unsigned int index);
	String getAT(
		const String at);
	String getATDetails( // same as getAT ?
		const String at);
	String getATIds();
	String getATLong(
		const String hexString);
	String getAccountATs(
		const String account);

	String getAllTrades(
		const String timestamp,
		const String firstIndex,
		const String lastIndex,
		const String includeAssetInfo);
	String getAssetIds(
		const String firstIndex,
		const String lastIndex);
	String getAssetTransfers(
		const String asset,
		const String account,
		const String firstIndex,
		const String lastIndex,
		const String includeAssetInfo);


#if BURSTKIT_SHABAL == 1
	bool m_supportSSE;
	bool m_supportSSE2;
	bool m_supportAVX;
	bool m_supportAVX2;

	simd_shabal_context ccInitSIMD;
	Array<simd_shabal_context> ccArraySIMD;

	sph_shabal_context ccInit;
	Array<sph_shabal_context> ccArray;

	Array<bool> ccUse;

	unsigned int Shabal256_ccID();
	void Shabal256_reset(unsigned int ccID);
	void Shabal256_update(unsigned int ccID, void *inbuf, int off = 0, int len = 0);
	void Shabal256_digest(unsigned int ccID, void *dst_32bytes);
#endif
	String toBase64Encoding(MemoryBlock const& bytes_to_encode);
	MemoryBlock fromBase64Encoding(String const& encoded_string);

	uint64 GetUINT64(const String uint64Str);
	MemoryBlock GetUINT64MemoryBlock(const String uint64Str);

	// cached account and host info
	struct localAccountData
	{
		MemoryBlock secretPhrase_enc[2];
		MemoryBlock memNonce[2];
		String accountID;
		String reedSolomon;
		String pubKey_HEX;
		String pubKey_b64;
	};
	localAccountData accountData;
	String protocol;
	String nodeAddress;

	int DetermineAccountUnit(String str);
	String convertToAlias(String str);
	String convertToReedSolomon(String str);
	String convertToAccountID(String str);
	String getAccountAliases(String str, const bool newestFirst = false);

	virtual String GetUrlStr(const String url);
	String GetSecretPhraseString(const unsigned int index = 0);

	void CalcPubKeys(const MemoryBlock pw, String &pubKey_hex, String &pubKey_b64, String &addressID);

	static String GetTransactionTypeText(const int type, const int subtype)
	{
		String tag;
		if (type == 0) //String constantsStr = burstKit.getConstants();
		{
			if (subtype == 0)
				tag = "Ordinary Payment";
			else if (subtype == 1)
				tag = "Multi-out Payment";
			else if (subtype == 2)
				tag = "Multi-out Same Payment";
		}
		else if (type == 1)
		{
			if (subtype == 0)
				tag = "Arbitrary Message";
			else if (subtype == 1)
				tag = "Alias Assignment";
			else if (subtype == 2)
				tag = "Account Info";
			else if (subtype == 1)
				tag = "Alias Sell";
			else if (subtype == 2)
				tag = "Alias Buy";
		}
		else if (type == 2)
		{
			if (subtype == 0)
				tag = "Asset Issuance";
			else if (subtype == 1)
				tag = "Asset Transfer";
			else if (subtype == 2)
				tag = "Ask Order Placement";
			else if (subtype == 3)
				tag = "Bid Order Placement";
			else if (subtype == 4)
				tag = "Ask Order Cancellation";
			else if (subtype == 5)
				tag = "Bid Order Cancellation";
		}
		else if (type == 3)
		{ // "Digital Goods",
			if (subtype == 0)
				tag = "DGS Listing";
			else if (subtype == 1)
				tag = "DGS Delisting";
			else if (subtype == 2)
				tag = "DGS Price Change";
			else if (subtype == 3)
				tag = "DGS Quantity Change";
			else if (subtype == 4)
				tag = "DGS Purchase";
			else if (subtype == 5)
				tag = "DGS Delivery";
			else if (subtype == 6)
				tag = "DGS Feedback";
			else if (subtype == 7)
				tag = "DGS Refund";
		}
		else if (type == 4)
		{ // "Account Control",
			if (subtype == 0)
				tag = "Account Control Effective Balance Leasing"; // NXT
		}
		else if (type == 20)
		{ // "Burst Mining"
			if (subtype == 0)
				tag = "Reward Recipient Assignment";
		}
		else if (type == 20)
		{ // "Advanced Payment";
			if (subtype == 0)
				tag = "Escrow Creation";
			else if (subtype == 1)
				tag = "Escrow Sign";
			else if (subtype == 2)
				tag = "Escrow Result";
			else if (subtype == 3)
				tag = "Subscription Subscribe";
			else if (subtype == 4)
				tag = "Subscription Cancel";
			else if (subtype == 5)
				tag = "Subscription Payment";
		}
		else if (type == 22)
		{ // "Automated Transactions",
			if (subtype == 0)
				tag = "AT Creation";
			else if (subtype == 1)
				tag = "AT Payment";
		}
		return tag;
	}

private:
	MemoryBlock GetSecretPhrase(const unsigned int index = 0);
	String ConvertPubKeyToNumerical(const MemoryBlock pubKey);


	String CreateTX(
		const String url,
		const String feeNQT,
		const String deadlineMinutes,
		const bool broadcast,
		const unsigned int index = 0);
	String GetUnsignedTransactionBytes(String url);
	String TxRequestArgs(
		String publicKey,
		String feeNQT,
		String deadlineMinutes);
	String SignAndBroadcast(const String unsignedTransactionBytesStr, const unsigned int index);
	String Sign(const String unsignedTransactionBytesStr, const unsigned int index);

	var GetUrlJson(const String urlStr, String *json = nullptr);

	StringArray toReedSolomonEncoding(MemoryBlock mb); // TODO implement like https://github.com/aprock/BurstKit/blob/master/BurstKit/Utils/BurstAddress.swift
	MemoryBlock fromReedSolomonEncoding(StringArray encoded_strings);

	bool forceSSL;
	Crypto crypto;
	const char *burstKitVersionString;
	int lastErrorCode;
	String lastErrorDescription;

	MemoryBlock memKey[2];
};

#endif

/*
To be implemented
=
generateToken

getAccountEscrowTransactions
getEscrowTransaction

sendMoneyEscrow
escrowSign

getAccountLessors
getAccountSubscriptions
getSubscription
getSubscriptionsToAccount
sendMoneySubscription
subscriptionCancel

dgsDelisting
dgsDelivery
dgsFeedback
dgsListing
dgsPriceChange
dgsPurchase
dgsQuantityChange
dgsRefund
getDGSGood
getDGSGoods
getDGSPendingPurchases
getDGSPurchase
getDGSPurchases

submitNonce
getAccountsWithRewardRecipient

*/