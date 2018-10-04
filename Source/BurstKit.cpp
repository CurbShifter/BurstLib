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

#include "BurstKit.h"
#include "Version.h"

using namespace juce;

BurstKit::BurstKit(String hostUrl, String passPhrase) : host(hostUrl), burstKitVersionString(SVNRevision)
{
	if (passPhrase.isNotEmpty())
		SetSecretPhrase(passPhrase);
}

BurstKit::~BurstKit()
{
}

int BurstKit::GetBurstKitVersionNumber()
{
	return SVNRevisionN;
}

const char* BurstKit::GetBurstKitVersionString()
{
	return burstKitVersionString;
}

void BurstKit::SetNode(String hostUrl)
{
	host = hostUrl;
}

String BurstKit::GetNode()
{
	return host;
}

String BurstKit::GetAccountRS()
{
	return accountData.reedSolomon;
}

String BurstKit::GetAccountID()
{
	return accountData.accountID;
}

String BurstKit::GetLastError(int &errorCode)
{
	errorCode = lastErrorCode;
	return lastErrorDescription;
};

// LOCAL ACCOUNT ---------------------------------------------------------------------------------------------
void BurstKit::SetSecretPhrase(String passphrase)
{
	accountData.secretPhrase = passphrase; // TODO only store SHA256 of pw in memory and sign txs with that instead of full str
	
	// get the public key
	MemoryBlock pubKey;
	crypto.getPublicKey(MemoryBlock(accountData.secretPhrase.toUTF8(), accountData.secretPhrase.getNumBytesAsUTF8()), pubKey);
	accountData.pubKey_64HEX = String::toHexString(pubKey.getData(), pubKey.getSize()).removeCharacters(" ");
	
	// get the RS	
	SHA256 shapub(pubKey);// take SHA256 of pubKey
	MemoryBlock shapubMem = shapub.getRawData();
	String srt = String::toHexString(shapubMem.getData(), shapubMem.getSize()).removeCharacters(" ");

	MemoryBlock shapubMemSwapped(shapubMem.getData(), 8);
	BigInteger bi;
	bi.loadFromMemoryBlock(shapubMemSwapped);
	SetAccountID(bi.toString(10, 1));
}

void BurstKit::SetAccount(String account)
{
	SetAccountRS(ensureReedSolomon(account));
}

void BurstKit::SetAccountID(String accountID)
{
	accountData.reedSolomon = GetJSONvalue(rsConvert(accountID), "accountRS");
	accountData.accountID = accountID;
}

void BurstKit::SetAccountRS(String reedSolomonIn)
{
	accountData.reedSolomon = reedSolomonIn;
	if (accountData.reedSolomon.startsWith("BURST-") == false)
		accountData.reedSolomon = ("BURST-") + accountData.reedSolomon;

	accountData.accountID = GetJSONvalue(getAccount(accountData.reedSolomon), "account");
}

// CreateTX, Sign And Broadcast ------------------------------------------------------------------------------------
String BurstKit::CreateTX(String url, String feeNQT, String deadlineMinutes, bool broadcast)
{
	int retry = 5;
	String signedTransactionBytesStr;
	while (signedTransactionBytesStr.isEmpty() && --retry > 0)
	{
		const String unsignedTransactionBytesStr = GetUnsignedTransactionBytes(url + TxRequestArgs(accountData.pubKey_64HEX, feeNQT, deadlineMinutes));

		signedTransactionBytesStr = Sign(unsignedTransactionBytesStr);
		// parse the tx to check the validity
		const String parseResult = parseTransaction(signedTransactionBytesStr, String::empty);

		String verify = GetJSONvalue(parseResult, "verify");
		if ((verify.getIntValue() > 0 || verify.compareIgnoreCase("true") == 0) == false)
		{
			signedTransactionBytesStr.clear();
			Time::waitForMillisecondCounter(Time::getApproximateMillisecondCounter() + 100);
		}
	}
	if (broadcast && signedTransactionBytesStr.isNotEmpty())
		return broadcastTransaction(signedTransactionBytesStr);
	else return signedTransactionBytesStr;
}

String BurstKit::SignAndBroadcast(String unsignedTransactionBytesStr)
{
	return broadcastTransaction(Sign(unsignedTransactionBytesStr));
}

