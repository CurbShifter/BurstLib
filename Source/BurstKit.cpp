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
#include <bitset>

using namespace juce;

BurstKit::BurstKit(String hostUrl, String passPhrase) : burstKitVersionString(SVNRevision), forceSSL(false)
{	
	SetNode(hostUrl);

	if (passPhrase.isNotEmpty())
	{
		SetSecretPhrase(passPhrase, 0);
	}
#if BURSTKIT_SHABAL == 1
	simd_shabal256_init(&ccInitSIMD);
	sph_shabal256_init(&ccInit);

	int cpuInformation[4];
	__cpuid(cpuInformation, 0);

	int nIds = cpuInformation[0];

	if (nIds<1){
		// possible old cpu
		return;
	}

	int cpuInformationEx[4];
	__cpuidex(cpuInformationEx, 1, 0);

	std::bitset<32> Ecx = cpuInformationEx[2];
	std::bitset<32> Edx = cpuInformationEx[3];

	m_supportSSE = Edx[25];
	m_supportSSE2 = Edx[26];
	m_supportAVX = Ecx[28];

	if (nIds>6){
		__cpuidex(cpuInformationEx, 7, 0);
		std::bitset<32> Ebx = cpuInformationEx[1];
		m_supportAVX2 = Ebx[5];
	}

	//m_supportAVX = m_supportAVX2 = false;
#endif
	memKey[0].setSize(32);
	memKey[1].setSize(32);

	Random random;
	random.fillBitsRandomly(memKey[0].getData(), 32);
	random.fillBitsRandomly(memKey[1].getData(), 32);
}

BurstKit::~BurstKit()
{
}

bool BurstKit::IsOnTestNet()
{
	return nodeAddress.contains("test"); // obviously not fool proof
}

int BurstKit::GetBurstKitVersionNumber()
{
	return SVNRevisionN;
}

const char* BurstKit::GetBurstKitVersionString()
{
	return burstKitVersionString;
}

void BurstKit::SetNode(const String hostUrl, const bool allowNonSSL)
{
	nodeAddress = hostUrl;
	if (hostUrl.isEmpty())
		return;

	if (nodeAddress.endsWithChar('/')) // remove trailing
		nodeAddress = nodeAddress.substring(0, nodeAddress.length() - 1);

	protocol = "https://";
	// remove protocol from nodeAddress
	if (nodeAddress.startsWith("http://"))
	{
		protocol = "http://";
		nodeAddress = nodeAddress.substring(strlen("http://"), nodeAddress.length());
	}
	else if (nodeAddress.startsWith("https://"))
	{
		nodeAddress = nodeAddress.substring(strlen("https://"), nodeAddress.length());
	}

	// check protocol
	if (forceSSL)
		protocol = "https://";
	else
	{ 
		if (allowNonSSL && protocol.compareIgnoreCase("https://") == 0)
		{ // check if ssl is available. else default back to plain		
			String v = getMyInfo();
			if (v.isEmpty())
				protocol = "http://";
		}
	}
	// refresh account data
	if (accountData.secretPhrase_enc[0].getSize() > 0)
	{
		String addressID;
		CalcPubKeys(GetSecretPhrase(), accountData.pubKey_HEX, accountData.pubKey_b64, addressID);

		SetAccountID(addressID);
	}
}

void BurstKit::SetForceSSL_TSL(const bool force)
{
	forceSSL = force;
	if (forceSSL)
		SetNode(nodeAddress);
}

bool BurstKit::GetForceSSL_TSL()
{
	return forceSSL;
}

String BurstKit::GetNode()
{
	return protocol + nodeAddress + "/";
}

String BurstKit::GetAccountPublicKey(const unsigned int index)
{
	if (index == 0)
		return accountData.pubKey_HEX;
	else
	{
		String pubKey_hex, pubKey_b64, rs, num;
		GetWallet(index, pubKey_hex, pubKey_b64, rs, num);
		return pubKey_hex;
	}
}

String BurstKit::GetAccountRS(const unsigned int index)
{
	if (index == 0)
		return accountData.reedSolomon;
	else
	{
		String pubKey_hex, pubKey_b64, rs, num;
		GetWallet(index, pubKey_hex, pubKey_b64, rs, num);
		return rs;
	}
}

String BurstKit::GetAccountID(const unsigned int index)
{
	if (index == 0)
		return accountData.accountID;
	else
	{
		String pubKey_hex, pubKey_b64, rs, num;
		GetWallet(index, pubKey_hex, pubKey_b64, rs, num);
		return num;
	}
}

String BurstKit::GetLastError(int &errorCode)
{
	errorCode = lastErrorCode;
	return lastErrorDescription;
};

void BurstKit::SetError(int errorCode, String msg)
{
	lastErrorCode = errorCode;
	lastErrorDescription = msg;
}

// LOCAL ACCOUNT ---------------------------------------------------------------------------------------------
String BurstKit::GetSecretPhraseString(const unsigned int index)
{
	const MemoryBlock r = GetSecretPhrase(index);
	if (CharPointer_UTF8::isValidString((const char* const)r.getData(), r.getSize()))
		return r.toString();
	else return String::empty;
}

MemoryBlock BurstKit::GetSecretPhrase(const unsigned int index)
{
	const MemoryBlock secretPhrase_mem = crypto.aesDecrypt(accountData.secretPhrase_enc[index % 2], memKey[0], memKey[1], accountData.memNonce[index % 2]);
	
	if (String((const char*)secretPhrase_mem.getData(), jmin<int>(secretPhrase_mem.getSize(), 3)).compare("MEM") == 0)
		return MemoryBlock(&((char*)secretPhrase_mem.getData())[3], jmax<int>(0, secretPhrase_mem.getSize() - 3)); // "MEM"
	else return MemoryBlock();
}

void BurstKit::SetSecretPhrase(const String passphrase, const unsigned int index)
{
	if (passphrase.isNotEmpty())
	{
		const String mem = "MEM" + passphrase;

		accountData.secretPhrase_enc[index % 2] = crypto.aesEncrypt(MemoryBlock(mem.toUTF8(), mem.getNumBytesAsUTF8()), memKey[0], memKey[1], accountData.memNonce[index % 2]);

		String addressID;
		CalcPubKeys(MemoryBlock(passphrase.toUTF8(), passphrase.getNumBytesAsUTF8()), accountData.pubKey_HEX, accountData.pubKey_b64, addressID);
		SetAccountID(addressID);
	}
	else
	{
		accountData.secretPhrase_enc[index % 2].reset();
		accountData.pubKey_HEX.clear();
		accountData.pubKey_b64.clear();

		accountData.reedSolomon.clear();
		accountData.accountID.clear();
	}
}

void BurstKit::CalcPubKeys(const MemoryBlock pw, String &pubKey_hex, String &pubKey_b64, String &addressID)
{
	MemoryBlock pubKey;
	crypto.getPublicKey(pw, pubKey);

	pubKey_hex = String::toHexString(pubKey.getData(), pubKey.getSize(), 0);	
	pubKey_b64 = toBase64Encoding(pubKey);
	addressID = ConvertPubKeyToNumerical(pubKey);
}

String BurstKit::ConvertPubKeyToNumerical(const MemoryBlock pubKey)
{
	SHA256 shapub(pubKey);// take SHA256 of pubKey
	MemoryBlock shapubMem = shapub.getRawData();

	MemoryBlock shapubMemSwapped(shapubMem.getData(), 8);
	BigInteger bi;
	bi.loadFromMemoryBlock(shapubMemSwapped);

	return bi.toString(10, 1);
}

void BurstKit::GetWallet(const unsigned int index, String &pubKey_hex, String &pubKey_b64, String &reedSolomon, String &addressID)
{
	CalcPubKeys(GetSecretPhrase(index), pubKey_hex, pubKey_b64, addressID);
	reedSolomon = convertToReedSolomon(addressID);
}

void BurstKit::SetAccount(const String account)
{
	SetAccountRS(convertToReedSolomon(account));
}

void BurstKit::SetAccountID(const String accountID)
{
	accountData.reedSolomon = GetJSONvalue(rsConvert(accountID), "accountRS");
	accountData.accountID = accountID;
}

void BurstKit::SetAccountRS(const String reedSolomonIn)
{
	accountData.reedSolomon = reedSolomonIn;
	if (accountData.reedSolomon.startsWith("BURST-") == false)
		accountData.reedSolomon = ("BURST-") + accountData.reedSolomon;

	accountData.accountID = GetJSONvalue(getAccount(accountData.reedSolomon), "account");
}

uint64 BurstKit::GetAssetBalance(const String assetID, const unsigned int index)
{
	String accountStr = getAccount(accountData.reedSolomon);

	var jsonStructure;
	Result r = JSON::parse(accountStr, jsonStructure);

	if (jsonStructure["assetBalances"].isArray())
	{
		for (int i = 0; i < jsonStructure["assetBalances"].size(); i++)
		{
			if (jsonStructure["assetBalances"][i]["asset"].toString().compare(assetID) == 0)
			{
				return jsonStructure["assetBalances"][i]["balanceQNT"].toString().getLargeIntValue();
			}
		}
	}
	return 0;
}

uint64 BurstKit::GetBalance(const unsigned int index)
{	
	uint64 total = 0;
	if (index == -1)
	{ // add all wallet balances together
		if (accountData.reedSolomon.isNotEmpty())
		{
			String balanceJSON = getBalance(accountData.reedSolomon);
			String balanceStr = GetJSONvalue(balanceJSON, "balanceNQT");
			total += GetUINT64(balanceStr);
		}
		int maxChainDepth = 10;
		unsigned int index = 1;
		while (maxChainDepth > 0)
		{
			uint64 bal = GetWalletBalance(index);
			total += bal;

			if (bal == 0)
				maxChainDepth--;
			else maxChainDepth = 10;

			index++;
		}
	}
	else
	{ // single wallet balance
		total = GetWalletBalance(index);
	}
	return total;
}

uint64 BurstKit::GetWalletBalance(const unsigned int index)
{
	String pubKey_hex, pubKey_b64;
	String reedSolomon;
	String addressID;
	GetWallet(index, pubKey_hex, pubKey_b64, reedSolomon, addressID);

	uint64 bal = 0;
	String pubKeyFromChain = GetJSONvalue(getAccountPublicKey(reedSolomon), "publicKey");
	if (pubKeyFromChain.compareIgnoreCase(pubKey_hex) == 0)
	{ // make sure the onchain wallet matches our pubkey.
		String balanceJSON = getBalance(reedSolomon);
		String balanceStr = GetJSONvalue(balanceJSON, "balanceNQT");
		return GetUINT64(balanceStr);
	}

	return 0;
}

