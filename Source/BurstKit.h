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

#define US_NET "https://wallet1.burst-team.us:2083/"

#define FEE_QUANT 735000

class BurstKit
{
public:
	BurstKit(String hostUrl = US_NET, String passPhrase = String::empty);
	~BurstKit();

	int GetBurstKitVersionNumber();
	const char *GetBurstKitVersionString();

	String GetLastError(int &errorCode);
	void SetError(int errorCode, String msg);

	void SetNode(String hostUrl);
	String GetNode();
	
	void SetSecretPhrase(const String passphrase, const unsigned int index = 0);
	void GetWallet(unsigned int index, String &pubKey_hex, String &pubKey_b64, String &reedSolomon, String &addressID);
	
	uint64 GetBalance(const unsigned int index = -1);
	uint64 GetWalletBalance(const unsigned int index);
	String GetRecipientPublicKey(const String recipient);

	void SetAccount(const String account);
	void SetAccountRS(const String reedSolomonIn);
	void SetAccountID(const String accountID);
	String GetAccountRS(const unsigned int index = 0);
	String GetAccountID(const unsigned int index = 0);
	String GetJSONvalue(const String json, const String key);

	// API
	String broadcastTransaction( // Broadcast a transaction to the network. POST only.
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
		String numberOfConfirmations = String::empty); // is the required number of confirmations per transaction(optional)
	String getAccountPublicKey( // Get the public key associated with an account ID. 
		String account); // is the account ID
	String getAccountTransactions( // Get the transactions associated with an account in reverse block timestamp order. 
		String account, // is the account ID
		String timestamp = String::empty, // is the earliest block(in seconds since the genesis block) to retrieve(optional)
		String type = String::empty, // is the type of transactions to retrieve(optional)
		String subtype = String::empty, // is the subtype of transactions to retrieve(optional)
		String firstIndex = String::empty, // is the a zero - based index to the first transaction ID to retrieve(optional)
		String lastIndex = String::empty, // is the a zero - based index to the last transaction ID to retrieve(optional)
		String numberOfConfirmations = String::empty); // is the required number of confirmations per transaction(optional)

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
		String account); // is the account ID(optional)		
	String parseTransaction( // Get a transaction object given a (signed or unsigned) transaction bytecode, or re-parse a transaction object. Verify the signature.
		String transactionBytes, // is the signed or unsigned bytecode of the transaction(optional)
		String transactionJSON = String::empty); // is the transaction object(optional if transactionBytes is included)		
	String getTransactionBytes( // Get the bytecode of a transaction. 
		String transaction); // is the transaction ID		

	String sendMoney( // Send individual amounts of BURST to up to 64 recipients. POST only. Refer to Create Transaction Request for common parameters. 
		String recipient,
		String amountNQT,
		String feeNQT,
		String deadlineMinutes,
		String referencedTransactionFullHash,
		bool broadcast = true,
		const unsigned int index = 0);
	String sendMoneyWithMessage(
		String recipient,
		String amountNQT,
		String feeNQT,
		String deadlineMinutes,
		String message,
		bool encrpyted,
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
		String active, // is true for active(not NON_CONNECTED) peers only(optional, if true overrides state)
		String state); // is the state of the peers, one of NON_CONNECTED, CONNECTED, or DISCONNECTED(optional)
	String getTime(); // Get the current time. 
	String getState( // Get the state of the server node and network. 
		String includeCounts); // is true if the fields beginning with numberOf... are to be included(optional); password protected like the Debug Operations if true.

	String longConvert( // Converts an ID to the signed long integer representation used internally. 
		String id); // is the numerical ID, in decimal form but equivalent to an 8-byte unsigned integer as produced by SHA-256 hashing
	String rsConvert( // Get both the Reed-Solomon account address and the account number given an account ID. 
		String account); // is the account ID (either RS address or number)

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
	String host;

	int DetermineAccountUnit(String str);
	String ensureAccountAlias(String str);
	String ensureAccountRS(String str);
	String ensureAccountID(String str);
	String getAccountAliases(String str);

	String GetSecretPhraseString(const unsigned int index = 0);
private:
	MemoryBlock GetSecretPhrase(const unsigned int index = 0);
	void CalcPubKeys(const MemoryBlock pw, String &pubKey_hex, String &pubKey_b64, String &addressID);
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
	String GetUrlStr(const String url);

	StringArray toReedSolomonEncoding(MemoryBlock mb); // TODO implement like https://github.com/aprock/BurstKit/blob/master/BurstKit/Utils/BurstAddress.swift
	MemoryBlock fromReedSolomonEncoding(StringArray encoded_strings);

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

cancelAskOrder
cancelBidOrder
createATProgram

escrowSign
generateToken
getAT
getATDetails
getATIds
getATLong
getAccountATs
getAccountCurrentAskOrderIds
getAccountCurrentAskOrders
getAccountCurrentBidOrderIds
getAccountCurrentBidOrders
getAccountEscrowTransactions
getAccountLessors
getAccountSubscriptions
getAllAssets
getAllOpenAskOrders
getAllOpenBidOrders
getAllTrades
getAskOrder
getAskOrderIds
getAskOrders
getAsset
getAssetAccounts
getAssetIds
getAssetTransfers
getAssets
getAssetsByIssuer
getBidOrder
getBidOrderIds
getBidOrders

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

getEscrowTransaction

getSubscription
getSubscriptionsToAccount

getTrades
issueAsset
placeAskOrder
placeBidOrder

sendMoneyEscrow
sendMoneySubscription
submitNonce
subscriptionCancel
transferAsset

getAccountsWithRewardRecipient();

*/