String BurstKit::Sign(String unsignedTransactionBytesStr)
{
	if (accountData.secretPhrase.isEmpty())
		return String::empty;

	MemoryBlock unsignedTransactionBytes;
	unsignedTransactionBytes.loadFromHexString(unsignedTransactionBytesStr);
	if (unsignedTransactionBytes.getSize() > 0)
	{
		// get the prepared but unsigned bytes, and sign it locally with the pass phrase
		// can test with signTransaction https://burstwiki.org/wiki/The_Burst_API#Sign_Transaction
		MemoryBlock signature = crypto.sign(unsignedTransactionBytes, MemoryBlock(accountData.secretPhrase.toUTF8(), accountData.secretPhrase.getNumBytesAsUTF8()));
		MemoryBlock signedTransactionBytes(unsignedTransactionBytes);
		signedTransactionBytes.copyFrom(signature.getData(), 32 * 3, signature.getSize());
		String transactionBytesHex = String::toHexString(signedTransactionBytes.getData(), signedTransactionBytes.getSize()).removeCharacters(" ");

		// for debug
		//	URL signUrl(host + "burst?requestType=signTransaction&unsignedTransactionBytes=" + unsignedTransactionBytesStr + "&secretPhrase=" + secretPhraseEscapeChars);
		//	String signUrlRes = signUrl.readEntireTextStream(true);
		//	String transactionBytes; // the final transactionBytes. unsignedTransactionBytesStr + transactionBytesHex

		return transactionBytesHex;
	}
	return String::empty;
}

// UTILS --------------------------------------------------------------------------------------------------
String BurstKit::GetJSONvalue(String json, String key)
{
	var jsonStructure;
	Result r = JSON::parse(json, jsonStructure);
	return jsonStructure.getProperty(key, String::empty);
}

String BurstKit::GetUrlStr(String url)
{
	return URL(url).readEntireTextStream(true);
}

var BurstKit::GetUrlJson(String urlStr)
{
	String url = urlStr;
	String json = URL(url).readEntireTextStream(true);
	var jsonStructure;
	Result r = JSON::parse(json, jsonStructure);
	return jsonStructure;
}

String BurstKit::GetUnsignedTransactionBytes(String url)
{
	const var unsignedTX = GetUrlJson(url);
	return unsignedTX["unsignedTransactionBytes"].toString();
}

String BurstKit::TxRequestArgs(
	String publicKey,
	String feeNQT,
	String deadlineMinutes)
{
	// TODO implement suggest fee
	return
		"&publicKey=" + publicKey +
		"&feeNQT=" + feeNQT +
		"&deadline=" + deadlineMinutes;
}

String BurstKit::ensureReedSolomon(String str)
{
	if (str.containsOnly("0123456789"))
	{
		String rs = rsConvert(str);
		str = GetJSONvalue(rs, "accountRS");
	}
	return str;
}

static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

static inline bool is_base64(unsigned char c) {
	return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string BurstKit::toBase64Encoding(unsigned char const* bytes_to_encode, unsigned int in_len)
{
	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (in_len--) {
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for (i = 0; (i <4); i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while ((i++ < 3))
			ret += '=';
	}

	return ret;
}

juce::MemoryBlock BurstKit::fromBase64EncodingToMB(std::string const& encoded_string)
{
	size_t in_len = encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	juce::MemoryBlock ret;

	while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i == 4) {
			for (i = 0; i <4; i++)
				char_array_4[i] = (unsigned char)base64_chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret.append(&char_array_3[i], sizeof(char));
			i = 0;
		}
	}

	if (i) {
		for (j = i; j <4; j++)
			char_array_4[j] = 0;

		for (j = 0; j <4; j++)
			char_array_4[j] = (unsigned char)base64_chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++)// ret += char_array_3[j];
			ret.append(&char_array_3[j], sizeof(char));
	}
	return ret;
}

std::string BurstKit::fromBase64Encoding(std::string const& encoded_string)
{
	size_t in_len = encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	std::string ret;

	while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i == 4) {
			for (i = 0; i <4; i++)
				char_array_4[i] = (unsigned char)base64_chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret += char_array_3[i];
			i = 0;
		}
	}

	if (i) {
		for (j = i; j <4; j++)
			char_array_4[j] = 0;

		for (j = 0; j <4; j++)
			char_array_4[j] = (unsigned char)base64_chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
	}
	return ret;
}

// API 
String BurstKit::broadcastTransaction( // Broadcast a transaction to the network. POST only.
	String signedTransactionBytesStrHex) // is the bytecode of a signed transaction (optional)
{
	if (signedTransactionBytesStrHex.isNotEmpty())
	{
		// send the signed transaction, POST only.
		URL url(host + "burst");
		String post("requestType=broadcastTransaction" + (signedTransactionBytesStrHex.isNotEmpty() ? "&transactionBytes=" + signedTransactionBytesStrHex : ""));

		URL postURL = url.withPOSTData(MemoryBlock(post.toUTF8(), post.getNumBytesAsUTF8()));
		String json = postURL.readEntireTextStream(true);
	//	var resultTX;
	//	Result r = JSON::parse(json, resultTX);

		return json;
	}
	return String::empty;
}