String BurstKit::GetRecipientPublicKey(const String recipient)
{
	const String base64_chars("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=");
	if (recipient.containsOnly("0123456789ABCDEFabcdef") && recipient.length() == 64)
		return recipient;	
	if (recipient.containsOnly(base64_chars) && recipient.endsWithChar('=') && recipient.length() == 44)
	{
		MemoryBlock m = fromBase64Encoding(recipient);
		return String::toHexString(m.getData(), m.getSize(), 0);
	}
	return String::empty;
}

// CreateTX, Sign And Broadcast ------------------------------------------------------------------------------------
String BurstKit::CreateTX(const String url, const String feeNQT, const String deadlineMinutes, const bool broadcast, const unsigned int index)
{
	int retry = 5;
	String signedTransactionBytesStr;
	String unsignedTransactionBytesStr;
	while (signedTransactionBytesStr.isEmpty() && --retry > 0)
	{
		unsignedTransactionBytesStr = GetUnsignedTransactionBytes(url + TxRequestArgs(accountData.pubKey_HEX, feeNQT, deadlineMinutes));
		if (unsignedTransactionBytesStr.isNotEmpty() && unsignedTransactionBytesStr.containsOnly("0123456789ABCDEFabcdef"))
		{
			signedTransactionBytesStr = Sign(unsignedTransactionBytesStr, index);
			// parse the tx to check the validity
			const String parseResult = parseTransaction(signedTransactionBytesStr, String::empty);

			String verify = GetJSONvalue(parseResult, "verify");
			if ((verify.getIntValue() > 0 || verify.compareIgnoreCase("true") == 0) == false)
			{
				signedTransactionBytesStr.clear();
				Time::waitForMillisecondCounter(Time::getApproximateMillisecondCounter() + 100);
			}
		}
		else Time::waitForMillisecondCounter(Time::getApproximateMillisecondCounter() + 100);
	}
	if (broadcast && signedTransactionBytesStr.isNotEmpty())
		return broadcastTransaction(signedTransactionBytesStr);
	else if (signedTransactionBytesStr.isNotEmpty())
		return signedTransactionBytesStr;
	else return unsignedTransactionBytesStr; // probably an error occured. description returned
}

String BurstKit::SignAndBroadcast(const String unsignedTransactionBytesStr, const unsigned int index)
{
	return broadcastTransaction(Sign(unsignedTransactionBytesStr, index));
}

String BurstKit::Sign(const String unsignedTransactionBytesStr, const unsigned int index)
{
	if (accountData.secretPhrase_enc[index %2].getSize() <= 0)
		return String::empty;

	MemoryBlock unsignedTransactionBytes;
	unsignedTransactionBytes.loadFromHexString(unsignedTransactionBytesStr);
	if (unsignedTransactionBytes.getSize() > 0)
	{
		// get the prepared but unsigned bytes, and sign it locally with the pass phrase
		// can test with signTransaction https://burstwiki.org/wiki/The_Burst_API#Sign_Transaction
		const MemoryBlock signature = crypto.sign(unsignedTransactionBytes, GetSecretPhrase(index));
		MemoryBlock signedTransactionBytes(unsignedTransactionBytes);
		signedTransactionBytes.copyFrom(signature.getData(), 32 * 3, signature.getSize());
		String transactionBytesHex = String::toHexString(signedTransactionBytes.getData(), signedTransactionBytes.getSize(), 0);

		// for debug
		//	URL signUrl(GetNode() + "burst?requestType=signTransaction&unsignedTransactionBytes=" + unsignedTransactionBytesStr + "&secretPhrase=" + secretPhraseEscapeChars);
		//	String signUrlRes = signUrl.readEntireTextStream(true);
		//	String transactionBytes; // the final transactionBytes. unsignedTransactionBytesStr + transactionBytesHex

		return transactionBytesHex;
	}
	return String::empty;
}

// UTILS --------------------------------------------------------------------------------------------------
String BurstKit::GetJSONvalue(const String json, const String key)
{
	var jsonStructure;
	Result r = JSON::parse(json, jsonStructure);
	return jsonStructure.getProperty(key, String::empty);
}

String BurstKit::GetUrlStr(const String url)
{
	const String r =  URL(url).readEntireTextStream(true);
	return r;
}

var BurstKit::GetUrlJson(const String urlStr, String *json)
{
	String url = urlStr;
	String jsonLocal;
	if (json == nullptr)
		json = &jsonLocal;
	*json = URL(url).readEntireTextStream(true);

	var jsonStructure;
	Result r = JSON::parse(*json, jsonStructure);
	return jsonStructure;
}

String BurstKit::GetUnsignedTransactionBytes(String url)
{
	String jsonLocal;
	const var unsignedTX = GetUrlJson(url, &jsonLocal);
	const String unsignedTransactionBytes = unsignedTX["unsignedTransactionBytes"].toString();
	if (unsignedTransactionBytes.isNotEmpty())
		return unsignedTransactionBytes;
	else
	{ // {"errorDescription":"Not enough funds","errorCode":6}
		return jsonLocal;
	}
}

String BurstKit::TxRequestArgs(
	String publicKey,
	String feeNQT,
	String deadlineMinutes)
{
	// suggest fee
	String feeNQTstr = feeNQT;
	if (feeNQT.compareIgnoreCase("cheap") == 0)
		feeNQTstr = GetJSONvalue(suggestFee(), "cheap");
	else if (feeNQT.compareIgnoreCase("standard") == 0 || feeNQT.compareIgnoreCase("normal") == 0 || feeNQT.compareIgnoreCase("0") == 0)
		feeNQTstr = GetJSONvalue(suggestFee(), "standard");
	else if (feeNQT.compareIgnoreCase("priority") == 0)
		feeNQTstr = GetJSONvalue(suggestFee(), "priority");

	return
		"&publicKey=" + publicKey +
		"&feeNQT=" + feeNQTstr +
		"&deadline=" + deadlineMinutes;
}

int BurstKit::DetermineAccountUnit(String str)
{
	const String base64_chars("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=");
	bool isID = str.containsOnly("0123456789");
	bool isPUBKEY = (str.containsOnly("0123456789ABCDEFabcdef") && str.length() == 64) || (str.containsOnly(base64_chars) && str.endsWithChar('=') && str.length() == 44);
	bool isRS = (str.startsWithIgnoreCase("BURST-") && str.length() == 26); // BURST-XXXX-XXXX-XXXX-XXXXX
	isRS = isRS ? isRS : (str.retainCharacters("-").length() == 3 && str.length() == 20); // XXXX-XXXX-XXXX-XXXXX
	bool isAlias = str.startsWithChar('@') || (!isID && !isRS && !isPUBKEY);

	if (isID)
		return 0;
	else if (isRS)
		return 1;
	else if (isAlias)
		return 2;
	else if (isPUBKEY)
		return 3;

	return -1;
}

String BurstKit::convertToAlias(String str)
{
	int accUnit = DetermineAccountUnit(str);
	if (accUnit != 2)
	{ // convert to the first Alias of the account
		String aliases = getAliases(str);
		var aliasesJSON;
		Result r = JSON::parse(aliases, aliasesJSON);
		if (aliasesJSON["aliases"].isArray() && aliasesJSON["aliases"].size() > 0)
		{
			const String aliasName = aliasesJSON["aliases"][0]["aliasName"];
			return aliasName;
		}
	}
	return str;
}

String BurstKit::getAccountAliases(String str, const bool newestFirst, const bool useCache)
{
	//int accUnit = DetermineAccountUnit(str);
	//if (accUnit != 2)
	{ // convert to the first Alias of the account
		String aliases;
		var aliasesJSON;
		if (useCache && getAliasesMap.contains(str))
			aliases = getAliasesMap[str];
		else aliases = getAliases(str);

		Result r = JSON::parse(aliases, aliasesJSON);
		if (aliasesJSON["aliases"].isArray() && aliasesJSON["aliases"].size() > 0)
		{
			StringArray aliasNames;
			int64 aliasTime = 0;
			int64 aliasTimeIdx = -1;
			const int s = aliasesJSON["aliases"].size();
			for (int i = 0; i < s; i++)
			{
				aliasNames.add(aliasesJSON["aliases"][i]["aliasName"].toString());
				if (aliasTime < (int64)(aliasesJSON["aliases"][i]["timestamp"]) && 
					(aliasesJSON["aliases"][i]["aliasURI"].toString().containsIgnoreCase(str))) // needs to refer to this account
				{
					aliasTime = (aliasesJSON["aliases"][i]["timestamp"]);
					aliasTimeIdx = i;
				}
			}
			// move the newest in front
			if (newestFirst && aliasTimeIdx >= 0 && aliasTimeIdx < aliasNames.size())
				aliasNames.move(aliasTimeIdx, 0);
			
			if (aliasNames .size() > 0) // cache
				getAliasesMap.set(str, aliases);

			return aliasNames.joinIntoString(";");
		}
	}
	return String::empty;
}

String BurstKit::convertToReedSolomon(String str, const bool useCache)
{
	int accUnit = DetermineAccountUnit(str);
	if (accUnit == 0)
	{
		String rs = rsConvert(str);
		str = GetJSONvalue(rs, "accountRS");
	}
	else if (accUnit == 2)
	{
		String alias = getAlias("", str.startsWithChar('@') ? str.substring(1) : str, useCache);
		var aliasJSON;
		Result r = JSON::parse(alias, aliasJSON);
		str = aliasJSON["accountRS"];
	}
	else if (accUnit == 3)
	{
		MemoryBlock pubKey;
		//pubKey.loadFromHexString(str);
		pubKey = fromBase64Encoding(str);
		String addressID = ConvertPubKeyToNumerical(pubKey);

		BurstAddress burstAddress;
		str = "BURST-" + burstAddress.encode(GetUINT64(addressID));
	}
	return str;
}

