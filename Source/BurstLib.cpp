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

#include "BurstLib.h"
#include "BurstExt.h"

// FIX CentOS error load lib "cannot restore segment prot after reloc: Permission denied"
// terminal:  chcon -t texrel_shlib_t '*.so'

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#elif __APPLE__
// combined with -fvisibility=hidden in project definitions. will only expose the functions with EXPORT defined
#define EXPORT __attribute__((visibility("default")))
#elif defined(__linux__)
#define EXPORT
#endif

extern "C"
{
	EXPORT burstlibPtr BurstLib_GetHandle()
	{
		return reinterpret_cast<burstlibPtr>(new BurstExt());
	}
	EXPORT bool BurstLib_DeleteHandle(const burstlibPtr handle)
	{
		BurstExt *l = reinterpret_cast<BurstExt*>(handle);
		if(l)
			delete l;

		return true;
	}
	EXPORT int BurstLib_GetBurstLibVersionNumber(const burstlibPtr handle)
	{
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);
		return kit != NULL ? kit->GetBurstKitVersionNumber() : 0;
	}
	EXPORT bool BurstLib_GetLastError(const burstlibPtr handle, char *returnStr, int &returnBytes)
	{
		if (handle == NULL || returnBytes <= 0)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		int errorCode = 0;
		String txt = kit->GetLastError(errorCode);

		returnBytes = jmin(returnBytes, txt.length());
		memcpy(returnStr, txt.toRawUTF8(), returnBytes * sizeof(char));
		return true;
	}
	EXPORT bool BurstLib_SetNode Args_SetNode
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);
		kit->SetNode(String(hostUrl, hostUrlBytes));
		return true;
	}
	EXPORT bool BurstLib_GetNode Args_GetNode
	{
		if (handle == NULL || returnBytes <= 0)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		const String txt = kit->GetNode();
		returnBytes = jmin(returnBytes, txt.length());
		memcpy(returnStr, txt.toRawUTF8(), returnBytes * sizeof(char));
		return true;
	}
	EXPORT bool BurstLib_SetSecretPhrase Args_SetSecretPhrase
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);
		kit->SetSecretPhrase(String(passphrase, passphraseBytes));
		return true;
	}
	EXPORT bool BurstLib_SetAccount Args_SetAccount
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);
		kit->SetAccount(String(account, accountBytes));
		return true;
	}
	EXPORT bool BurstLib_GetAccount Args_GetAccount
	{
		if (handle == NULL || returnBytes <= 0)
		return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		const String txt = kit->GetAccountRS();
		returnBytes = jmin(returnBytes, txt.length());
		memcpy(returnStr, txt.toRawUTF8(), returnBytes * sizeof(char));
		return true;
	}
	EXPORT bool BurstLib_GetJSONvalue Args_GetJSONvalue
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		const String returnString = kit->GetJSONvalue(
			String(jsonStr, jsonBytes),
			String(keyStr, keyBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	// EXT ====================================================================================
	EXPORT bool BurstLib_CloudCancel Args_CloudCancel
	{
		if (handle == NULL || returnBytes <= 0)
		return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		const String txt = kit->CloudCancel();
		returnBytes = jmin(returnBytes, txt.length());
		memcpy(returnStr, txt.toRawUTF8(), returnBytes * sizeof(char));
		return true;
	}
	EXPORT bool BurstLib_CloudDownloadStart Args_CloudDownloadStart
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);
						
		return (kit->CloudDownloadStart(
			String(cloudID, cloudIDBytes),
			String(dlFolder, dlFolderBytes)));
	}
	
	EXPORT bool BurstLib_CloudDownloadFinished Args_CloudDownloadFinished
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		// return datas
		String fileName;
		MemoryBlock data;
		String msgString;
		uint64 epochLast = 0;
		progress = 0.f;
		if (kit->CloudDownloadFinished(
			String(cloudID, cloudIDBytes),
			fileName,
			data,
			msgString, 
			epochLast,
			progress))
		{
			dlFilenameSize = fileName.copyToUTF8(dlFilename, dlFilenameSize);
			returnBytes = msgString.copyToUTF8(returnStr, returnBytes);

			if (dlData != 0 && (int)data.getSize() < dlDataSize)
				data.copyTo(dlData, 0, data.getSize());

			dlDataSize = data.getSize();
			epoch = epochLast;

			return true;
		}
		else return false;		
	}

	EXPORT bool BurstLib_CloudUploadStart Args_CloudUploadStart
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		uint64 deadline = 1440;

		String jobID = kit->CloudUploadStart(
			String(message, messageBytes),
			String(fileToUpload, fileToUploadBytes),
			deadline,
			stackSize,
			fee);
		if (jobID.isNotEmpty())
		{
			returnBytes = jobID.copyToUTF8(returnStr, returnBytes);
			return true;
		}
		return false;
	}

	EXPORT bool BurstLib_CloudUploadFinished Args_CloudUploadFinished
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		// return datas
		String cloudId;
		Array<StringArray> allAddresses;
		progress = 0.f;
		if (kit->CloudUploadFinished(
			String(jobId, jobIdBytes),
			cloudId,
			txFeePlancks,
			feePlancks,
			burnPlancks,
			confirmTime,
			progress))
		{
			returnBytes = cloudId.copyToUTF8(returnStr, returnBytes);
			costsNQT = txFeePlancks + feePlancks + burnPlancks;

			return true;
		}
		return false;
	}

	EXPORT bool BurstLib_CloudCalcCostsStart Args_CloudCalcCostsStart
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String jobID = kit->CloudCalcCostsStart(
			String(message, messageBytes),
			String(fileToUpload, fileToUploadBytes),
			stackSize,
			fee);
		if (jobID.isNotEmpty())
		{
			returnBytes = jobID.copyToUTF8(returnStr, returnBytes);

			return true;
		}
		return false;
	}

	EXPORT bool BurstLib_CloudCalcCostsFinished Args_CloudCalcCostsFinished
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		// return datas
		Array<StringArray> allAddresses;
		progress = 0.f;
		if (kit->CloudCalcCostsFinished(
			String(jobId, jobIdBytes),
			txFeePlancks,
			feePlancks,
			burnPlancks,
			confirmTime,
			progress))
		{
			costsNQT = txFeePlancks + feePlancks + burnPlancks;
			returnBytes = String(costsNQT).copyToUTF8(returnStr, returnBytes);

			return true;
		}
		return false;
	}

	EXPORT bool BurstLib_CreateCoupon Args_CreateCoupon
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->CreateCoupon(
			String(txSignedHexStr, txSignedHexBytes),
			String(passwordStr, passwordBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}
	EXPORT bool BurstLib_RedeemCoupon Args_RedeemCoupon
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->RedeemCoupon(
			String(couponHexStr, couponHexBytes),
			String(passwordStr, passwordBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}
	EXPORT bool BurstLib_ValidateCoupon Args_ValidateCoupon
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->ValidateCoupon(
			String(couponHexStr, couponHexBytes),
			String(passwordStr, passwordBytes),
			*valid);

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	// API ====================================================================================
	EXPORT bool BurstLib_broadcastTransaction Args_broadcastTransaction
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		const String returnString = kit->broadcastTransaction(
			String(signedTransactionBytesStr, signedTransactionBytesStrBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}
	EXPORT bool BurstLib_calculateFullHash Args_calculateFullHash
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		const String returnString = kit->calculateFullHash(
			String(unsignedTransactionBytesStr, unsignedTransactionBytesStrBytes),
			String(signatureHash, signatureHashBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}
	EXPORT bool BurstLib_decryptFrom Args_decryptFrom
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->decryptFrom(
			String(accountStr, accountBytes),
			String(dataStr, dataBytes),
			String(nonceStr, nonceBytes),
			String(decryptedMessageIsTextStr, decryptedMessageIsTextBytes),
			String(secretPhraseStr, secretPhraseBytes));
		
		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}
	EXPORT bool BurstLib_encryptTo Args_encryptTo
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->encryptTo(
			String(recipientStr, recipientBytes),
			String(messageToEncryptStr, messageToEncryptBytes),
			String(messageToEncryptIsTextStr, messageToEncryptIsTextBytes),
			String(secretPhraseStr, secretPhraseBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}
	EXPORT bool BurstLib_getAccount Args_getAccount
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getAccount(
			String(account_RS_or_IDStr, account_RS_or_IDBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}
	EXPORT bool BurstLib_getAccountBlockIds Args_getAccountBlockIds
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getAccountBlockIds(
			String(accountStr, accountBytes),
			String(timestampStr, timestampBytes),
			String(firstIndexStr, firstBytes),
			String(lastIndexStr, lastBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}
	EXPORT bool BurstLib_getAccountBlocks Args_getAccountBlocks
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getAccountBlocks(
			String(accountStr, accountBytes),
			String(timestampStr, timestampBytes),
			String(firstIndexStr, firstBytes),
			String(lastIndexStr, lastBytes),
			String(includeTransactions, includeTransactionsBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}
	EXPORT bool BurstLib_getAccountId Args_getAccountId
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getAccountId(
			String(pubKey_64HEX, pubKey_64HEXBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}
	EXPORT bool BurstLib_getAccountTransactionIds Args_getAccountTransactionIds
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getAccountTransactionIds(
			String(account, accountBytes),
			String(timestampStr, timestampBytes),
			String(type, typeBytes),
			String(subtype, subtypeBytes),
			String(firstIndex, firstIndexBytes),
			String(lastIndex, lastIndexBytes),
			String(numberOfConfirmations, numberOfConfirmationsBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}
	EXPORT bool BurstLib_getAccountPublicKey Args_getAccountPublicKey
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getAccountPublicKey(
			String(account, accountBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}
	EXPORT bool BurstLib_getAccountTransactions Args_getAccountTransactions
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getAccountTransactions(
			String(account, accountBytes),
			String(timestamp, timestampBytes),
			String(type, typeBytes),
			String(subtype, subtypeBytes),
			String(firstIndex, firstIndexBytes),
			String(lastIndex, lastIndexBytes),
			String(numberOfConfirmations, numberOfConfirmationsBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}
	EXPORT bool BurstLib_setAccountInfo Args_setAccountInfo
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		const String returnString = kit->setAccountInfo(
			String(name, nameBytes),
			String(description, descriptionBytes),
			String(feeNQT),
			String(deadlineMinutes),
			broadcast);

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}
	EXPORT bool BurstLib_getAlias Args_getAlias
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		const String returnString = kit->getAlias(
			String(alias, aliasBytes),
			String(aliasName, aliasNameBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}
	EXPORT bool BurstLib_setAlias Args_setAlias
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		const String returnString = kit->setAlias(
			String(aliasName, aliasNameBytes),
			String(aliasURI, aliasURIBytes),
			String(feeNQT),
			String(deadlineMinutes),
			broadcast);

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}	
	EXPORT bool BurstLib_getAliases Args_getAliases
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getAliases(
			String(timestamp, timestampBytes),
			String(account, accountBytes),
			String(firstIndex, firstIndexBytes),
			String(lastIndex, lastIndexBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}
	EXPORT bool BurstLib_buyAlias Args_buyAlias
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->buyAlias(
			String(alias, aliasBytes),
			String(aliasName, aliasNameBytes),
			String(amountNQT, amountNQTBytes),
			String(recipient, recipientBytes),
			String(feeNQT),
			String(deadlineMinutes),
			broadcast);

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}
	EXPORT bool BurstLib_sellAlias Args_sellAlias
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->sellAlias(
			String(alias, aliasBytes),
			String(aliasName, aliasNameBytes),
			String(priceNQT, priceNQTBytes),
			String(recipient, recipientBytes),
			String(feeNQT),
			String(deadlineMinutes),
			broadcast);

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}
	EXPORT bool BurstLib_getBalance Args_getBalance
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getBalance(
			String(account, accountBytes),
			String(includeEffectiveBalance, includeEffectiveBalanceBytes),
			String(height, heightBytes),
			String(requireBlock, requireBlockBytes),
			String(requireLastBlock, requireLastBlockBytes) );

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_getGuaranteedBalance Args_getGuaranteedBalance
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getGuaranteedBalance(
			String(account, accountBytes),
			String(numberOfConfirmations, numberOfConfirmationsBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_getTransaction Args_getTransaction
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getTransaction(
			String(transactionID, transactionIDBytes),
			String(fullHash, fullHashBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_getUnconfirmedTransactionsIds Args_getUnconfirmedTransactionsIds
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getUnconfirmedTransactionsIds(
			String(account, accountBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_getUnconfirmedTransactions Args_getUnconfirmedTransactions
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getUnconfirmedTransactions(
			String(account, accountBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_parseTransaction Args_parseTransaction
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->parseTransaction(
			String(transactionBytes, transactionBytesBytes),
			String(transactionJSON, transactionJSONBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_getTransactionBytes Args_getTransactionBytes
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getTransactionBytes(
			String(transactionID, transactionIDBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_sendMoney Args_sendMoney
	{
		if (handle == NULL)
		return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->sendMoney(
			String(recipient, recipientBytes),
			String(amountNQT),
			String(feeNQT),
			String(deadlineMinutes),
			broadcast);

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_sendMoneyMulti Args_sendMoneyMulti
	{
		if (handle == NULL)
		return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->sendMoneyMulti(
			StringArray::fromTokens(String(recipients, recipientsBytes), ";", ""),
			StringArray::fromTokens(String(amountsNQT, amountsNQTBytes), ";", ""),
			String(feeNQT),
			String(deadlineMinutes),
			broadcast);

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_sendMoneyMultiSame Args_sendMoneyMultiSame
	{
		if (handle == NULL)
		return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->sendMoneyMultiSame(
			StringArray::fromTokens(String(recipients, recipientsBytes), ";", ""),
			String(amountNQT),
			String(feeNQT),
			String(deadlineMinutes),
			broadcast);

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_readMessage Args_readMessage
	{
		if (handle == NULL)
		return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->readMessage(
			String(transactionID, transactionIDBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_sendMessage Args_sendMessage
	{
		if (handle == NULL)
		return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->sendMessage(
			String(message, messageBytes),
			String(messageIsText, messageIsTextBytes),
			String(messageToEncrypt, messageToEncryptBytes), 
			String(messageToEncryptIsText, messageToEncryptIsTextBytes), 
			String(encryptedMessageData, encryptedMessageDataBytes), 
			String(encryptedMessageNonce, encryptedMessageNonceBytes), 
			String(messageToEncryptToSelf, messageToEncryptToSelfBytes), 
			String(messageToEncryptToSelfIsText, messageToEncryptToSelfIsTextBytes), 
			String(encryptToSelfMessageData, encryptToSelfMessageDataBytes), 
			String(encryptToSelfMessageNonce, encryptToSelfMessageNonceBytes), 
			String(recipientPublicKey, recipientPublicKeyBytes), 
			String(feeNQT),
			String(deadlineMinutes),
			broadcast);

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_suggestFee Args_suggestFee
	{
		if (handle == NULL)
		return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->suggestFee();

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_getRewardRecipient Args_getRewardRecipient
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getRewardRecipient(
			String(account, accountBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_setRewardRecipient Args_setRewardRecipient
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->setRewardRecipient(
			String(recipient, recipientBytes),
			String(feeNQT),
			String(deadlineMinutes),
			broadcast);

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_getBlock Args_getBlock
	{
		if (handle == NULL)
		return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getBlock(
			String(block, blockBytes),
			String(height, heightBytes),
			String(timestamp, timestampBytes),
			String(includeTransactions, includeTransactionsBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_getBlockId Args_getBlockId
	{
		if (handle == NULL)
		return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getBlockId(
			String(height, heightBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_getBlockchainStatus Args_getBlockchainStatus
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getBlockchainStatus();

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_getBlocks Args_getBlocks
	{
		if (handle == NULL)
		return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getBlocks(
			String(firstIndex, firstIndexBytes),
			String(lastIndex, lastIndexBytes),
			String(includeTransactions, includeTransactionsBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}
	
	EXPORT bool BurstLib_getConstants Args_getConstants
	{
		if (handle == NULL)
		return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getConstants();

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_getECBlock Args_getECBlock
	{
		if (handle == NULL)
		return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getECBlock(
			String(timestamp, timestampBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}
	
	EXPORT bool BurstLib_getMiningInfo Args_getMiningInfo
	{
		if (handle == NULL)
		return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getMiningInfo();

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_getMyInfo Args_getMyInfo
	{
		if (handle == NULL)
		return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getMyInfo();

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_getPeer Args_getPeer
	{
		if (handle == NULL)
		return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getPeer(
			String(peer, peerBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_getPeers Args_getPeers
	{
		if (handle == NULL)
		return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getPeers(
			String(active, activeBytes),
			String(state, stateBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_getTime Args_getTime
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getTime();

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_getState Args_getState
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->getState(
			String(includeCounts, includeCountsBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_longConvert Args_longConvert
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->longConvert(
			String(id, idBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}

	EXPORT bool BurstLib_rsConvert Args_rsConvert
	{
		if (handle == NULL)
			return false;
		BurstExt* kit = reinterpret_cast<BurstExt*>(handle);

		String returnString = kit->rsConvert(
			String(account, accountBytes));

		returnBytes = returnString.copyToUTF8(returnStr, returnBytes);
		return true;
	}
}