String BurstKit::calculateFullHash(
	String unsignedTransactionBytes, // is the unsigned bytes of a transaction (optional if unsignedTransactionJSON is provided)
	String signatureHash) // is the SHA-256 hash of the transaction signature
{
	return GetUrlStr(host + "burst?requestType=calculateFullHash&unsignedTransactionBytes=" + unsignedTransactionBytes + "&signatureHash=" + signatureHash);
}

String BurstKit::encryptTo( // Encrypt a message using AES without sending it.
	String recipient, // is the account ID of the recipient.
	String messageToEncrypt, // is either UTF - 8 text or a string of hex digits to be compressed and converted into a 1000 byte maximum bytecode then encrypted using AES
	String messageToEncryptIsText, // is false if the message to encrypt is a hex string, true if the encrypted message is text
	String secretPhrase) // is the secret passphrase of the recipient
{
	MemoryBlock myPrivateKey;
	crypto.getPrivateKey(MemoryBlock(secretPhrase.toRawUTF8(), secretPhrase.getNumBytesAsUTF8()), myPrivateKey);

	MemoryBlock theirPublicKey;
	theirPublicKey.loadFromHexString(GetJSONvalue(getAccountPublicKey(recipient), "publicKey"));

	const bool messageToEncryptIsTextBool = messageToEncryptIsText.compareIgnoreCase("true") == 0 || messageToEncryptIsText.getIntValue() > 0;
	MemoryBlock plainData;
	if (!messageToEncryptIsTextBool)
	{
		MemoryBlock data;
		data.loadFromHexString(messageToEncrypt);
		MemoryOutputStream destStream(0); // gzip with DEFAULT_COMPRESSION == 6
		juce::GZIPCompressorOutputStream zipper(&destStream, 6, false, juce::GZIPCompressorOutputStream::windowBitsGZIP);
		zipper.write(data.getData(), data.getSize());
		zipper.flush();
		plainData = destStream.getMemoryBlock();
	}
	else plainData = MemoryBlock(messageToEncrypt.toRawUTF8(), messageToEncrypt.getNumBytesAsUTF8());

	if (plainData.getSize() <= 1000)
	{

		MemoryBlock nonceMB;
		MemoryBlock encrypted = crypto.aesEncrypt(plainData, myPrivateKey, theirPublicKey, nonceMB);
		String nonce = String::toHexString(nonceMB.getData(), nonceMB.getSize(), 0);
		String returnStr = String::toHexString(encrypted.getData(), encrypted.getSize(), 0);

		return returnStr + nonce;
	}
	return String::empty;
	//String chechReturnStr = GetUrlStr(host + "burst?requestType=encryptTo&account=" + ensureReedSolomon(recipient) + "&messageToEncrypt=" + messageToEncrypt + "&messageToEncryptIsText=" + messageToEncryptIsText + "&secretPhrase=" + secretPhraseEscapeChars);
}

String BurstKit::decryptFrom( // Decrypt an AES-encrypted message. 
	String account, // is the account ID of the recipient
	String data, // is the AES-encrypted data
	String nonce, // is the unique nonce associated with the encrypted data
	String decryptedMessageIsText, // is false if the decrypted message is a hex string, true if the decrypted message is text
	String secretPhrase) // is the secret passphrase of the recipient
{
	MemoryBlock dataEncrypted;
	if (nonce.isEmpty() && data.length() > (32 * 2))
	{
		nonce = data.substring(data.length() - (32 * 2));
		data = data.substring(0, data.length() - (32 * 2));
	}
	dataEncrypted.loadFromHexString(data);

	MemoryBlock myPrivateKey;
	crypto.getPrivateKey(MemoryBlock(secretPhrase.toRawUTF8(), secretPhrase.getNumBytesAsUTF8()), myPrivateKey);
	
	MemoryBlock theirPublicKey;
	theirPublicKey.loadFromHexString(GetJSONvalue(getAccountPublicKey(account), "publicKey"));

	MemoryBlock nonceMB;
	nonceMB.loadFromHexString(nonce);

	MemoryBlock decrypted = crypto.aesDecrypt(dataEncrypted, myPrivateKey, theirPublicKey, nonceMB);

	const bool decryptedMessageIsTextBool = decryptedMessageIsText.compareIgnoreCase("true") == 0 || decryptedMessageIsText.getIntValue() > 0;
	{	// unzip
		MemoryInputStream srcStream(decrypted, false);
		juce::MemoryBlock mb;
		juce::GZIPDecompressorInputStream dezipper(&srcStream, false, juce::GZIPDecompressorInputStream::gzipFormat);
		dezipper.readIntoMemoryBlock(mb);
		if (decryptedMessageIsTextBool)
			return mb.toString(); 
		else return String::toHexString(mb.getData(), mb.getSize(), 0);
	}
	// return GetUrlStr(host + "burst?requestType=decryptFrom&account=" + ensureReedSolomon(account) + "&data=" + data + "&nonce=" + nonce + "&decryptedMessageIsText=" + decryptedMessageIsText + "&secretPhrase=" + secretPhraseEscapeChars);
}