String BurstKit::convertToAccountID(String str, const bool useCache)
{
	if (str.compare("*") == 0)
		return String::empty;

	int accUnit = DetermineAccountUnit(str);
	if (accUnit == 1)
	{
		String id = rsConvert(str);
		str = GetJSONvalue(id, "account");
	}
	else if (accUnit == 2)
	{
		String alias = getAlias("", str.startsWithChar('@') ? str.substring(1) : str, useCache);
		var aliasJSON;
		Result r = JSON::parse(alias, aliasJSON);
		str = aliasJSON["account"];
	}
	else if (accUnit == 3)
	{
		MemoryBlock pubKey;
	//	pubKey.loadFromHexString(str);
		pubKey = fromBase64Encoding(str);
		str = ConvertPubKeyToNumerical(pubKey);
	}
	return str;
}

static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

static inline bool is_base64(unsigned char c) {
	return (isalnum(c) || (c == '+') || (c == '/'));
}

uint64 BurstKit::GetUINT64(const String uint64Str)
{
	return *((uint64*)(GetUINT64MemoryBlock(uint64Str).getData()));
}

MemoryBlock BurstKit::GetUINT64MemoryBlock(const String uint64Str)
{ // TODO drop the use of BigInteger. but String only returns signed version of int 64 (max size should be 2^64)
	juce::BigInteger bigInt; 
	bigInt.parseString(uint64Str, 10);
	MemoryBlock m(bigInt.toMemoryBlock());
	m.ensureSize(8, true);
	return m;
}

String BurstKit::toBase64Encoding(MemoryBlock const& bytes_to_encode)
{
	String ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];
	unsigned int in_len = bytes_to_encode.getSize();
	unsigned char * bytes_to_encode_iter = (unsigned char*)bytes_to_encode.getData();
	while(in_len--)
	{
		char_array_3[i++] = *(bytes_to_encode_iter++);
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

MemoryBlock BurstKit::fromBase64Encoding(String const& encoded_string)
{
	size_t in_len = encoded_string.length();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	juce::MemoryBlock ret;
	const String base64_chars("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");

	while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_]))
	{
		char_array_4[i++] = encoded_string[in_];
		in_++;
		if (i == 4)
		{
			for (i = 0; i <4; i++)
				char_array_4[i] = (unsigned char)base64_chars.indexOfChar(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret.append(&char_array_3[i], sizeof(char));
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j <4; j++)
			char_array_4[j] = 0;

		for (j = 0; j <4; j++)
			char_array_4[j] = (unsigned char)base64_chars.indexOfChar(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++)
			ret.append(&char_array_3[j], sizeof(char));
	}
	return ret;
}

// API 
String BurstKit::broadcastTransaction( // Broadcast a transaction to the network. POST only.
	String signedTransactionBytesStrHex) // is the bytecode of a signed transaction (optional)
{
	if (signedTransactionBytesStrHex.isNotEmpty())
	{ // send the signed transaction, POST only.
		URL url(GetNode() + "burst");
		String post("requestType=broadcastTransaction" + (signedTransactionBytesStrHex.isNotEmpty() ? "&transactionBytes=" + signedTransactionBytesStrHex : String::empty));

		URL postURL = url.withPOSTData(MemoryBlock(post.toUTF8(), post.getNumBytesAsUTF8()));
		String json = postURL.readEntireTextStream(true);

		return json;
	}
	return String::empty;
}

String BurstKit::calculateFullHash(
	String unsignedTransactionBytes, // is the unsigned bytes of a transaction (optional if unsignedTransactionJSON is provided)
	String signatureHash) // is the SHA-256 hash of the transaction signature
{
	return GetUrlStr(GetNode() + "burst?requestType=calculateFullHash&unsignedTransactionBytes=" + unsignedTransactionBytes + "&signatureHash=" + signatureHash);
}

String BurstKit::encryptTo( // Encrypt a message using AES without sending it.
	String &nonce,
	String recipient, // is the account ID of the recipient.
	String messageToEncrypt, // is either UTF - 8 text or a string of hex digits to be compressed and converted into a 1000 byte maximum bytecode then encrypted using AES
	String messageToEncryptIsText, // is false if the message to encrypt is a hex string, true if the encrypted message is text
	String secretPhrase, // is the secret passphrase of the recipient
	const unsigned int index) // is the private chain index
{
	MemoryBlock myPrivateKey;
	if (secretPhrase.isNotEmpty())
		crypto.getPrivateKey(MemoryBlock(secretPhrase.toRawUTF8(), secretPhrase.getNumBytesAsUTF8()), myPrivateKey);
	else crypto.getPrivateKey(GetSecretPhrase(index), myPrivateKey);

	String pubKey = getAccountPublicKey(recipient);
	if ((recipient.containsOnly("0123456789ABCDEFabcdef") && recipient.length() == 64) == false)
		pubKey = GetJSONvalue(pubKey, "publicKey");

	if (pubKey.isNotEmpty())
	{
		MemoryBlock theirPublicKey;
		theirPublicKey.loadFromHexString(pubKey);

		const bool messageToEncryptIsTextBool = messageToEncryptIsText.compareIgnoreCase("true") == 0 || messageToEncryptIsText.getIntValue() > 0;
		MemoryBlock plainData;
		MemoryBlock data;
		if (!messageToEncryptIsTextBool)
			data.loadFromHexString(messageToEncrypt);
		else data = MemoryBlock(messageToEncrypt.toRawUTF8(), messageToEncrypt.getNumBytesAsUTF8());

		MemoryOutputStream destStream(0); // gzip with DEFAULT_COMPRESSION == 6
		juce::GZIPCompressorOutputStream zipper(&destStream, 6, false, juce::GZIPCompressorOutputStream::windowBitsGZIP);
		if (data.getSize() > 0)
			zipper.write(data.getData(), data.getSize());
		zipper.flush();
		plainData = destStream.getMemoryBlock();

		if (plainData.getSize() <= 1000)
		{
			MemoryBlock nonceMB;
			MemoryBlock encrypted = crypto.aesEncrypt(plainData, myPrivateKey, theirPublicKey, nonceMB);
			nonce = String::toHexString(nonceMB.getData(), nonceMB.getSize(), 0);
			String returnStr = String::toHexString(encrypted.getData(), encrypted.getSize(), 0);

			String decryptedMessageIsText(messageToEncryptIsTextBool ? "true" : "false"); // is false if the decrypted message is a hex string, true if the decrypted message is text											
			String decryptedMessage = decryptFrom(recipient, returnStr, nonce, decryptedMessageIsText);

			if (decryptedMessage.compareIgnoreCase(messageToEncrypt) == 0)
				return returnStr;
		}
	}
	return String::empty;
	//String chechReturnStr = GetUrlStr(GetNode() + "burst?requestType=encryptTo&account=" + convertToReedSolomon(recipient) + "&messageToEncrypt=" + messageToEncrypt + "&messageToEncryptIsText=" + messageToEncryptIsText + "&secretPhrase=" + secretPhraseEscapeChars);
}

String BurstKit::decryptFrom( // Decrypt an AES-encrypted message. 
	String account, // is the account ID of the recipient. or the pubkey in hex
	String data, // is the AES-encrypted data
	String nonce, // is the unique nonce associated with the encrypted data
	String decryptedMessageIsText, // is false if the decrypted message is a hex string, true if the decrypted message is text
	String secretPhrase, // is the secret passphrase of the recipient
	const unsigned int index) // is private chain index
{
	MemoryBlock dataEncrypted;
	if (nonce.isEmpty() && data.length() > (32 * 2))
	{
		nonce = data.substring(data.length() - (32 * 2));
		data = data.substring(0, data.length() - (32 * 2));
	}
	dataEncrypted.loadFromHexString(data);
	
	MemoryBlock decrypted;
	{
		MemoryBlock myPrivateKey;
		if (secretPhrase.isNotEmpty())
			crypto.getPrivateKey(MemoryBlock(secretPhrase.toRawUTF8(), secretPhrase.getNumBytesAsUTF8()), myPrivateKey);
		else crypto.getPrivateKey(GetSecretPhrase(index), myPrivateKey);

		MemoryBlock theirPublicKey;
		if ((account.containsOnly("0123456789ABCDEFabcdef") && account.length() == 64) == false)
		{
			if (account.compareIgnoreCase(GetAccountRS()) == 0)
				theirPublicKey.loadFromHexString(GetAccountPublicKey());
			else theirPublicKey.loadFromHexString(GetJSONvalue(getAccountPublicKey(account), "publicKey"));
		}
		else theirPublicKey.loadFromHexString(account);

		MemoryBlock nonceMB;
		nonceMB.loadFromHexString(nonce);

		decrypted = crypto.aesDecrypt(dataEncrypted, myPrivateKey, theirPublicKey, nonceMB);
	}
	const bool decryptedMessageIsTextBool = decryptedMessageIsText.compareIgnoreCase("true") == 0 || decryptedMessageIsText.getIntValue() > 0;
	{	// unzip
		MemoryInputStream srcStream(decrypted, false);
		juce::MemoryBlock mb;
		juce::GZIPDecompressorInputStream dezipper(&srcStream, false, juce::GZIPDecompressorInputStream::gzipFormat);
		dezipper.readIntoMemoryBlock(mb);
		if (decryptedMessageIsTextBool && CharPointer_UTF8::isValidString((const char* const)mb.getData(), mb.getSize()))
			return mb.toString(); 
		else return String::toHexString(mb.getData(), mb.getSize(), 0);
	}
	// return GetUrlStr(GetNode() + "burst?requestType=decryptFrom&account=" + convertToReedSolomon(account) + "&data=" + data + "&nonce=" + nonce + "&decryptedMessageIsText=" + decryptedMessageIsText + "&secretPhrase=" + secretPhraseEscapeChars);
}

String BurstKit::getAccount( // Get account information given an account ID.
	String account_RS_or_ID) // is the account ID (or ReedSolomon, detected by BURST- Prefix and non numerical chars)
{
	return GetUrlStr(GetNode() + "burst?requestType=getAccount&account=" + convertToReedSolomon(account_RS_or_ID));
}

String BurstKit::getAccountBlockIds( // Get the block IDs of all blocks forged (generated) by an account in reverse block height order. 
	String account, // is the account ID of the recipient
	String timestamp, // is the earliest block(in seconds since the genesis block) to retrieve(optional)
	String firstIndex, // is the zero-based index to the first block to retrieve (optional)
	String lastIndex) // is the zero-based index to the last block to retrieve (optional)
{
	return GetUrlStr(GetNode() + "burst?requestType=getAccountBlockIds" +
		"&account=" + convertToReedSolomon(account) +
		(timestamp.isNotEmpty() ? "&timestamp=" + timestamp : String::empty) +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : String::empty) +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : String::empty) );
}