String BurstKit::getAccount( // Get account information given an account ID.
	String account_RS_or_ID) // is the account ID (or ReedSolomon, detected by BURST- Prefix and non numerical chars)
{
	return GetUrlStr(host + "burst?requestType=getAccount&account=" + ensureReedSolomon(account_RS_or_ID));
}

String BurstKit::getAccountBlockIds( // Get the block IDs of all blocks forged (generated) by an account in reverse block height order. 
	String account, // is the account ID of the recipient
	String timestamp, // is the earliest block(in seconds since the genesis block) to retrieve(optional)
	String firstIndex, // is the zero-based index to the first block to retrieve (optional)
	String lastIndex) // is the zero-based index to the last block to retrieve (optional)
{
	return GetUrlStr(host + "burst?requestType=getAccountBlockIds" +
		"&account=" + ensureReedSolomon(account) +
		(timestamp.isNotEmpty() ? "&timestamp=" + timestamp : "") +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : "") +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : "") );
}

String BurstKit::getAccountBlocks( // Get all blocks forged (generated) by an account in reverse block height order. 
	String account, // is the account ID of the recipient
	String timestamp, // is the earliest block(in seconds since the genesis block) to retrieve (optional)
	String firstIndex, // is the zero - based index to the first block to retrieve (optional)
	String lastIndex, // is the zero - based index to the last block to retrieve (optional)
	String includeTransactions) // is the true to retrieve transaction details, otherwise only transaction IDs are retrieved (optional)
{
	return GetUrlStr(host + "burst?requestType=getAccountBlocks" +
		"&account=" + ensureReedSolomon(account) +
		(timestamp.isNotEmpty() ? "&timestamp=" + timestamp : "") +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : "") +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : "") +
		(includeTransactions.isNotEmpty() ? "&includeTransactions=" + includeTransactions : ""));
}

String BurstKit::getAccountId( // Get an account ID given public key. (or a secret passphrase (POST only))
	String pubKey_64HEXIn) // is the public key of the account
{
	return GetUrlStr(host + "burst?requestType=getAccountId&publicKey=" + pubKey_64HEXIn);
}

String BurstKit::getAccountTransactionIds( // Get the transaction IDs associated with an account in reverse block timestamp order. Note: Refer to Get Constants for definitions of types and subtypes
	String account, // is the account ID
	String timestamp, // is the earliest block(in seconds since the genesis block) to retrieve(optional)
	String type, // is the type of transactions to retrieve(optional)
	String subtype, // is the subtype of transactions to retrieve(optional)
	String firstIndex, // is the a zero - based index to the first transaction ID to retrieve(optional)
	String lastIndex, // is the a zero - based index to the last transaction ID to retrieve(optional)
	String numberOfConfirmations) // is the required number of confirmations per transaction(optional)
{
	return GetUrlStr(host + "burst?requestType=getAccountTransactionIds" +
		"&account=" + ensureReedSolomon(account) + 
		(timestamp.isNotEmpty() ? "&timestamp=" + timestamp : "") + 
		(type.isNotEmpty() ? "&type=" + type : "") +
		(subtype.isNotEmpty() ? "&subtype=" + subtype : "") +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : "") +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : "") +
		(numberOfConfirmations.isNotEmpty() ? "&numberOfConfirmations=" + numberOfConfirmations : "") );
}

String BurstKit::getAccountPublicKey( // Get the public key associated with an account ID. 
	String account) // is the account ID
{
	return GetUrlStr(host + "burst?requestType=getAccountPublicKey&account=" + ensureReedSolomon(account));
}