String BurstKit::getAccountBlocks( // Get all blocks forged (generated) by an account in reverse block height order. 
	String account, // is the account ID of the recipient
	String timestamp, // is the earliest block(in seconds since the genesis block) to retrieve (optional)
	String firstIndex, // is the zero - based index to the first block to retrieve (optional)
	String lastIndex, // is the zero - based index to the last block to retrieve (optional)
	String includeTransactions) // is the true to retrieve transaction details, otherwise only transaction IDs are retrieved (optional)
{
	return GetUrlStr(GetNode() + "burst?requestType=getAccountBlocks" +
		"&account=" + convertToReedSolomon(account) +
		(timestamp.isNotEmpty() ? "&timestamp=" + timestamp : String::empty) +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : String::empty) +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : String::empty) +
		(includeTransactions.isNotEmpty() ? "&includeTransactions=" + includeTransactions : String::empty));
}

String BurstKit::getAccountId( // Get an account ID given public key. (or a secret passphrase (POST only))
	String pubKey_64HEXIn) // is the public key of the account
{
	//return ConvertPubKeyToNumerical(pubKey); // "accountRS" "publicKey" "account"
	return GetUrlStr(GetNode() + "burst?requestType=getAccountId&publicKey=" + pubKey_64HEXIn);
}

String BurstKit::getAccountTransactionIds( // Get the transaction IDs associated with an account in reverse block timestamp order. Note: Refer to Get Constants for definitions of types and subtypes
	String account, // is the account ID
	String timestamp, // is the earliest block(in seconds since the genesis block) to retrieve(optional)
	String type, // is the type of transactions to retrieve(optional)
	String subtype, // is the subtype of transactions to retrieve(optional)
	String firstIndex, // is the a zero - based index to the first transaction ID to retrieve(optional)
	String lastIndex, // is the a zero - based index to the last transaction ID to retrieve(optional)
	String numberOfConfirmations, // is the required number of confirmations per transaction(optional)
	bool includeIndirect)
{
	String r = GetUrlStr(GetNode() + "burst?requestType=getAccountTransactionIds" +
		"&account=" + convertToReedSolomon(account) + 
		(timestamp.isNotEmpty() ? "&timestamp=" + timestamp : String::empty) + 
		(type.isNotEmpty() ? "&type=" + type : String::empty) +
		(subtype.isNotEmpty() ? "&subtype=" + subtype : String::empty) +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : String::empty) +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : String::empty) +
		(numberOfConfirmations.isNotEmpty() ? "&numberOfConfirmations=" + numberOfConfirmations : String::empty) +
		(includeIndirect ? "&includeIndirect=true" : String::empty) );

	if (r.contains("includeIndirect\\\"param not known")) // resolve includeIndirect pre BRS 2.3.1
	{
		r = GetUrlStr(GetNode() + "burst?requestType=getAccountTransactionIds" +
			"&account=" + convertToReedSolomon(account) +
			(timestamp.isNotEmpty() ? "&timestamp=" + timestamp : String::empty) +
			(type.isNotEmpty() ? "&type=" + type : String::empty) +
			(subtype.isNotEmpty() ? "&subtype=" + subtype : String::empty) +
			(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : String::empty) +
			(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : String::empty) +
			(numberOfConfirmations.isNotEmpty() ? "&numberOfConfirmations=" + numberOfConfirmations : String::empty));
	}
	return r;
}

String BurstKit::getAccountPublicKey( // Get the public key associated with an account ID. 
	String account) // is the account ID
{
	return GetUrlStr(GetNode() + "burst?requestType=getAccountPublicKey&account=" + convertToReedSolomon(account));
}

String BurstKit::getAccountTransactions( // Get the transactions associated with an account in reverse block timestamp order. 
	String account, // is the account ID
	String timestamp, // is the earliest block(in seconds since the genesis block) to retrieve(optional)
	String type, // is the type of transactions to retrieve(optional)
	String subtype, // is the subtype of transactions to retrieve(optional)
	String firstIndex, // is the a zero - based index to the first transaction ID to retrieve(optional)
	String lastIndex, // is the a zero - based index to the last transaction ID to retrieve(optional)
	String numberOfConfirmations, // is the required number of confirmations per transaction(optional)
	bool includeIndirect)
{
	String r = GetUrlStr(GetNode() + "burst?requestType=getAccountTransactions" +
		"&account=" + convertToReedSolomon(account) +
		(timestamp.isNotEmpty() ? "&timestamp=" + timestamp : String::empty) +
		(type.isNotEmpty() ? "&type=" + type : String::empty) +
		(subtype.isNotEmpty() ? "&subtype=" + subtype : String::empty) +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : String::empty) +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : String::empty) +
		(numberOfConfirmations.isNotEmpty() ? "&numberOfConfirmations=" + numberOfConfirmations : String::empty) +
		(includeIndirect ? "&includeIndirect=true" : String::empty) );

	if (r.contains("includeIndirect\\\"param not known")) // resolve includeIndirect pre BRS 2.3.1
	{
		r = GetUrlStr(GetNode() + "burst?requestType=getAccountTransactions" +
			"&account=" + convertToReedSolomon(account) +
			(timestamp.isNotEmpty() ? "&timestamp=" + timestamp : String::empty) +
			(type.isNotEmpty() ? "&type=" + type : String::empty) +
			(subtype.isNotEmpty() ? "&subtype=" + subtype : String::empty) +
			(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : String::empty) +
			(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : String::empty) +
			(numberOfConfirmations.isNotEmpty() ? "&numberOfConfirmations=" + numberOfConfirmations : String::empty));
	}

	return r;
}

String BurstKit::setAccountInfo( // Set account information. POST only. Refer to Create Transaction Request for common parameters. 
	String name, // is the name to associate with the account
	String description, // is the description to associate with the account
	String feeNQT,
	String deadlineMinutes,
	bool broadcast,
	const unsigned int index)
{
	String url(GetNode() + "burst?requestType=setAccountInfo" +
		(name.isNotEmpty() ? "&name=" + name : String::empty) +
		(description.isNotEmpty() ? "&description=" + description : String::empty));
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast, index);
}

String BurstKit::getAlias( // Get information about a given alias. 
	String alias, // is the alias ID (optional)
	String aliasName, // is the name of the alias (optional if alias provided)
	const bool useCache)
{
	String returnValue;

	// optimise recurring calls for aliasses. will go wrong if alias changes.
	if (useCache && alias.isNotEmpty() && aliasMap.contains(alias))
	{
		returnValue = aliasMap[alias];
	}
	else if (useCache && aliasName.isNotEmpty() && aliasMap.contains(aliasName))
	{
		returnValue = aliasMap[aliasName];
	}
	
	if (returnValue.isEmpty())
	{
		returnValue = GetUrlStr(GetNode() + "burst?requestType=getAlias" +
			(alias.isNotEmpty() ? "&alias=" + alias : String::empty) +
			(aliasName.isNotEmpty() ? "&aliasName=" + aliasName : String::empty));

		if (alias.isNotEmpty())
			aliasMap.set(alias, returnValue);
		else if (aliasName.isNotEmpty())
			aliasMap.set(aliasName, returnValue);
	}
	return returnValue;
}

String BurstKit::setAlias( // Create and/or assign an alias. POST only. Refer to Create Transaction Request for common parameters. 
	String aliasName, // is the alias name
	String aliasURI, // is the alias URI(e.g.http://www.google.com/)
	String feeNQT,
	String deadlineMinutes,
	bool broadcast,
	const unsigned int index)
{
	String url(GetNode() + "burst?requestType=setAlias" +
		(aliasName.isNotEmpty() ? "&aliasName=" + aliasName : String::empty) +
		(aliasURI.isNotEmpty() ? "&aliasURI=" + aliasURI : String::empty));
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast, index);
}

String BurstKit::getAliases( // Get information on aliases owned by a given account in alias name order. 
	String account, // is the ID of the account that owns the aliases
	String timestamp, // is the earliest creation time(in seconds since the genesis block) of the aliases(optional)
	String firstIndex, // is the zero - based index to the first alias to retrieve(optional)
	String lastIndex) // is the zero - based index to the last alias to retrieve(optional)
{
	return GetUrlStr(GetNode() + "burst?requestType=getAliases" +
		"&account=" + convertToReedSolomon(account) +
		(timestamp.isNotEmpty() ? "&timestamp=" + timestamp : String::empty) +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : String::empty) +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : String::empty));
}

String BurstKit::buyAlias( // Buy an alias. POST only. Refer to Create Transaction Request for common parameters. 
	String alias, // is the ID of the alias (optional)
	String aliasName, // is the alias name (optional if alias provided)
	String amountNQT, // is the amount (in NQT) offered for an alias for sale (buyAlias only)
	String recipient, // is the account ID of the recipient (only required for sellAlias and only if there is a designated recipient)
	String feeNQT,
	String deadlineMinutes,
	bool broadcast,
	const unsigned int index)
{
	const String url(GetNode() + "burst?requestType=buyAlias" +
		(alias.isNotEmpty() ? "&alias=" + alias : String::empty) +
		(aliasName.isNotEmpty() ? "&aliasName=" + aliasName : String::empty) +
		(amountNQT.isNotEmpty() ? "&amountNQT=" + amountNQT : String::empty) +
		(recipient.isNotEmpty() ? "&recipient=" + convertToReedSolomon(recipient) : String::empty));
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast, index);
}

String BurstKit::sellAlias( // Sell an alias. POST only. Refer to Create Transaction Request for common parameters. 
	String alias, // is the ID of the alias(optional)
	String aliasName, // is the alias name(optional if alias provided)
	String priceNQT, // is the asking price(in NQT) of the alias(sellAlias only)
	String recipient, // is the account ID of the recipient(only required for sellAlias and only if there is a designated recipient)
	String feeNQT,
	String deadlineMinutes,
	bool broadcast,
	const unsigned int index)
{
	const String url(GetNode() + "burst?requestType=sellAlias" +
		(alias.isNotEmpty() ? "&alias=" + alias : String::empty) +
		(aliasName.isNotEmpty() ? "&aliasName=" + aliasName : String::empty) +
		(priceNQT.isNotEmpty() ? "&priceNQT=" + priceNQT : String::empty) +
		(recipient.isNotEmpty() ? "&recipient=" + convertToReedSolomon(recipient) : String::empty));
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast, index);
}

String BurstKit::getBalance( // Get the balance of an account. 
	String account, // is the account ID / RS
//	String includeEffectiveBalance, // is true to include effectiveBalanceNXT and guaranteedBalanceNQT (optional)
	String height, // is the height to retrieve account balance for, if still available(optional)
	String requireBlock, // is the block ID of a block that must be present in the blockchain during execution(optional)
	String requireLastBlock) // is the block ID of a block that must be last in the blockchain during execution(optional)
{
	return GetUrlStr(GetNode() + "burst?requestType=getBalance" +
		"&account=" + convertToReedSolomon(account) +
	//	(includeEffectiveBalance.isNotEmpty() ? "&includeEffectiveBalance=" + includeEffectiveBalance : String::empty) +
		(height.isNotEmpty() ? "&height=" + height : String::empty) +
		(requireBlock.isNotEmpty() ? "&requireBlock=" + requireBlock : String::empty) +
		(requireLastBlock.isNotEmpty() ? "&requireLastBlock=" + requireLastBlock : String::empty));
}

String BurstKit::getGuaranteedBalance( // Get the balance of an account that is confirmed at least a specified number of times. 
	String account, // is the account ID
	String numberOfConfirmations) // is the minimum number of confirmations for a transaction to be included in the guaranteed balance(optional, if omitted or zero then minimally confirmed transactions are included)
{
	return GetUrlStr(GetNode() + "burst?requestType=getGuaranteedBalance" +
		"&account=" + convertToReedSolomon(account) +
		(numberOfConfirmations.isNotEmpty() ? "&numberOfConfirmations=" + numberOfConfirmations : String::empty) );
}

String BurstKit::getTransaction( // Get a transaction object given a transaction ID. 
	String transactionID, // a transaction ID. 
	String fullHash) //  is the full hash of the transaction (optional if transaction ID is provided)	
{
	return GetUrlStr(GetNode() + "burst?requestType=getTransaction" +
		(transactionID.isNotEmpty() ? "&transaction=" + transactionID : String::empty) +
		(fullHash.isNotEmpty() ? "&fullHash=" + fullHash : String::empty));
}
String BurstKit::getEscrowTransaction( // Get information regarding an escrow with the escrow transaction ID. 
	String transactionID) // a transaction ID. 
{
	return GetUrlStr(GetNode() + "burst?requestType=getEscrowTransaction" +
		(transactionID.isNotEmpty() ? "&escrow=" + transactionID : String::empty));
}
String BurstKit::getUnconfirmedTransactionsIds( // Get a list of unconfirmed transaction IDs associated with an account. 
	String account) // is the account ID(optional)
{
	return GetUrlStr(GetNode() + "burst?requestType=getUnconfirmedTransactionIds" +
		(account.isNotEmpty() ? "&account=" + account : String::empty));
}
String BurstKit::getUnconfirmedTransactions( // Get a list of unconfirmed transactions associated with an account. 
	String account) // is the account ID(optional)		
{
	return GetUrlStr(GetNode() + "burst?requestType=getUnconfirmedTransactions" +
		(account.isNotEmpty() ? "&account=" + convertToAccountID(account) : String::empty));
}
String BurstKit::parseTransaction( // Get a transaction object given a (signed or unsigned) transaction bytecode, or re-parse a transaction object. Verify the signature.
	String transactionBytes, // is the signed or unsigned bytecode of the transaction(optional)
	String transactionJSON) // is the transaction object(optional if transactionBytes is included)		
{
	return GetUrlStr(GetNode() + "burst?requestType=parseTransaction" +
		(transactionBytes.isNotEmpty() ? "&transactionBytes=" + transactionBytes : String::empty) +
		(transactionJSON.isNotEmpty() ? "&transactionJSON=" + transactionJSON : String::empty));
}

String BurstKit::getTransactionBytes( // Get the bytecode of a transaction. 
	String transaction) // is the transaction ID
{
	return GetUrlStr(GetNode() + "burst?requestType=getTransactionBytes" +
		(transaction.isNotEmpty() ? "&transaction=" + transaction : String::empty));
}

String BurstKit::sendMoney( // Send individual amounts of BURST to up to 64 recipients. POST only. Refer to Create Transaction Request for common parameters. 
	String recipient,
	String amountNQT,
	String feeNQT,
	String deadlineMinutes,
	String referencedTransactionFullHash,
	bool broadcast,
	const unsigned int index)
{	// make the url and send the data
	return sendMoneyWithMessage(recipient, amountNQT, feeNQT, deadlineMinutes, String::empty, false, false, referencedTransactionFullHash, broadcast, index);
}

String BurstKit::createMessageArgs(
	const String message,
	const bool isText, // is false if the message to encrypt is a hex string, true if the encrypted message is text
	const bool encrpyted,
	const String recipient)
{
	String messageArgs;
	String messageToEncryptIsText = isText ? "true" : "false"; // is false if the message to encrypt is a hex string, true if the encrypted message is text
	if (encrpyted)
	{
		String encryptedMessageNonce;
		String encryptedMessageData = encryptTo( // Encrypt a message using AES without sending it.
			encryptedMessageNonce,
			recipient, // is the account ID of the recipient.
			message, // is either UTF - 8 text or a string of hex digits to be compressed and converted into a 1000 byte maximum bytecode then encrypted using AES
			messageToEncryptIsText); // is false if the message to encrypt is a hex string, true if the encrypted message is text

		messageArgs = String((messageToEncryptIsText.isNotEmpty() ? "&messageIsText=" + messageToEncryptIsText : String::empty) +
							(encryptedMessageData.isNotEmpty() ? "&encryptedMessageData=" + encryptedMessageData : String::empty) +
							(encryptedMessageNonce.isNotEmpty() ? "&encryptedMessageNonce=" + encryptedMessageNonce : String::empty));
	}
	else
	{
		messageArgs = String((message.isNotEmpty() ? "&message=" + URL::addEscapeChars(message, true, false) + "&messageIsText=" + messageToEncryptIsText : String::empty));
	}
	return messageArgs;
}

String BurstKit::sendMoneyWithMessage(
	const String recipient,
	const String amountNQT,
	const String feeNQT,
	const String deadlineMinutes,
	const String message,
	const bool messageIsText,
	const bool encrpyted,
	const String referencedTransactionFullHash,
	const bool broadcast,
	const unsigned int index)
{	// make the url and send the data
	String messageArgs = createMessageArgs(message, messageIsText, encrpyted, recipient);
	String recipientPublicKey = GetRecipientPublicKey(recipient);

	String url = String(GetNode() + "burst?requestType=" +
		(amountNQT.getLargeIntValue() > 0 ? "sendMoney" : "sendMessage") +
		"&recipient=" + convertToReedSolomon(recipient) +
		(recipientPublicKey.isNotEmpty() ? "&recipientPublicKey=" + recipientPublicKey : String::empty) +
		(amountNQT.getLargeIntValue() > 0 ? "&amountNQT=" + amountNQT : String::empty) +
		(referencedTransactionFullHash.isNotEmpty() ? "&referencedTransactionFullHash=" + referencedTransactionFullHash : String::empty) + 
		messageArgs);

	return CreateTX(url, feeNQT, deadlineMinutes, broadcast, index);
}

String BurstKit::sendMoneyMulti( // Send the same amount of BURST to up to 128 recipients. POST only. Refer to Create Transaction Request for common parameters. 
	StringArray recipients, // is the account ID of the recipients. The recipients string is <numid1>:<amount1>;<numid2>:<amount2>;<numidN>:<amountN>
	StringArray amountsNQT,
	String feeNQT,
	String deadlineMinutes,
	bool broadcast,
	const unsigned int index)
{
	StringArray recipientStrArray;
	for (int i = 0; i < jmin<int>(recipients.size(), amountsNQT.size(), 64); i++) // up to 64 recipients
	{
		recipientStrArray.add(convertToAccountID(recipients[i]) + ":" + amountsNQT[i]);
	}
	String recipientStr = recipientStrArray.joinIntoString(";");

	String url(GetNode() + "burst?requestType=sendMoneyMulti" +
		"&recipients=" + recipientStr);
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast, index);
}

String BurstKit::sendMoneyMultiSame( // Send the same amount of BURST to up to 128 recipients. POST only. Refer to Create Transaction Request for common parameters. 
	StringArray recipients, // is the account ID of the recipients. The recipients string is <numid1>;<numid2>;<numidN>
	String amountNQT,
	String feeNQT,
	String deadlineMinutes,
	bool broadcast,
	const unsigned int index)
{
	String recipientStr = recipients.joinIntoString(";");
	String url(GetNode() + "burst?requestType=sendMoneyMultiSame" +
		"&recipients=" + recipientStr + 
		"&amountNQT=" + amountNQT);
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast, index);
}