String BurstKit::getAccountTransactions( // Get the transactions associated with an account in reverse block timestamp order. 
	String account, // is the account ID
	String timestamp, // is the earliest block(in seconds since the genesis block) to retrieve(optional)
	String type, // is the type of transactions to retrieve(optional)
	String subtype, // is the subtype of transactions to retrieve(optional)
	String firstIndex, // is the a zero - based index to the first transaction ID to retrieve(optional)
	String lastIndex, // is the a zero - based index to the last transaction ID to retrieve(optional)
	String numberOfConfirmations) // is the required number of confirmations per transaction(optional)
{
	return GetUrlStr(host + "burst?requestType=getAccountTransactions" +
		"&account=" + ensureReedSolomon(account) +
		(timestamp.isNotEmpty() ? "&timestamp=" + timestamp : "") +
		(type.isNotEmpty() ? "&type=" + type : "") +
		(subtype.isNotEmpty() ? "&subtype=" + subtype : "") +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : "") +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : "") +
		(numberOfConfirmations.isNotEmpty() ? "&numberOfConfirmations=" + numberOfConfirmations : ""));
}

String BurstKit::setAccountInfo( // Set account information. POST only. Refer to Create Transaction Request for common parameters. 
	String name, // is the name to associate with the account
	String description, // is the description to associate with the account
	String feeNQT,
	String deadlineMinutes,
	bool broadcast)
{
	String url(host + "burst?requestType=setAccountInfo" +
		(name.isNotEmpty() ? "&name=" + name : "") +
		(description.isNotEmpty() ? "&description=" + description : ""));
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast);
}

String BurstKit::getAlias( // Get information about a given alias. 
	String alias, // is the alias ID (optional)
	String aliasName) // is the name of the alias (optional if alias provided)
{
	return GetUrlStr(host + "burst?requestType=getAlias" +
		(alias.isNotEmpty() ? "&alias=" + alias : "") +
		(aliasName.isNotEmpty() ? "&aliasName=" + aliasName : ""));
}

String BurstKit::setAlias( // Create and/or assign an alias. POST only. Refer to Create Transaction Request for common parameters. 
	String aliasName, // is the alias name
	String aliasURI, // is the alias URI(e.g.http://www.google.com/)
	String feeNQT,
	String deadlineMinutes,
	bool broadcast)
{
	String url(host + "burst?requestType=setAlias" +
		(aliasName.isNotEmpty() ? "&aliasName=" + aliasName : "") +
		(aliasURI.isNotEmpty() ? "&aliasURI=" + aliasURI : ""));
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast);
}

String BurstKit::getAliases( // Get information on aliases owned by a given account in alias name order. 
	String account, // is the ID of the account that owns the aliases
	String timestamp, // is the earliest creation time(in seconds since the genesis block) of the aliases(optional)
	String firstIndex, // is the zero - based index to the first alias to retrieve(optional)
	String lastIndex) // is the zero - based index to the last alias to retrieve(optional)
{
	return GetUrlStr(host + "burst?requestType=getAliases" +
		"&account=" + ensureReedSolomon(account) +
		(timestamp.isNotEmpty() ? "&timestamp=" + timestamp : "") +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : "") +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : ""));
}

String BurstKit::buyAlias( // Buy an alias. POST only. Refer to Create Transaction Request for common parameters. 
	String alias, // is the ID of the alias (optional)
	String aliasName, // is the alias name (optional if alias provided)
	String amountNQT, // is the amount (in NQT) offered for an alias for sale (buyAlias only)
	String recipient, // is the account ID of the recipient (only required for sellAlias and only if there is a designated recipient)
	String feeNQT,
	String deadlineMinutes,
	bool broadcast)
{
	const String url(host + "burst?requestType=buyAlias" +
		(alias.isNotEmpty() ? "&alias=" + alias : "") +
		(aliasName.isNotEmpty() ? "&aliasName=" + aliasName : "") +
		(amountNQT.isNotEmpty() ? "&amountNQT=" + amountNQT : "") +
		(recipient.isNotEmpty() ? "&recipient=" + recipient : ""));
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast);
}

String BurstKit::sellAlias( // Sell an alias. POST only. Refer to Create Transaction Request for common parameters. 
	String alias, // is the ID of the alias(optional)
	String aliasName, // is the alias name(optional if alias provided)
	String priceNQT, // is the asking price(in NQT) of the alias(sellAlias only)
	String recipient, // is the account ID of the recipient(only required for sellAlias and only if there is a designated recipient)
	String feeNQT,
	String deadlineMinutes,
	bool broadcast)
{
	const String url(host + "burst?requestType=sellAlias" +
		(alias.isNotEmpty() ? "&alias=" + alias : "") +
		(aliasName.isNotEmpty() ? "&aliasName=" + aliasName : "") +
		(priceNQT.isNotEmpty() ? "&priceNQT=" + priceNQT : "") +
		(recipient.isNotEmpty() ? "&recipient=" + recipient : ""));
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast);
}