String BurstKit::readMessage( // Get a message given a transaction ID.
	String transaction) // is the transaction ID of the message	
{
	return GetUrlStr(GetNode() + "burst?requestType=readMessage" +
		(transaction.isNotEmpty() ? "&transaction=" + transaction : String::empty));
}
/*
String BurstKit::sendMoneyEscrow(
	String recipient,
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
	String referencedTransactionFullHash, // optional referencedTransactionFullHash parameter which creates a chained transaction, meaning that the new transaction cannot be confirmed unless the referenced transaction is also confirmed. This feature allows a simple way of transaction escrow.
	String escrowDeadline,
	String amountNQT,
	String feeNQT,
	String deadlineMinutes,
	bool broadcast)
{
	String url(GetNode() + "burst?requestType=sendMoneyEscrow" +
		(recipient.isNotEmpty() ? "&recipient=" + convertToReedSolomon(recipient) : String::empty) +
		(message.isNotEmpty() ? "&message=" + URL::addEscapeChars(message, true, false) : String::empty) +
		(messageIsText.isNotEmpty() ? "&messageIsText=" + messageIsText : String::empty) +
		(messageToEncrypt.isNotEmpty() ? "&messageToEncrypt=" + messageToEncrypt : String::empty) +
		(messageToEncryptIsText.isNotEmpty() ? "&messageToEncryptIsText=" + messageToEncryptIsText : String::empty) +
		(encryptedMessageData.isNotEmpty() ? "&encryptedMessageData=" + encryptedMessageData : String::empty) +
		(encryptedMessageNonce.isNotEmpty() ? "&encryptedMessageNonce=" + encryptedMessageNonce : String::empty) +
		(messageToEncryptToSelf.isNotEmpty() ? "&messageToEncryptToSelf=" + messageToEncryptToSelf : String::empty) +
		(messageToEncryptToSelfIsText.isNotEmpty() ? "&messageToEncryptToSelfIsText=" + messageToEncryptToSelfIsText : String::empty) +
		(encryptToSelfMessageData.isNotEmpty() ? "&encryptToSelfMessageData=" + encryptToSelfMessageData : String::empty) +
		(encryptToSelfMessageNonce.isNotEmpty() ? "&encryptToSelfMessageNonce=" + encryptToSelfMessageNonce : String::empty) +
		(referencedTransactionFullHash.isNotEmpty() ? "&referencedTransactionFullHash=" + referencedTransactionFullHash : String::empty) +
		(recipientPublicKey.isNotEmpty() ? "&recipientPublicKey=" + recipientPublicKey : String::empty));

	if (GetUINT64(feeNQT) < 200000000 && referencedTransactionFullHash.isNotEmpty()) // 2 BURST for transactions that make use of referencedTransactionFullHash property when creating a new transaction.
		feeNQT = "200000000";
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast);
}
*/
String BurstKit::sendMessage( // Create an Arbitrary Message transaction. POST only. Refer to Create Transaction Request for common parameters. 
	// Note: Any combination (including none or all) of the three options plain message, messageToEncrypt, and messageToEncryptToSelf will be included in the transaction. 
	// However, one and only one prunable message may be included in a single transaction if there is not already a message of the same type (either plain or encrypted). 
	String recipient,
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
	String referencedTransactionFullHash, // optional referencedTransactionFullHash parameter which creates a chained transaction, meaning that the new transaction cannot be confirmed unless the referenced transaction is also confirmed. This feature allows a simple way of transaction escrow.
	String feeNQT,
	String deadlineMinutes,
	bool broadcast,
	const unsigned int index)
{
	recipientPublicKey = GetRecipientPublicKey(recipient); // recipientPublicKey.isEmpty() && recipient.containsOnly("0123456789ABCDEFabcdef") && recipient.length() == 64 ? recipient : String::empty;
	String url(GetNode() + "burst?requestType=sendMessage" +
		(recipient.isNotEmpty() ? "&recipient=" + convertToReedSolomon(recipient) : String::empty) +
		(message.isNotEmpty() ? "&message=" + URL::addEscapeChars(message, true, false) : String::empty) +
		(messageIsText.isNotEmpty() ? "&messageIsText=" + messageIsText : String::empty) +
		(messageToEncrypt.isNotEmpty() ? "&messageToEncrypt=" + messageToEncrypt : String::empty) +
		(messageToEncryptIsText.isNotEmpty() ? "&messageToEncryptIsText=" + messageToEncryptIsText : String::empty) +
		(encryptedMessageData.isNotEmpty() ? "&encryptedMessageData=" + encryptedMessageData : String::empty) +
		(encryptedMessageNonce.isNotEmpty() ? "&encryptedMessageNonce=" + encryptedMessageNonce : String::empty) +
		(messageToEncryptToSelf.isNotEmpty() ? "&messageToEncryptToSelf=" + messageToEncryptToSelf : String::empty) +
		(messageToEncryptToSelfIsText.isNotEmpty() ? "&messageToEncryptToSelfIsText=" + messageToEncryptToSelfIsText : String::empty) +
		(encryptToSelfMessageData.isNotEmpty() ? "&encryptToSelfMessageData=" + encryptToSelfMessageData : String::empty) +
		(encryptToSelfMessageNonce.isNotEmpty() ? "&encryptToSelfMessageNonce=" + encryptToSelfMessageNonce : String::empty) +
		(referencedTransactionFullHash.isNotEmpty() ? "&referencedTransactionFullHash=" + referencedTransactionFullHash : String::empty) +
		(recipientPublicKey.isNotEmpty() ? "&recipientPublicKey=" + recipientPublicKey : String::empty));

	if (GetUINT64(feeNQT) < 200000000 && referencedTransactionFullHash.isNotEmpty()) // 2 BURST for transactions that make use of referencedTransactionFullHash property when creating a new transaction.
		feeNQT = "200000000";
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast, index);
}

String BurstKit::suggestFee() // Get a cheap, standard, and priority fee. 
{
	return GetUrlStr(GetNode() + "burst?requestType=suggestFee");
}

String BurstKit::getRewardRecipient(
	String account) // is the account ID.
{
	return GetUrlStr(GetNode() + "burst?requestType=getRewardRecipient&account=" + convertToReedSolomon(account));
}

String BurstKit::setRewardRecipient( // Set Reward recipient is used to set the reward recipient of a given account. 
	String recipient, // is the account ID of the recipient.
	String feeNQT,
	String deadlineMinutes,
	bool broadcast,
	const unsigned int index)
{
	String recipientPublicKey = GetRecipientPublicKey(recipient); // recipient.containsOnly("0123456789ABCDEFabcdef") && recipient.length() == 64 ? recipient : String::empty;
	String url(GetNode() + "burst?requestType=setRewardRecipient" +
		(recipient.isNotEmpty() ? "&recipient=" + convertToReedSolomon(recipient) : String::empty) +
		(recipientPublicKey.isNotEmpty() ? "&recipientPublicKey=" + recipientPublicKey : String::empty) );
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast, index);
}

String BurstKit::getBlock( // Note: block overrides height which overrides timestamp.
	String block, // is the block ID(optional)
	String height, // is the block height(optional if block provided)
	String timestamp, // is the timestamp(in seconds since the genesis block) of the block(optional if height provided)
	String includeTransactions) // is true to include transaction details(optional)
{
	return GetUrlStr(GetNode() + "burst?requestType=getBlock" +
		(block.isNotEmpty() ? "&block=" + block : String::empty) +
		(height.isNotEmpty() ? "&height=" + height : String::empty) +
		(timestamp.isNotEmpty() ? "&timestamp=" + timestamp : String::empty) +
		(includeTransactions.isNotEmpty() ? "&includeTransactions=" + includeTransactions : String::empty));
}

String BurstKit::getBlockId( // Get a block ID given a block height.
	String height) // is the block height
{
	return GetUrlStr(GetNode() + "burst?requestType=getBlockId&height=" + height);
}

String BurstKit::getBlockchainStatus() // Get the blockchain status. 
{
	return GetUrlStr(GetNode() + "burst?requestType=getBlockchainStatus");
}

String BurstKit::getBlocks( // Get blocks from the blockchain in reverse block height order.
	String firstIndex, // is the first block to retrieve(optional, default is zero or the last block on the blockchain)
	String lastIndex, // is the last block to retrieve(optional, default is firstIndex + 99)
	String includeTransactions) // is true to include transaction details(optional)
{
	return GetUrlStr(GetNode() + "burst?requestType=getBlocks" +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : String::empty) +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : String::empty) +
		(includeTransactions.isNotEmpty() ? "&includeTransactions=" + includeTransactions : String::empty));
}

String BurstKit::getConstants() // Get all defined constants. 
{
	return GetUrlStr(GetNode() + "burst?requestType=getConstants");
}

String BurstKit::getECBlock( // Get Economic Cluster block data.  Note: If timestamp is more than 15 seconds before the timestamp of the last block on the blockchain, errorCode 4 is returned.
	String timestamp) // is the timestamp(in seconds since the genesis block) of the EC block(optional, default (or zero) is the current timestamp)
{
	return GetUrlStr(GetNode() + "burst?requestType=getECBlock&timestamp=" + timestamp);
}

String BurstKit::getMiningInfo() // Get Mining Info
{
	return GetUrlStr(GetNode() + "burst?requestType=getMiningInfo");
}

String BurstKit::getMyInfo() // Get hostname and address of the requesting node. 
{
	return GetUrlStr(GetNode() + "burst?requestType=getMyInfo");
}

String BurstKit::getPeer( // Get information about a given peer. 
	String peer) // is the IP address or domain name of the peer(plus optional port)
{
	return GetUrlStr(GetNode() + "burst?requestType=getPeer&peer=" + peer);
}

String BurstKit::getPeers( // Get a list of peer IP addresses.  Note: If neither active nor state is specified, all known peers are retrieved. 
	String active, // is true for active(not NON_CONNECTED) peers only(optional, if true overrides state)
	String state) // is the state of the peers, one of NON_CONNECTED, CONNECTED, or DISCONNECTED(optional)
{
	return GetUrlStr(GetNode() + "burst?requestType=getPeers" +
		(active.isNotEmpty() ? "&active=" + active : String::empty) +
		(state.isNotEmpty() ? "&state=" + state : String::empty));
}

String BurstKit::getTime() // Get the current time. 
{
	return GetUrlStr(GetNode() + "burst?requestType=getTime");
}

String BurstKit::getState( // Get the state of the server node and network. 
	const String includeCounts) // is true if the fields beginning with numberOf... are to be included(optional); password protected like the Debug Operations if true.
{
	return GetUrlStr(GetNode() + "burst?requestType=getState" +
		(includeCounts.isNotEmpty() ? "&includeCounts=" + includeCounts : String::empty));
}

String BurstKit::longConvert( // Converts an ID to the signed long integer representation used internally. 
	String id) // is the numerical ID, in decimal form but equivalent to an 8-byte unsigned integer as produced by SHA-256 hashing
{
	return GetUrlStr(GetNode() + "burst?requestType=longConvert&id=" + id);
}

String BurstKit::rsConvert( // Get both the Reed-Solomon account address and the account number given an account ID. 
	String account) // is the account ID (either RS address or number)
{ // implemented local
	String rs(account.containsOnly("0123456789") ? String::empty : account);
	String id(account.containsOnly("0123456789") ? account : String::empty);;

	BurstAddress burstAddress;
	if (rs.isEmpty())
		rs = "BURST-" + burstAddress.encode(GetUINT64(account));
	else id = String(burstAddress.decode(account));

	return String("{\"accountRS\":\"" + rs + "\", \"account\" : \"" + id + "\"}");		
//	return GetUrlStr(GetNode() + "burst?requestType=rsConvert&account=" + account);
}

String BurstKit::getAsset( // Get asset information given an asset ID. 
	const String asset) // is the ID of the asset
{
	String url(GetNode() + "burst?requestType=getAsset" +
		"&asset=" + asset);
	return GetUrlStr(url);
}

String BurstKit::transferAsset( // Get the state of the server node and network. 
	const String recipient, // is the recipient account ID
	const String asset, // is the ID of the asset being transferred
	const String quantityQNT, // is the amount(in QNT) of the asset being transferred,
	const String feeNQT,
	const String deadlineMinutes,
	bool broadcast,
	const unsigned int index)
{
	String recipientPublicKey = GetRecipientPublicKey(recipient); // recipient.containsOnly("0123456789ABCDEFabcdef") && recipient.length() == 64 ? recipient : String::empty;
	String url(GetNode() + "burst?requestType=transferAsset" +
		"&recipient=" + convertToReedSolomon(recipient) +
		(recipientPublicKey.isNotEmpty() ? "&recipientPublicKey=" + recipientPublicKey : String::empty) +
		(asset.isNotEmpty() ? "&asset=" + asset : String::empty) +
		(quantityQNT.isNotEmpty() ? "&quantityQNT=" + quantityQNT : String::empty));
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast, index);
}

String BurstKit::transferAsset( // Get the state of the server node and network. 
	const String recipient, // is the recipient account ID
	const String asset, // is the ID of the asset being transferred
	const String quantityQNT, // is the amount(in QNT) of the asset being transferred,
	const String feeNQT,
	const String deadlineMinutes,
	const String message,
	const bool encrypted,
	const String referencedTransactionFullHash,
	const bool broadcast,
	const unsigned int index)
{
	String url;
	String recipientPublicKey = GetRecipientPublicKey(recipient); // recipient.containsOnly("0123456789ABCDEFabcdef") && recipient.length() == 64 ? recipient : String::empty;
	if (encrypted && message.isNotEmpty())
	{
		String encryptedMessageNonce;
		String messageToEncryptIsText = ("true"); // is false if the message to encrypt is a hex string, true if the encrypted message is text
		String encryptedMessageData = encryptTo( // Encrypt a message using AES without sending it.
			encryptedMessageNonce,
			recipient, // is the account ID of the recipient.
			message, // is either UTF - 8 text or a string of hex digits to be compressed and converted into a 1000 byte maximum bytecode then encrypted using AES
			messageToEncryptIsText); // is false if the message to encrypt is a hex string, true if the encrypted message is text

		url = String(GetNode() + "burst?requestType=transferAsset" +
				"&recipient=" + convertToReedSolomon(recipient) +
				(recipientPublicKey.isNotEmpty() ? "&recipientPublicKey=" + recipientPublicKey : String::empty) +
				(asset.isNotEmpty() ? "&asset=" + asset : String::empty) +
				(quantityQNT.getLargeIntValue() > 0 ? "&quantityQNT=" + quantityQNT : String::empty) +
				(referencedTransactionFullHash.isNotEmpty() ? "&referencedTransactionFullHash=" + referencedTransactionFullHash : String::empty) +
				(messageToEncryptIsText.isNotEmpty() ? "&messageIsText=" + messageToEncryptIsText : String::empty) +
				(encryptedMessageData.isNotEmpty() ? "&encryptedMessageData=" + encryptedMessageData : String::empty) +
				(encryptedMessageNonce.isNotEmpty() ? "&encryptedMessageNonce=" + encryptedMessageNonce : String::empty));
	}
	else
	{
		url = String(GetNode() + "burst?requestType=transferAsset" +
				"&recipient=" + convertToReedSolomon(recipient) +
				(recipientPublicKey.isNotEmpty() ? "&recipientPublicKey=" + recipientPublicKey : String::empty) +
				(asset.isNotEmpty() ? "&asset=" + asset : String::empty) +
				(quantityQNT.getLargeIntValue() > 0 ? "&quantityQNT=" + quantityQNT : String::empty) +
				(referencedTransactionFullHash.isNotEmpty() ? "&referencedTransactionFullHash=" + referencedTransactionFullHash : String::empty) +
				(message.isNotEmpty() ? "&message=" + URL::addEscapeChars(message, true, false) + "&messageIsText=true" : String::empty));
	}
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast, index);
}

String BurstKit::getAllAssets(
	const String firstIndex, // is a zero - based index to the first asset to retrieve(optional)
	const String lastIndex, // is a zero - based index to the last asset to retrieve(optional)
	const String includeCounts, // is true if the fields beginning with numberOf... are to be included(optional)
	const String requireBlock, // is the block ID of a block that must be present in the blockchain during execution(optional)
	const String requireLastBlock) // is the block ID of a block that must be last in the blockchain during execution(optional)
{
	String url(GetNode() + "burst?requestType=getAllAssets" +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : String::empty) +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : String::empty) +
		//(includeCounts.isNotEmpty() ? "&includeCounts=" + includeCounts : String::empty) +
		(requireBlock.isNotEmpty() ? "&requireBlock=" + requireBlock : String::empty) +
		(requireLastBlock.isNotEmpty() ? "&requireLastBlock=" + requireLastBlock : String::empty));
	return GetUrlStr(url);
}

String BurstKit::getAssets(
	const String assets, // asset IDs, comma separated
	const String includeCounts, // is true if the fields beginning with numberOf... are to be included(optional)
	const String requireBlock, // is the block ID of a block that must be present in the blockchain during execution(optional)
	const String requireLastBlock) // is the block ID of a block that must be last in the blockchain during execution(optional)
{
	String url(GetNode() + "burst?requestType=getAssets" +
		("&assets=" + assets.replace(",", "&assets=")) +
		(includeCounts.isNotEmpty() ? "&includeCounts=" + includeCounts : String::empty) +
		(requireBlock.isNotEmpty() ? "&requireBlock=" + requireBlock : String::empty) +
		(requireLastBlock.isNotEmpty() ? "&requireLastBlock=" + requireLastBlock : String::empty));
	return GetUrlStr(url);
}

String BurstKit::getAssetTransfers(
	const String asset,// is the asset ID(optional),
	const String account,// is the account ID(optional if asset provided),
	const String timestamp,// is the earliest transfer(in seconds since the genesis block) to retrieve(optional, does not apply to expected transfers),
	const String firstIndex,// is a zero - based index to the first transfer to retrieve(optional, does not apply to expected transfers),
	const String lastIndex,// is a zero - based index to the last transfer to retrieve(optional, does not apply to expected transfers),
	const String includeAssetInfo,// is true if the decimals and name fields are to be included(optional, does not apply to expected transfers),
	const String requireBlock,// is the block ID of a block that must be present in the blockchain during execution(optional),
	const String requireLastBlock)// is the block ID of a block that must be last in the blockchain during execution(optional),
{
	String url(GetNode() + "burst?requestType=getAssetTransfers" +
		(asset.isNotEmpty() ? "&asset=" + asset : String::empty) +
		(account.isNotEmpty() ? "&account=" + account : String::empty) +
		(timestamp.isNotEmpty() ? "&timestamp=" + timestamp : String::empty) +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : String::empty) +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : String::empty) +
		(includeAssetInfo.isNotEmpty() ? "&includeAssetInfo=" + includeAssetInfo : String::empty) +
		(requireBlock.isNotEmpty() ? "&requireBlock=" + requireBlock : String::empty) +
		(requireLastBlock.isNotEmpty() ? "&requireLastBlock=" + requireLastBlock : String::empty));
	return GetUrlStr(url);
}

String BurstKit::issueAsset( // Create an asset on the exchange
	const String name, // is the name of the asset
	const String description, // is the url - encoded description of the asset in UTF - 8 with a maximum length of 1000 bytes(optional)
	const String quantityQNT, // is the total amount(in QNT) of the asset in existence
	const String decimals, // is the number of decimal places used by the asset(optional, zero default)
	const String feeNQT,
	const String deadlineMinutes,
	bool broadcast,
	const unsigned int index)
{
	String url(GetNode() + "burst?requestType=issueAsset" +
		"&name=" + name +
		(description.isNotEmpty() ? "&description=" + description : String::empty) +
		"&quantityQNT=" + quantityQNT +
		(decimals.isNotEmpty() ? "&decimals=" + decimals : String::empty));
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast, index);
}

String BurstKit::getAssetAccounts( // Get trades associated with a given asset and/or account in reverse block height order. 
	const String asset, // is the asset ID
	const String height, // is the height of the blockchain to determine the accounts(optional, default is last block)
	const String firstIndex, // is a zero - based index to the first account to retrieve(optional)
	const String lastIndex) // is a zero - based index to the last account to retrieve(optional)
{
	String url(GetNode() + "burst?requestType=getAssetAccounts" +
		"&asset=" + asset +
		(height.isNotEmpty() ? "&height=" + height : String::empty) +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : String::empty) +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : String::empty));
	return GetUrlStr(url);
}

String BurstKit::getTrades( // Get trades associated with a given asset and/or account in reverse block height order. 
	const String asset, // is the asset ID
	const String account, // is the account ID(optional if asset provided)
	const String firstIndex, // is a zero - based index to the first trade to retrieve(optional)
	const String lastIndex, // is a zero - based index to the last trade to retrieve(optional)
	const String includeAssetInfo) // is true if the decimals and name fields are to be included(optional)
{
	String url(GetNode() + "burst?requestType=getTrades" +
		"&asset=" + asset +
		(account.isNotEmpty() ? "&account=" + convertToReedSolomon(account) : String::empty) +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : String::empty) +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : String::empty) +
		(includeAssetInfo.isNotEmpty() ? "&includeAssetInfo=" + includeAssetInfo : String::empty));
	return GetUrlStr(url);
}

String BurstKit::getAssetsByIssuer( // Get trades associated with a given asset and/or account in reverse block height order. 
	const String account, // is the account ID
	const String firstIndex, // is a zero - based index to the first trade to retrieve(optional)
	const String lastIndex) // is a zero - based index to the last trade to retrieve(optional)
{
	String url(GetNode() + "burst?requestType=getAssetsByIssuer" +
		(account.isNotEmpty() ? "&account=" + convertToReedSolomon(account) : String::empty) +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : String::empty) +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : String::empty));
	return GetUrlStr(url);
}