String BurstKit::getBalance( // Get the balance of an account. 
	String account, // is the account ID / RS
	String includeEffectiveBalance, // is true to include effectiveBalanceNXT and guaranteedBalanceNQT (optional)
	String height, // is the height to retrieve account balance for, if still available(optional)
	String requireBlock, // is the block ID of a block that must be present in the blockchain during execution(optional)
	String requireLastBlock) // is the block ID of a block that must be last in the blockchain during execution(optional)
{
	return GetUrlStr(host + "burst?requestType=getBalance" +
		"&account=" + ensureReedSolomon(account) +
		(includeEffectiveBalance.isNotEmpty() ? "&includeEffectiveBalance=" + includeEffectiveBalance : "") +
		(height.isNotEmpty() ? "&height=" + height : "") +
		(requireBlock.isNotEmpty() ? "&requireBlock=" + requireBlock : "") +
		(requireLastBlock.isNotEmpty() ? "&requireLastBlock=" + requireLastBlock : ""));
}

String BurstKit::getGuaranteedBalance( // Get the balance of an account that is confirmed at least a specified number of times. 
	String account, // is the account ID
	String numberOfConfirmations) // is the minimum number of confirmations for a transaction to be included in the guaranteed balance(optional, if omitted or zero then minimally confirmed transactions are included)
{
	return GetUrlStr(host + "burst?requestType=getGuaranteedBalance" +
		"&account=" + ensureReedSolomon(account) +
		(numberOfConfirmations.isNotEmpty() ? "&numberOfConfirmations=" + numberOfConfirmations : "") );
}

String BurstKit::getTransaction( // Get a transaction object given a transaction ID. 
	String transactionID, // a transaction ID. 
	String fullHash) //  is the full hash of the transaction (optional if transaction ID is provided)	
{
	return GetUrlStr(host + "burst?requestType=getTransaction" +
		(transactionID.isNotEmpty() ? "&transaction=" + transactionID : "") +
		(fullHash.isNotEmpty() ? "&fullHash=" + fullHash : ""));
}
String BurstKit::getUnconfirmedTransactionsIds( // Get a list of unconfirmed transaction IDs associated with an account. 
	String account) // is the account ID(optional)
{
	return GetUrlStr(host + "burst?requestType=getUnconfirmedTransactionIds" +
		(account.isNotEmpty() ? "&account=" + account : ""));
}
String BurstKit::getUnconfirmedTransactions( // Get a list of unconfirmed transactions associated with an account. 
	String account) // is the account ID(optional)		
{
	return GetUrlStr(host + "burst?requestType=getUnconfirmedTransactions" +
		(account.isNotEmpty() ? "&account=" + account : ""));
}
String BurstKit::parseTransaction( // Get a transaction object given a (signed or unsigned) transaction bytecode, or re-parse a transaction object. Verify the signature.
	String transactionBytes, // is the signed or unsigned bytecode of the transaction(optional)
	String transactionJSON) // is the transaction object(optional if transactionBytes is included)		
{
	return GetUrlStr(host + "burst?requestType=parseTransaction" +
		(transactionBytes.isNotEmpty() ? "&transactionBytes=" + transactionBytes : "") +
		(transactionJSON.isNotEmpty() ? "&transactionJSON=" + transactionJSON : ""));
}

String BurstKit::getTransactionBytes( // Get the bytecode of a transaction. 
	String transaction) // is the transaction ID
{
	return GetUrlStr(host + "burst?requestType=getTransactionBytes" +
		(transaction.isNotEmpty() ? "&transaction=" + transaction : ""));
}

String BurstKit::sendMoney( // Send individual amounts of BURST to up to 64 recipients. POST only. Refer to Create Transaction Request for common parameters. 
	String recipient,
	String amountNQT,
	String feeNQT,
	String deadlineMinutes,
	bool broadcast)
{	// make the url and send the data
	String url(host + "burst?requestType=sendMoney" +
		"&recipient=" + recipient + 
		"&amountNQT=" + amountNQT );
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast);
}

String BurstKit::sendMoneyMulti( // Send the same amount of BURST to up to 128 recipients. POST only. Refer to Create Transaction Request for common parameters. 
	StringArray recipients, // is the account ID of the recipients. The recipients string is <numid1>:<amount1>;<numid2>:<amount2>;<numidN>:<amountN>
	StringArray amountsNQT,
	String feeNQT,
	String deadlineMinutes,
	bool broadcast)
{
	StringArray recipientStrArray;
	for (int i = 0; i < jmin<int>(recipients.size(), amountsNQT.size(), 64); i++) // up to 64 recipients
	{
		recipientStrArray.add(recipients[i] + ":" + amountsNQT[i]);
	}
	String recipientStr = recipientStrArray.joinIntoString(";");

	String url(host + "burst?requestType=sendMoneyMulti" +
		"&recipients=" + recipientStr);
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast);
}