String BurstKit::placeAskOrder( // Place an asset order
	const String asset, // is the asset ID of the asset being ordered
	const String quantityQNT, // is the amount(in QNT) of the asset being ordered
	const String priceNQT, // is the bid / ask price(in NQT)
	const String feeNQT,
	const String deadlineMinutes,
	const String message,
	const bool messageIsText,
	bool broadcast,
	const unsigned int index)
{
	String messageArgs = createMessageArgs(message, messageIsText, false, String::empty);
	String url(GetNode() + "burst?requestType=placeAskOrder" +
		"&asset=" + asset +
		"&quantityQNT=" + quantityQNT +
		"&priceNQT=" + priceNQT +
		messageArgs);
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast, index);
}

String BurstKit::placeBidOrder( // Place an asset order
	const String asset, // is the asset ID of the asset being ordered
	const String quantityQNT, // is the amount(in QNT) of the asset being ordered
	const String priceNQT, // is the bid / ask price(in NQT)
	const String feeNQT,
	const String deadlineMinutes,
	const String message,
	const bool messageIsText,
	bool broadcast,
	const unsigned int index)
{
	String messageArgs = createMessageArgs(message, messageIsText, false, String::empty);
	String url(GetNode() + "burst?requestType=placeBidOrder" +
		"&asset=" + asset +
		"&quantityQNT=" + quantityQNT +
		"&priceNQT=" + priceNQT +
		messageArgs);
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast, index);
}

String BurstKit::getAskOrder(const String order)
{
	String url(GetNode() + "burst?requestType=getAskOrder" +
		(order.isNotEmpty() ? "&order=" + order : String::empty));
	return GetUrlStr(url);
}

String BurstKit::getAskOrderIds(
	const String asset,
	const String firstIndex,
	const String lastIndex)
{
	String url(GetNode() + "burst?requestType=getAskOrderIds" +
		(asset.isNotEmpty() ? "&asset=" + asset : String::empty) +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : String::empty) +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : String::empty));
	return GetUrlStr(url);
}

String BurstKit::getAskOrders(
	const String asset,
	const String firstIndex,
	const String lastIndex)
{
	String url(GetNode() + "burst?requestType=getAskOrders" +
		(asset.isNotEmpty() ? "&asset=" + asset : String::empty) +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : String::empty) +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : String::empty));
	return GetUrlStr(url);
}

String BurstKit::getAccountCurrentAskOrderIds(
	const String account,
	const String asset,
	const String firstIndex,
	const String lastIndex)
{
	String url(GetNode() + "burst?requestType=getAccountCurrentAskOrderIds" +
		(account.isNotEmpty() ? "&account=" + account : String::empty) +
		(asset.isNotEmpty() ? "&asset=" + asset : String::empty) +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : String::empty) +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : String::empty));
	return GetUrlStr(url);
}

String BurstKit::getAccountCurrentAskOrders(
	const String account,
	const String asset,
	const String firstIndex,
	const String lastIndex)
{
	String url(GetNode() + "burst?requestType=getAccountCurrentAskOrders" +
		(account.isNotEmpty() ? "&account=" + account : String::empty) +
		(asset.isNotEmpty() ? "&asset=" + asset : String::empty) +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : String::empty) +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : String::empty));
	return GetUrlStr(url);
}

String BurstKit::getAllOpenAskOrders(
	const String firstIndex,
	const String lastIndex)
{
	String url(GetNode() + "burst?requestType=getAllOpenAskOrders" +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : String::empty) +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : String::empty));
	return GetUrlStr(url);
}

String BurstKit::cancelAskOrder(
	const String order,
	const String feeNQT,
	const String deadlineMinutes,
	bool broadcast,
	const unsigned int index)
{
	String url(GetNode() + "burst?requestType=cancelAskOrder&order=" + order);
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast, index);
}

String BurstKit::getBidOrder(const String order)
{
	String url(GetNode() + "burst?requestType=getBidOrder" +
		(order.isNotEmpty() ? "&order=" + order : String::empty));
	return GetUrlStr(url);
}

String BurstKit::getBidOrderIds(
	const String asset,
	const String firstIndex,
	const String lastIndex)
{
	String url(GetNode() + "burst?requestType=getBidOrderIds" +
		(asset.isNotEmpty() ? "&asset=" + asset : String::empty) +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : String::empty) +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : String::empty));
	return GetUrlStr(url);
}

String BurstKit::getBidOrders(
	const String asset,
	const String firstIndex,
	const String lastIndex)
{
	String url(GetNode() + "burst?requestType=getBidOrders" +
		(asset.isNotEmpty() ? "&asset=" + asset : String::empty) +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : String::empty) +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : String::empty));
	return GetUrlStr(url);
}

String BurstKit::getAccountCurrentBidOrderIds(
	const String account,
	const String asset,
	const String firstIndex,
	const String lastIndex)
{
	String url(GetNode() + "burst?requestType=getAccountCurrentBidOrderIds" +
		(account.isNotEmpty() ? "&account=" + account : String::empty) +
		(asset.isNotEmpty() ? "&asset=" + asset : String::empty) +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : String::empty) +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : String::empty));
	return GetUrlStr(url);
}

String BurstKit::getAccountCurrentBidOrders(
	const String account,
	const String asset,
	const String firstIndex,
	const String lastIndex)
{
	String url(GetNode() + "burst?requestType=getAccountCurrentBidOrders" +
		(account.isNotEmpty() ? "&account=" + account : String::empty) +
		(asset.isNotEmpty() ? "&asset=" + asset : String::empty) +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : String::empty) +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : String::empty));
	return GetUrlStr(url);
}

String BurstKit::getAllOpenBidOrders(
	const String firstIndex,
	const String lastIndex)
{
	String url(GetNode() + "burst?requestType=getAllOpenBidOrders" +
		(firstIndex.isNotEmpty() ? "&firstIndex=" + firstIndex : String::empty) +
		(lastIndex.isNotEmpty() ? "&lastIndex=" + lastIndex : String::empty));
	return GetUrlStr(url);
}

String BurstKit::cancelBidOrder(
	const String order,
	const String feeNQT,
	const String deadlineMinutes,
	bool broadcast,
	const unsigned int index)
{
	String url(GetNode() + "burst?requestType=cancelBidOrder&order=" + order);
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast, index);
}

// AT -----------------------------------------------------------------------------
String BurstKit::createATProgram(
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
	const unsigned int index)
{
	String url(GetNode() + "burst?requestType=createATProgram" +
		"&name=" + name +
		"&description=" + description +
		"&creationBytes=" + creationBytes +
		"&code=" + code +
		"&data=" + data +
		"&dpages=" + dpages +
		"&cspages=" + cspages +
		"&uspages=" + uspages +
		"&minActivationAmountNQT=" + minActivationAmountNQT);
	return CreateTX(url, feeNQT, deadlineMinutes, broadcast, index);
}

String BurstKit::getAT(
	const String at)
{
	String url(GetNode() + "burst?requestType=getAT" +
		"&at=" + at);
	return GetUrlStr(url);
}

String BurstKit::getATDetails( // same as getAT ?
	const String at)
{
	String url(GetNode() + "burst?requestType=getATDetails" +
		"&at=" + at);
	return GetUrlStr(url);
}

String BurstKit::getATIds()
{
	String url(GetNode() + "burst?requestType=getATIds");
	return GetUrlStr(url);
}
String BurstKit::getATLong(
	const String hexString)
{
	String url(GetNode() + "burst?requestType=getATLong" +
		"&hexString=" + hexString);
	return GetUrlStr(url);
}

String BurstKit::getAccountATs(
	const String account)
{
	String url(GetNode() + "burst?requestType=getAccountATs" +
		"&account=" + account);
	return GetUrlStr(url);
}

#if BURSTKIT_SHABAL == 1
unsigned int BurstKit::Shabal256_ccID()
{
	const int freeCCidx = ccUse.indexOf(false);
	if (freeCCidx >= 0) // reuse contexts
	{
		if (m_supportAVX2 || m_supportAVX)
			ccArraySIMD.getReference(freeCCidx) = ccInitSIMD;
		else ccArray.getReference(freeCCidx) = ccInit;
		ccUse.getReference(freeCCidx) = (true);
		return freeCCidx + 1;
	}
	else // creates a new context when needed
	{
		if (m_supportAVX2 || m_supportAVX)
			ccArraySIMD.add(ccInitSIMD);
		else ccArray.add(ccInit);
		ccUse.add(true);
	}
	return jmax<int>(ccArray.size(), ccArraySIMD.size()); // actual index is minus 1
}

void BurstKit::Shabal256_reset(unsigned int ccID)
{
	if (ccID < 1) return;

	if (m_supportAVX2 || m_supportAVX)
		simd_shabal256_init(&ccArraySIMD.getReference(ccID - 1));
	else sph_shabal256_init(&ccArray.getReference(ccID - 1));
}

void BurstKit::Shabal256_update(unsigned int ccID, void *inbuf, int off, int len)
{
	if (ccID < 1) return;

	if (ccID <= ccArraySIMD.size())
	{
		if (m_supportAVX2)
			AVX2_simd_shabal256(&ccArraySIMD.getReference(ccID - 1), &(((unsigned char*)inbuf)[off]), len);
		else if (m_supportAVX)
			AVX_simd_shabal256(&ccArraySIMD.getReference(ccID - 1), &(((unsigned char*)inbuf)[off]), len);
		/*	else if (m_supportSSE)
				SSE_simd_shabal256(&ccArraySIMD.getReference(ccID - 1), &(((unsigned char*)inbuf)[off]), len);
			else if (m_supportSSE2)
				SSE2_simd_shabal256(&ccArraySIMD.getReference(ccID - 1), &(((unsigned char*)inbuf)[off]), len);*/
	}
	else sph_shabal256(&ccArray.getReference(ccID - 1), &(((unsigned char*)inbuf)[off]), len);
}

void BurstKit::Shabal256_digest(unsigned int ccID, void *dst_32bytes) // dst -> (32 bytes)
{
	if (ccID < 1) return;

	if (ccID <= ccArraySIMD.size())
	{
		if (m_supportAVX2)
			AVX2_simd_shabal256_close(&ccArraySIMD.getReference(ccID - 1), dst_32bytes);
		else if (m_supportAVX)
			AVX_simd_shabal256_close(&ccArraySIMD.getReference(ccID - 1), dst_32bytes);
	}
	else sph_shabal256_close(&ccArray.getReference(ccID - 1), dst_32bytes);
	
	ccUse.getReference(ccID - 1) = false; // free up the mem
}
#endif