String BurstKit::sendMoneyMultiSame( // Send the same amount of BURST to up to 128 recipients. POST only. Refer to Create Transaction Request for common parameters. 
	StringArray recipients, // is the account ID of the recipients. The recipients string is <numid1>;<numid2>;<numidN>
	String amountNQT,
	String feeNQT,
	String deadlineMinutes,
	bool broadcast)
{
	String recipientStr = recipients.joinIntoString(";");
	String url(host + "burst?requestType=sendMoneyMultiSame" +
		"&recipients=" + recipientStr + 
		"&amountNQT=" + amountNQT);
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast);
}

String BurstKit::readMessage( // Get a message given a transaction ID.
	String transaction) // is the transaction ID of the message	
{
	return GetUrlStr(host + "burst?requestType=readMessage" +
		(transaction.isNotEmpty() ? "&transaction=" + transaction : ""));
}

String BurstKit::sendMessage( // Create an Arbitrary Message transaction. POST only. Refer to Create Transaction Request for common parameters. 
	// Note: Any combination (including none or all) of the three options plain message, messageToEncrypt, and messageToEncryptToSelf will be included in the transaction. 
	// However, one and only one prunable message may be included in a single transaction if there is not already a message of the same type (either plain or encrypted). 
	String message, // is either UTF - 8 text or a string of hex digits(perhaps previously encoded using an arbitrary algorithm) to be converted into a bytecode with a maximum length of one kilobyte(optional)
	String messageIsText, // is false if the message is a hex string, otherwise the message is text (optional)
	String messageToEncrypt, // is either UTF-8 text or a string of hex digits to be compressed and converted into a bytecode with a maximum length of one kilobyte, then encrypted using AES (optional)
	String messageToEncryptIsText, // is false if the message to encrypt is a hex string, otherwise the message to encrypt is text (optional)
	String encryptedMessageData, // is already encrypted data which overrides messageToEncrypt if provided (optional)
	String encryptedMessageNonce, // is a unique 32-byte number which cannot be reused (optional unless encryptedMessageData is provided)
	String messageToEncryptToSelf, // is either UTF-8 text or a string of hex digits to be compressed and converted into a one kilobyte maximum bytecode then encrypted with AES, then sent to the sending account (optional)
	String messageToEncryptToSelfIsText, // is false if the message to self-encrypt is a hex string, otherwise the message to encrypt is text (optional)
	String encryptToSelfMessageData, // is already encrypted data which overrides messageToEncryptToSelf if provided (optional)
	String encryptToSelfMessageNonce, // is a unique 32-byte number which cannot be reused (optional unless encryptToSelfMessageData is provided)
	String recipientPublicKey, // is the public key of the receiving account (optional, enhances security of a new account)
	String feeNQT,
	String deadlineMinutes,
	bool broadcast)
{
	String url(host + "burst?requestType=sendMessage" +
		(message.isNotEmpty() ? "&message=" + message : "") +
		(messageIsText.isNotEmpty() ? "&messageIsText=" + messageIsText : "") +
		(messageToEncrypt.isNotEmpty() ? "&messageToEncrypt=" + messageToEncrypt : "") +
		(messageToEncryptIsText.isNotEmpty() ? "&messageToEncryptIsText=" + messageToEncryptIsText : "") +
		(encryptedMessageData.isNotEmpty() ? "&encryptedMessageData=" + encryptedMessageData : "") +
		(encryptedMessageNonce.isNotEmpty() ? "&encryptedMessageNonce=" + encryptedMessageNonce : "") +
		(messageToEncryptToSelf.isNotEmpty() ? "&messageToEncryptToSelf=" + messageToEncryptToSelf : "") +
		(messageToEncryptToSelfIsText.isNotEmpty() ? "&messageToEncryptToSelfIsText=" + messageToEncryptToSelfIsText : "") +
		(encryptToSelfMessageData.isNotEmpty() ? "&encryptToSelfMessageData=" + encryptToSelfMessageData : "") +
		(encryptToSelfMessageNonce.isNotEmpty() ? "&encryptToSelfMessageNonce=" + encryptToSelfMessageNonce : "") +
		(recipientPublicKey.isNotEmpty() ? "&recipientPublicKey=" + recipientPublicKey : "") );
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast);
}

String BurstKit::suggestFee() // Get a cheap, standard, and priority fee. 
{
	return GetUrlStr(host + "burst?requestType=suggestFee");
}

String BurstKit::getRewardRecipient(
	String account) // is the account ID.
{
	return GetUrlStr(host + "burst?requestType=getRewardRecipient&account=" + ensureReedSolomon(account));
}

String BurstKit::setRewardRecipient( // Set Reward recipient is used to set the reward recipient of a given account. 
	String recipient, // is the account ID of the recipient.
	String feeNQT,
	String deadlineMinutes,
	bool broadcast)
{
	String url(host + "burst?requestType=setRewardRecipient" +
		(recipient.isNotEmpty() ? "&recipient=" + recipient : ""));
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast);
}

String BurstKit::getBlock( // Note: block overrides height which overrides timestamp.
	String block, // is the block ID(optional)
	String height, // is the block height(optional if block provided)
	String timestamp, // is the timestamp(in seconds since the genesis block) of the block(optional if height provided)
	String includeTransactions) // is true to include transaction details(optional)
{
	return GetUrlStr(host + "burst?requestType=getBlock" +
		(block.isNotEmpty() ? "&block=" + block : "") +
		(height.isNotEmpty() ? "&height=" + height : "") +
		(timestamp.isNotEmpty() ? "&timestamp=" + timestamp : "") +
		(includeTransactions.isNotEmpty() ? "&includeTransactions=" + includeTransactions : ""));
}

String BurstKit::getBlockId( // Get a block ID given a block height.
	String height) // is the block height
{
	return GetUrlStr(host + "burst?requestType=getBlockId&height=" + height);
}

String BurstKit::getBlockchainStatus() // Get the blockchain status. 
{
	return GetUrlStr(host + "burst?requestType=getBlockchainStatus");
}

String BurstKit::getBlocks( // Get blocks from the blockchain in reverse block height order.
	String firstIndex, // is the first block to retrieve(optional, default is zero or the last block on the blockchain)
	String lastIndex, // is the last block to retrieve(optional, default is firstIndex + 99)
	String includeTransactions) // is true to include transaction details(optional)
{
	return GetUrlStr(host + "burst?requestType=getBlocks" +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : "") +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : "") +
		(includeTransactions.isNotEmpty() ? "&includeTransactions=" + includeTransactions : ""));
}

String BurstKit::getConstants() // Get all defined constants. 
{
	return GetUrlStr(host + "burst?requestType=getConstants");
}

String BurstKit::getECBlock( // Get Economic Cluster block data.  Note: If timestamp is more than 15 seconds before the timestamp of the last block on the blockchain, errorCode 4 is returned.
	String timestamp) // is the timestamp(in seconds since the genesis block) of the EC block(optional, default (or zero) is the current timestamp)
{
	return GetUrlStr(host + "burst?requestType=getECBlock&timestamp=" + timestamp);
}

String BurstKit::getMiningInfo() // Get Mining Info
{
	return GetUrlStr(host + "burst?requestType=getMiningInfo");
}

String BurstKit::getMyInfo() // Get hostname and address of the requesting node. 
{
	return GetUrlStr(host + "burst?requestType=getMyInfo");
}

String BurstKit::getPeer( // Get information about a given peer. 
	String peer) // is the IP address or domain name of the peer(plus optional port)
{
	return GetUrlStr(host + "burst?requestType=getPeer&peer=" + peer);
}

String BurstKit::getPeers( // Get a list of peer IP addresses.  Note: If neither active nor state is specified, all known peers are retrieved. 
	String active, // is true for active(not NON_CONNECTED) peers only(optional, if true overrides state)
	String state) // is the state of the peers, one of NON_CONNECTED, CONNECTED, or DISCONNECTED(optional)
{
	return GetUrlStr(host + "burst?requestType=getPeers" +
		(active.isNotEmpty() ? "&active=" + active : "") +
		(state.isNotEmpty() ? "&state=" + state : ""));
}

String BurstKit::getTime() // Get the current time. 
{
	return GetUrlStr(host + "burst?requestType=getTime");
}

String BurstKit::getState( // Get the state of the server node and network. 
	String includeCounts) // is true if the fields beginning with numberOf... are to be included(optional); password protected like the Debug Operations if true.
{
	return GetUrlStr(host + "burst?requestType=getState" +
		(includeCounts.isNotEmpty() ? "&includeCounts=" + includeCounts : ""));
}

String BurstKit::longConvert( // Converts an ID to the signed long integer representation used internally. 
	String id) // is the numerical ID, in decimal form but equivalent to an 8-byte unsigned integer as produced by SHA-256 hashing
{
	return GetUrlStr(host + "burst?requestType=longConvert&id=" + id);
}

String BurstKit::rsConvert( // Get both the Reed-Solomon account address and the account number given an account ID. 
	String account) // is the account ID (either RS address or number)
{
	return GetUrlStr(host + "burst?requestType=rsConvert&account=" + account);
}

