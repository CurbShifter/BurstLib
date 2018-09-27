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

#ifndef __BURSTKITC__
#define __BURSTKITC__

#ifdef __cplusplus
extern "C"
{
#endif	// __cplusplus

#if defined(WIN32) || defined(WIN64)
    #define SHOW
	#if defined(EXPORT_LIB)
        #define PREFIX_PORT __declspec( dllexport )
    #else
        #define PREFIX_PORT __declspec( dllimport )
    #endif
#elif __APPLE__
    #define SHOW
	#define PREFIX_PORT
#elif defined(__linux__)
    #define SHOW __attribute__((visibility("default")))
    #define PREFIX_PORT
#endif

typedef unsigned long long burstlibPtr;

// function arguments ==================================================================================
// defined to be reusable for defs for import / export and realtime loading of the dynlib
#define Args_GetHandle ()
#define Args_DeleteHandle (const burstlibPtr handle)
#define Args_GetBurstLibVersionNumber (const burstlibPtr handle)
#define Args_GetBurstLibVersionString (const burstlibPtr handle, char *returnStr, int &returnBytes)
#define Args_GetLastError (const burstlibPtr handle, char *returnStr, int &returnBytes)
#define Args_SetNode (const burstlibPtr handle, const char *hostUrl, const int hostUrlBytes)
#define Args_GetNode (const burstlibPtr handle, char *returnStr, int &returnBytes)
#define Args_SetSecretPhrase (const burstlibPtr handle, const char *passphrase, const int passphraseBytes)
#define Args_SetAccount (const burstlibPtr handle, const char *account, const int accountBytes)
#define Args_GetAccount (const burstlibPtr handle, char *returnStr, int &returnBytes)
#define Args_GetJSONvalue (const burstlibPtr handle, char *returnStr, int &returnBytes, char *jsonStr, int jsonBytes, char *keyStr, int keyBytes)

// Ext
#define Args_CloudDownload (const burstlibPtr handle, char *returnStr, int &returnBytes, char *cloudID, int cloudIDBytes, char *dlFolder, int dlFolderBytes, char *dlFilename, int &dlFilenameSize, void *dlData, int &dlDataSize)
#define Args_CloudCalcCosts (const burstlibPtr handle, char *returnStr, int &returnBytes, char *message, int messageBytes, char *fileToUpload, int fileToUploadBytes, const long long stackSize, const long long fee, long long &addressesNum, long long &txFeePlancks, long long &feePlancks, long long &burnPlancks, long long &costsNQT)
#define Args_CloudUpload (const burstlibPtr handle, char *returnStr, int &returnBytes, char *message, int messageBytes, char *fileToUpload, int fileToUploadBytes, const long long stackSize, const long long fee, long long &addressesNum, long long &txFeePlancks, long long &feePlancks, long long &burnPlancks, long long &costsNQT)

#define Args_CreateCoupon (const burstlibPtr handle, char *returnStr, int &returnBytes, char *txSignedHexStr, int txSignedHexBytes, char *passwordStr, int passwordBytes)
#define Args_RedeemCoupon (const burstlibPtr handle, char *returnStr, int &returnBytes, char *couponHexStr, int couponHexBytes, char *passwordStr, int passwordBytes)
#define Args_ValidateCoupon (const burstlibPtr handle, char *returnStr, int &returnBytes, char *couponHexStr, int couponHexBytes, char *passwordStr, int passwordBytes, bool *valid)

// BRS API
#define Args_broadcastTransaction (const burstlibPtr handle, char *returnStr, int &returnBytes, char *signedTransactionBytesStr, int signedTransactionBytesStrBytes)
#define Args_calculateFullHash (const burstlibPtr handle, char *returnStr, int &returnBytes, char *unsignedTransactionBytesStr, int unsignedTransactionBytesStrBytes, char *signatureHash, int signatureHashBytes)
#define Args_decryptFrom (const burstlibPtr handle, char *returnStr, int &returnBytes, char *accountStr, int accountBytes, char *dataStr, int dataBytes, char *nonceStr, int nonceBytes, char *decryptedMessageIsTextStr, int decryptedMessageIsTextBytes, char *secretPhraseStr, int secretPhraseBytes)
#define Args_encryptTo (const burstlibPtr handle, char *returnStr, int &returnBytes, char *recipientStr, int recipientBytes, char *messageToEncryptStr, int messageToEncryptBytes, char *messageToEncryptIsTextStr, int messageToEncryptIsTextBytes, char *secretPhraseStr, int secretPhraseBytes)
#define Args_getAccount (const burstlibPtr handle, char *returnStr, int &returnBytes, char *account_RS_or_IDStr, int account_RS_or_IDBytes)
#define Args_getAccountBlockIds (const burstlibPtr handle, char *returnStr, int &returnBytes, char *accountStr, int accountBytes, char *timestampStr, int timestampBytes, char *firstIndexStr, int firstBytes, char *lastIndexStr, int lastBytes)
#define Args_getAccountBlocks (const burstlibPtr handle, char *returnStr, int &returnBytes, char *accountStr, int accountBytes, char *timestampStr, int timestampBytes, char *firstIndexStr, int firstBytes, char *lastIndexStr, int lastBytes, char *includeTransactions, int includeTransactionsBytes)
#define Args_getAccountId (const burstlibPtr handle, char *returnStr, int &returnBytes, char *pubKey_64HEX, int pubKey_64HEXBytes)
#define Args_getAccountTransactionIds (const burstlibPtr handle, char *returnStr, int &returnBytes, char *account, int accountBytes, char *timestampStr, int timestampBytes, char *type, int typeBytes, char *subtype, int subtypeBytes, char *firstIndex, int firstIndexBytes, char *lastIndex, int lastIndexBytes, char *numberOfConfirmations, int numberOfConfirmationsBytes)
#define Args_getAccountPublicKey (const burstlibPtr handle, char *returnStr, int &returnBytes, char *account, int accountBytes)
#define Args_getAccountTransactions (const burstlibPtr handle, char *returnStr, int &returnBytes, char *account, int accountBytes, char *timestamp, int timestampBytes, char *type, int typeBytes, char *subtype, int subtypeBytes, char *firstIndex, int firstIndexBytes, char *lastIndex, int lastIndexBytes, char *numberOfConfirmations, int numberOfConfirmationsBytes)
#define Args_setAccountInfo (const burstlibPtr handle, char *returnStr, int &returnBytes, char *name, int nameBytes, char *description, int descriptionBytes, long long feeNQT, long long deadlineMinutes, bool broadcast)
#define Args_getAlias (const burstlibPtr handle, char *returnStr, int &returnBytes, char *alias, int aliasBytes, char *aliasName, int aliasNameBytes)
#define Args_setAlias (const burstlibPtr handle, char *returnStr, int &returnBytes, char *aliasName, int aliasNameBytes, char *aliasURI, int aliasURIBytes, long long feeNQT, long long deadlineMinutes, bool broadcast)
#define Args_getAliases (const burstlibPtr handle, char *returnStr, int &returnBytes, char *timestamp, int timestampBytes, char *account, int accountBytes, char *firstIndex, int firstIndexBytes, char *lastIndex, int lastIndexBytes)
#define Args_buyAlias (const burstlibPtr handle, char *returnStr, int &returnBytes, char *alias, int aliasBytes, char *aliasName, int aliasNameBytes, char *amountNQT, int amountNQTBytes, char *recipient, int recipientBytes, long long feeNQT, long long deadlineMinutes, bool broadcast)
#define Args_sellAlias (const burstlibPtr handle, char *returnStr, int &returnBytes, char *alias, int aliasBytes, char *aliasName, int aliasNameBytes, char *priceNQT, int priceNQTBytes, char *recipient, int recipientBytes, long long feeNQT, long long deadlineMinutes, bool broadcast)
#define Args_getBalance (const burstlibPtr handle, char *returnStr, int &returnBytes, char *account, int accountBytes, char *includeEffectiveBalance, int includeEffectiveBalanceBytes, char *height, int heightBytes, char *requireBlock, int requireBlockBytes, char *requireLastBlock, int requireLastBlockBytes)
#define Args_getGuaranteedBalance (const burstlibPtr handle, char *returnStr, int &returnBytes, char *account, int accountBytes, char *numberOfConfirmations, int numberOfConfirmationsBytes)
#define Args_getTransaction (const burstlibPtr handle, char *returnStr, int &returnBytes, char *transactionID, int transactionIDBytes, char *fullHash, int fullHashBytes)
#define Args_getUnconfirmedTransactionsIds (const burstlibPtr handle, char *returnStr, int &returnBytes, char *account, int accountBytes)
#define Args_getUnconfirmedTransactions (const burstlibPtr handle, char *returnStr, int &returnBytes, char *account, int accountBytes)
#define Args_parseTransaction (const burstlibPtr handle, char *returnStr, int &returnBytes, char *transactionBytes, int transactionBytesBytes, char *transactionJSON, int transactionJSONBytes)
#define Args_getTransactionBytes (const burstlibPtr handle, char *returnStr, int &returnBytes, char *transactionID, int transactionIDBytes)
#define Args_sendMoney (const burstlibPtr handle, char *returnStr, int &returnBytes, char *recipient, int recipientBytes, long long amountNQT, long long feeNQT, long long deadlineMinutes, bool broadcast)
#define Args_sendMoneyMulti (const burstlibPtr handle, char *returnStr, int &returnBytes, char *recipients, int recipientsBytes, char *amountsNQT, int amountsNQTBytes, long long feeNQT, long long deadlineMinutes, bool broadcast)
#define Args_sendMoneyMultiSame (const burstlibPtr handle, char *returnStr, int &returnBytes, char *recipients, int recipientsBytes, long long amountNQT, long long feeNQT, long long deadlineMinutes, bool broadcast)
#define Args_readMessage (const burstlibPtr handle, char *returnStr, int &returnBytes, char *transactionID, int transactionIDBytes)
#define Args_sendMessage (const burstlibPtr handle, char *returnStr, int &returnBytes, char *message, int messageBytes, char *messageIsText, int messageIsTextBytes, char *messageToEncrypt, int messageToEncryptBytes, char *messageToEncryptIsText, int messageToEncryptIsTextBytes, char *encryptedMessageData, int encryptedMessageDataBytes, char *encryptedMessageNonce, int encryptedMessageNonceBytes, char *messageToEncryptToSelf, int messageToEncryptToSelfBytes, char *messageToEncryptToSelfIsText, int messageToEncryptToSelfIsTextBytes, char *encryptToSelfMessageData, int encryptToSelfMessageDataBytes, char *encryptToSelfMessageNonce, int encryptToSelfMessageNonceBytes, char *recipientPublicKey, int recipientPublicKeyBytes, long long feeNQT, long long deadlineMinutes, bool broadcast)
#define Args_suggestFee (const burstlibPtr handle, char *returnStr, int &returnBytes)
#define Args_getRewardRecipient (const burstlibPtr handle, char *returnStr, int &returnBytes, char *account, int accountBytes)
#define Args_setRewardRecipient (const burstlibPtr handle, char *returnStr, int &returnBytes, char *recipient, int recipientBytes, long long feeNQT, long long deadlineMinutes, bool broadcast)
#define Args_getBlock (const burstlibPtr handle, char *returnStr, int &returnBytes, char *block, int blockBytes, char *height, int heightBytes, char *timestamp, int timestampBytes, char *includeTransactions, int includeTransactionsBytes)
#define Args_getBlockId (const burstlibPtr handle, char *returnStr, int &returnBytes, char *height, int heightBytes)
#define Args_getBlockchainStatus (const burstlibPtr handle, char *returnStr, int &returnBytes)
#define Args_getBlocks (const burstlibPtr handle, char *returnStr, int &returnBytes, char *firstIndex, int firstIndexBytes, char *lastIndex, int lastIndexBytes, char *includeTransactions, int includeTransactionsBytes)
#define Args_getConstants (const burstlibPtr handle, char *returnStr, int &returnBytes)
#define Args_getECBlock (const burstlibPtr handle, char *returnStr, int &returnBytes, char *timestamp, int timestampBytes)
#define Args_getMiningInfo (const burstlibPtr handle, char *returnStr, int &returnBytes)
#define Args_getMyInfo (const burstlibPtr handle, char *returnStr, int &returnBytes)
#define Args_getPeer (const burstlibPtr handle, char *returnStr, int &returnBytes, char *peer, int peerBytes)
#define Args_getPeers (const burstlibPtr handle, char *returnStr, int &returnBytes, char *active, int activeBytes, char *state, int stateBytes)
#define Args_getTime (const burstlibPtr handle, char *returnStr, int &returnBytes)
#define Args_getState (const burstlibPtr handle, char *returnStr, int &returnBytes, char *includeCounts, int includeCountsBytes)
#define Args_longConvert (const burstlibPtr handle, char *returnStr, int &returnBytes, char *id, int idBytes)
#define Args_rsConvert (const burstlibPtr handle, char *returnStr, int &returnBytes, char *account, int accountBytes)


// IMPORT EXPORT =======================================================================================
#if defined(IMPORT_LIB) || defined(EXPORT_LIB)

PREFIX_PORT burstlibPtr SHOW BurstLib_GetHandle Args_GetHandle;
PREFIX_PORT bool SHOW BurstLib_DeleteHandle Args_DeleteHandle;
PREFIX_PORT int SHOW BurstLib_GetBurstLibVersionNumber Args_GetBurstLibVersionNumber;
PREFIX_PORT bool SHOW BurstLib_GetBurstLibVersionString Args_GetBurstLibVersionString;
PREFIX_PORT bool SHOW BurstLib_GetLastError Args_GetLastError;
PREFIX_PORT bool SHOW BurstLib_SetNode Args_SetNode;
PREFIX_PORT bool SHOW BurstLib_GetNode Args_GetNode;
PREFIX_PORT bool SHOW BurstLib_SetSecretPhrase Args_SetSecretPhrase;
PREFIX_PORT bool SHOW BurstLib_SetAccount Args_SetAccount;
PREFIX_PORT bool SHOW BurstLib_GetAccount Args_GetAccount;
PREFIX_PORT bool SHOW BurstLib_GetJSONvalue Args_GetJSONvalue;

// Ext
PREFIX_PORT bool SHOW BurstLib_CloudDownload Args_CloudDownload;
PREFIX_PORT bool SHOW BurstLib_CloudCalcCosts Args_CloudCalcCosts;
PREFIX_PORT bool SHOW BurstLib_CloudUpload Args_CloudUpload;

PREFIX_PORT bool SHOW BurstLib_CreateCoupon Args_CreateCoupon;
PREFIX_PORT bool SHOW BurstLib_RedeemCoupon Args_RedeemCoupon;
PREFIX_PORT bool SHOW BurstLib_ValidateCoupon Args_ValidateCoupon;

// API
PREFIX_PORT bool SHOW BurstLib_broadcastTransaction Args_broadcastTransaction;
PREFIX_PORT bool SHOW BurstLib_calculateFullHash Args_calculateFullHash;
PREFIX_PORT bool SHOW BurstLib_decryptFrom Args_decryptFrom;
PREFIX_PORT bool SHOW BurstLib_encryptTo Args_encryptTo;
PREFIX_PORT bool SHOW BurstLib_getAccount Args_getAccount;
PREFIX_PORT bool SHOW BurstLib_getAccountBlockIds Args_getAccountBlockIds;
PREFIX_PORT bool SHOW BurstLib_getAccountBlocks Args_getAccountBlocks;
PREFIX_PORT bool SHOW BurstLib_getAccountId Args_getAccountId;
PREFIX_PORT bool SHOW BurstLib_getAccountTransactionIds Args_getAccountTransactionIds;
PREFIX_PORT bool SHOW BurstLib_getAccountPublicKey Args_getAccountPublicKey;
PREFIX_PORT bool SHOW BurstLib_getAccountTransactions Args_getAccountTransactions;
PREFIX_PORT bool SHOW BurstLib_setAccountInfo Args_setAccountInfo;
PREFIX_PORT bool SHOW BurstLib_getAlias Args_getAlias;
PREFIX_PORT bool SHOW BurstLib_setAlias Args_setAlias;
PREFIX_PORT bool SHOW BurstLib_getAliases Args_getAliases;
PREFIX_PORT bool SHOW BurstLib_buyAlias Args_buyAlias;
PREFIX_PORT bool SHOW BurstLib_sellAlias Args_sellAlias;
PREFIX_PORT bool SHOW BurstLib_getBalance Args_getBalance;
PREFIX_PORT bool SHOW BurstLib_getGuaranteedBalance Args_getGuaranteedBalance;
PREFIX_PORT bool SHOW BurstLib_getTransaction Args_getTransaction;
PREFIX_PORT bool SHOW BurstLib_getUnconfirmedTransactionsIds Args_getUnconfirmedTransactionsIds;
PREFIX_PORT bool SHOW BurstLib_getUnconfirmedTransactions Args_getUnconfirmedTransactions;
PREFIX_PORT bool SHOW BurstLib_parseTransaction Args_parseTransaction;
PREFIX_PORT bool SHOW BurstLib_getTransactionBytes Args_getTransactionBytes;
PREFIX_PORT bool SHOW BurstLib_sendMoney Args_sendMoney;
PREFIX_PORT bool SHOW BurstLib_sendMoneyMulti Args_sendMoneyMulti;
PREFIX_PORT bool SHOW BurstLib_sendMoneyMultiSame Args_sendMoneyMultiSame;
PREFIX_PORT bool SHOW BurstLib_readMessage Args_readMessage;
PREFIX_PORT bool SHOW BurstLib_sendMessage Args_sendMessage;
PREFIX_PORT bool SHOW BurstLib_suggestFee Args_suggestFee;
PREFIX_PORT bool SHOW BurstLib_getRewardRecipient Args_getRewardRecipient;
PREFIX_PORT bool SHOW BurstLib_setRewardRecipient Args_setRewardRecipient;
PREFIX_PORT bool SHOW BurstLib_getBlock Args_getBlock;
PREFIX_PORT bool SHOW BurstLib_getBlockId Args_getBlockId;
PREFIX_PORT bool SHOW BurstLib_getBlockchainStatus Args_getBlockchainStatus;
PREFIX_PORT bool SHOW BurstLib_getBlocks Args_getBlocks;
PREFIX_PORT bool SHOW BurstLib_getConstants Args_getConstants;
PREFIX_PORT bool SHOW BurstLib_getECBlock Args_getECBlock;
PREFIX_PORT bool SHOW BurstLib_getMiningInfo Args_getMiningInfo;
PREFIX_PORT bool SHOW BurstLib_getMyInfo Args_getMyInfo;
PREFIX_PORT bool SHOW BurstLib_getPeer Args_getPeer;
PREFIX_PORT bool SHOW BurstLib_getPeers Args_getPeers;
PREFIX_PORT bool SHOW BurstLib_getTime Args_getTime;
PREFIX_PORT bool SHOW BurstLib_getState Args_getState;
PREFIX_PORT bool SHOW BurstLib_longConvert Args_longConvert;
PREFIX_PORT bool SHOW BurstLib_rsConvert Args_rsConvert;

#else
// Typedefs =============================================================================================
typedef burstlibPtr(*BurstLib_GetHandle_Ptr) Args_GetHandle;
typedef bool(*BurstLib_DeleteHandle_Ptr) Args_DeleteHandle;
typedef int(*BurstLib_GetBurstLibVersionNumber_Ptr) Args_GetBurstLibVersionNumber;
typedef bool(*BurstLib_GetBurstLibVersionString_Ptr) Args_GetBurstLibVersionString;
typedef bool(*BurstLib_GetLastError_Ptr) Args_GetLastError;
typedef bool(*BurstLib_SetNode_Ptr) Args_SetNode;
typedef bool(*BurstLib_GetNode_Ptr) Args_GetNode;
typedef bool(*BurstLib_SetSecretPhrase_Ptr) Args_SetSecretPhrase;
typedef bool(*BurstLib_SetAccount_Ptr) Args_SetAccount;
typedef bool(*BurstLib_GetAccount_Ptr) Args_GetAccount;
typedef bool(*BurstLib_GetJSONvalue_Ptr) Args_GetJSONvalue;

// Ext
typedef bool(*BurstLib_CloudDownload_Ptr) Args_CloudDownload;
typedef bool(*BurstLib_CloudCalcCosts_Ptr) Args_CloudCalcCosts;
typedef bool(*BurstLib_CloudUpload_Ptr) Args_CloudUpload;

typedef bool(*BurstLib_CreateCoupon_Ptr) Args_CreateCoupon;
typedef bool(*BurstLib_RedeemCoupon_Ptr) Args_RedeemCoupon;
typedef bool(*BurstLib_ValidateCoupon_Ptr) Args_ValidateCoupon;

// API
typedef bool(*BurstLib_broadcastTransaction_Ptr) Args_broadcastTransaction;
typedef bool(*BurstLib_calculateFullHash_Ptr) Args_calculateFullHash;
typedef bool(*BurstLib_decryptFrom_Ptr) Args_decryptFrom;
typedef bool(*BurstLib_encryptTo_Ptr) Args_encryptTo;
typedef bool(*BurstLib_getAccount_Ptr) Args_getAccount;
typedef bool(*BurstLib_getAccountBlockIds_Ptr) Args_getAccountBlockIds;
typedef bool(*BurstLib_getAccountBlocks_Ptr) Args_getAccountBlocks;
typedef bool(*BurstLib_getAccountId_Ptr) Args_getAccountId;
typedef bool(*BurstLib_getAccountTransactionIds_Ptr) Args_getAccountTransactionIds;
typedef bool(*BurstLib_getAccountPublicKey_Ptr) Args_getAccountPublicKey;
typedef bool(*BurstLib_getAccountTransactions_Ptr) Args_getAccountTransactions;
typedef bool(*BurstLib_setAccountInfo_Ptr) Args_setAccountInfo;
typedef bool(*BurstLib_getAlias_Ptr) Args_getAlias;
typedef bool(*BurstLib_setAlias_Ptr) Args_setAlias;
typedef bool(*BurstLib_getAliases_Ptr) Args_getAliases;
typedef bool(*BurstLib_buyAlias_Ptr) Args_buyAlias;
typedef bool(*BurstLib_sellAlias_Ptr) Args_sellAlias;
typedef bool(*BurstLib_getBalance_Ptr) Args_getBalance;
typedef bool(*BurstLib_getGuaranteedBalance_Ptr) Args_getGuaranteedBalance;
typedef bool(*BurstLib_getTransaction_Ptr) Args_getTransaction;
typedef bool(*BurstLib_getUnconfirmedTransactionsIds_Ptr) Args_getUnconfirmedTransactionsIds;
typedef bool(*BurstLib_getUnconfirmedTransactions_Ptr) Args_getUnconfirmedTransactions;
typedef bool(*BurstLib_parseTransaction_Ptr) Args_parseTransaction;
typedef bool(*BurstLib_getTransactionBytes_Ptr) Args_getTransactionBytes;
typedef bool(*BurstLib_sendMoney_Ptr) Args_sendMoney;
typedef bool(*BurstLib_sendMoneyMulti_Ptr) Args_sendMoneyMulti;
typedef bool(*BurstLib_sendMoneyMultiSame_Ptr) Args_sendMoneyMultiSame;
typedef bool(*BurstLib_readMessage_Ptr) Args_readMessage;
typedef bool(*BurstLib_sendMessage_Ptr) Args_sendMessage;
typedef bool(*BurstLib_suggestFee_Ptr) Args_suggestFee;
typedef bool(*BurstLib_getRewardRecipient_Ptr) Args_getRewardRecipient;
typedef bool(*BurstLib_setRewardRecipient_Ptr) Args_setRewardRecipient;
typedef bool(*BurstLib_getBlock_Ptr) Args_getBlock;
typedef bool(*BurstLib_getBlockId_Ptr) Args_getBlockId;
typedef bool(*BurstLib_getBlockchainStatus_Ptr) Args_getBlockchainStatus;
typedef bool(*BurstLib_getBlocks_Ptr) Args_getBlocks;
typedef bool(*BurstLib_getConstants_Ptr) Args_getConstants;
typedef bool(*BurstLib_getECBlock_Ptr) Args_getECBlock;
typedef bool(*BurstLib_getMiningInfo_Ptr) Args_getMiningInfo;
typedef bool(*BurstLib_getMyInfo_Ptr) Args_getMyInfo;
typedef bool(*BurstLib_getPeer_Ptr) Args_getPeer;
typedef bool(*BurstLib_getPeers_Ptr) Args_getPeers;
typedef bool(*BurstLib_getTime_Ptr) Args_getTime;
typedef bool(*BurstLib_getState_Ptr) Args_getState;
typedef bool(*BurstLib_longConvert_Ptr) Args_longConvert;
typedef bool(*BurstLib_rsConvert_Ptr) Args_rsConvert;

// Struct to hold the function pointers =================================================================
struct BurstLib_FunctionHandles
{
	BurstLib_GetHandle_Ptr GetHandle;
	BurstLib_DeleteHandle_Ptr DeleteHandle;
	BurstLib_GetBurstLibVersionNumber_Ptr GetBurstLibVersionNumber;
	BurstLib_GetBurstLibVersionString_Ptr GetBurstLibVersionString;
	BurstLib_GetLastError_Ptr GetLastError;
	BurstLib_SetNode_Ptr SetNode;
	BurstLib_GetNode_Ptr GetNode;
	BurstLib_SetSecretPhrase_Ptr SetSecretPhrase;
	BurstLib_SetAccount_Ptr SetAccount;
	BurstLib_GetAccount_Ptr GetAccount;
	BurstLib_GetJSONvalue_Ptr GetJSONvalue;

	// Ext
	BurstLib_CloudDownload_Ptr CloudDownload;
	BurstLib_CloudCalcCosts_Ptr CloudCalcCosts;
	BurstLib_CloudUpload_Ptr CloudUpload;

	BurstLib_CreateCoupon_Ptr CreateCoupon;
	BurstLib_RedeemCoupon_Ptr RedeemCoupon;
	BurstLib_ValidateCoupon_Ptr ValidateCoupon;

	// API
	BurstLib_broadcastTransaction_Ptr broadcastTransaction;
	BurstLib_calculateFullHash_Ptr calculateFullHash;
	BurstLib_decryptFrom_Ptr decryptFrom;
	BurstLib_encryptTo_Ptr encryptTo;
	BurstLib_getAccount_Ptr getAccount;
	BurstLib_getAccountBlockIds_Ptr getAccountBlockIds;
	BurstLib_getAccountBlocks_Ptr getAccountBlocks;
	BurstLib_getAccountId_Ptr getAccountId;
	BurstLib_getAccountTransactionIds_Ptr getAccountTransactionIds;
	BurstLib_getAccountPublicKey_Ptr getAccountPublicKey;
	BurstLib_getAccountTransactions_Ptr getAccountTransactions;
	BurstLib_setAccountInfo_Ptr setAccountInfo;
	BurstLib_getAlias_Ptr getAlias;
	BurstLib_setAlias_Ptr setAlias;
	BurstLib_getAliases_Ptr getAliases;
	BurstLib_buyAlias_Ptr buyAlias;
	BurstLib_sellAlias_Ptr sellAlias;
	BurstLib_getBalance_Ptr getBalance;
	BurstLib_getGuaranteedBalance_Ptr getGuaranteedBalance;
	BurstLib_getTransaction_Ptr getTransaction;
	BurstLib_getUnconfirmedTransactionsIds_Ptr getUnconfirmedTransactionsIds;
	BurstLib_getUnconfirmedTransactions_Ptr getUnconfirmedTransactions;
	BurstLib_parseTransaction_Ptr parseTransaction;
	BurstLib_getTransactionBytes_Ptr getTransactionBytes;
	BurstLib_sendMoney_Ptr sendMoney;
	BurstLib_sendMoneyMulti_Ptr sendMoneyMulti;
	BurstLib_sendMoneyMultiSame_Ptr sendMoneyMultiSame;
	BurstLib_readMessage_Ptr readMessage;
	BurstLib_sendMessage_Ptr sendMessage;
	BurstLib_suggestFee_Ptr suggestFee;
	BurstLib_getRewardRecipient_Ptr getRewardRecipient;
	BurstLib_setRewardRecipient_Ptr setRewardRecipient;
	BurstLib_getBlock_Ptr getBlock;
	BurstLib_getBlockId_Ptr getBlockId;
	BurstLib_getBlockchainStatus_Ptr getBlockchainStatus;
	BurstLib_getBlocks_Ptr getBlocks;
	BurstLib_getConstants_Ptr getConstants;
	BurstLib_getECBlock_Ptr getECBlock;
	BurstLib_getMiningInfo_Ptr getMiningInfo;
	BurstLib_getMyInfo_Ptr getMyInfo;
	BurstLib_getPeer_Ptr getPeer;
	BurstLib_getPeers_Ptr getPeers;
	BurstLib_getTime_Ptr getTime;
	BurstLib_getState_Ptr getState;
	BurstLib_longConvert_Ptr longConvert;
	BurstLib_rsConvert_Ptr rsConvert;
};

#if defined(WIN32) || defined(WIN64)
	#include <windows.h>
	#define GETHANDLE_FUNC GetProcAddress
#elif defined(__APPLE__)
	// .dylib library loader
	#include <dlfcn.h>
	#define GETHANDLE_FUNC CreateDylibHandle
	// helper function to create the pointers to the functions in the library
	void* CreateDylibHandle(void *lib_handle, const char *functionname)
	{
		void *ambigious = (void *)dlsym(lib_handle, functionname);
		char *error = 0;
		if ((error = dlerror()) != 0)
		{
			//printf("%s\n", errorSO);
			return 0;
		}
		return ambigious;
	}
#elif defined(__linux__)
	// .so library loader
	#include <dlfcn.h>
	#define GETHANDLE_FUNC CreateSOHandle

	// helper function to create the pointers to the functions in the library
	void* CreateSOHandle(void *lib_handle, const char *functionname)
	{
		void *ambigious = (void *)dlsym(lib_handle, functionname);
		char *errorSO = 0;
		if((errorSO = dlerror()) != 0)
		{ //printf("%s\n", errorSO);
			return 0;
		}
		return ambigious;
	}
#endif

#if defined(WIN32) || defined(WIN64)
#if defined(UNICODE) || defined(NOUNICODE)
	wchar_t *convertCharArrayToLPCWSTR(const char* charArray)
	{
		wchar_t* wString=new wchar_t[4096];
		MultiByteToWideChar(CP_ACP, 0, charArray, -1, wString, 4096);
		return wString;
	}
#endif // defined(UNICODE) || defined(NOUNICODE)
HMODULE InitFunctions(const char *libPath, BurstLib_FunctionHandles *burstLib)
#elif defined(__APPLE__)
void* InitFunctions(const char *libPath, BurstLib_FunctionHandles *burstLib)
#elif defined(__linux__)
void* InitFunctions(const char *libPath, BurstLib_FunctionHandles *burstLib)
#endif
{ // handle to loaded library module, returns a handle to the DLL, otherwise NULL
#if defined(WIN32) || defined(_WIN64)
#if defined(UNICODE) || defined(NOUNICODE)
	wchar_t *lpcwstr = convertCharArrayToLPCWSTR(libPath);
	HMODULE lib_handle = LoadLibrary(lpcwstr);
	delete lpcwstr;
#else
	HMODULE lib_handle = LoadLibrary(libPath);
#endif
#elif defined(__APPLE__)
	void *lib_handle = dlopen(libPath, RTLD_LAZY);
	if (!lib_handle) {
		//printf("%s\n", dlerror()); // #include <iostream>
		return 0;
	}
#elif defined(__linux__)
	//void *lib_handle = dlmopen(LM_ID_NEWLM, libPath, RTLD_LAZY | RTLD_LOCAL);
	void *lib_handle = dlopen(libPath, RTLD_LAZY | RTLD_LOCAL);
	if(!lib_handle)
	{ //printf("%s\n", dlerror()); // #include <iostream>
		return 0;
	}
#endif
	// returns the address of the DLL functions, otherwise NULL
	burstLib->GetHandle = (BurstLib_GetHandle_Ptr) GETHANDLE_FUNC(lib_handle, "BurstLib_GetHandle");
	burstLib->DeleteHandle = (BurstLib_DeleteHandle_Ptr) GETHANDLE_FUNC(lib_handle, "BurstLib_DeleteHandle");
	burstLib->GetBurstLibVersionNumber = (BurstLib_GetBurstLibVersionNumber_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_GetBurstLibVersionNumber");
	burstLib->GetBurstLibVersionString = (BurstLib_GetBurstLibVersionString_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_GetBurstLibVersionString");
	burstLib->GetLastError = (BurstLib_GetLastError_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_GetLastError");
	burstLib->SetNode = (BurstLib_SetNode_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_SetNode");
	burstLib->GetNode = (BurstLib_GetNode_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_GetNode");
	burstLib->SetSecretPhrase = (BurstLib_SetSecretPhrase_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_SetSecretPhrase");
	burstLib->SetAccount = (BurstLib_SetAccount_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_SetAccount");
	burstLib->GetAccount = (BurstLib_GetAccount_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_GetAccount");
	burstLib->GetJSONvalue = (BurstLib_GetJSONvalue_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_GetJSONvalue");

	// Ext
	burstLib->CloudDownload = (BurstLib_CloudDownload_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_CloudDownload");
	burstLib->CloudCalcCosts = (BurstLib_CloudCalcCosts_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_CloudCalcCosts");
	burstLib->CloudUpload = (BurstLib_CloudUpload_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_CloudUpload");

	burstLib->CreateCoupon = (BurstLib_CreateCoupon_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_CreateCoupon");
	burstLib->RedeemCoupon = (BurstLib_RedeemCoupon_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_RedeemCoupon");
	burstLib->ValidateCoupon = (BurstLib_ValidateCoupon_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_ValidateCoupon");

	// API
	burstLib->broadcastTransaction = (BurstLib_broadcastTransaction_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_broadcastTransaction");
	burstLib->calculateFullHash = (BurstLib_calculateFullHash_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_calculateFullHash");
	burstLib->decryptFrom = (BurstLib_decryptFrom_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_decryptFrom");
	burstLib->encryptTo = (BurstLib_encryptTo_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_encryptTo");
	burstLib->getAccount = (BurstLib_getAccount_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getAccount");
	burstLib->getAccountBlockIds = (BurstLib_getAccountBlockIds_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getAccountBlockIds");
	burstLib->getAccountBlocks = (BurstLib_getAccountBlocks_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getAccountBlocks");
	burstLib->getAccountId = (BurstLib_getAccountId_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getAccountId");
	burstLib->getAccountTransactionIds = (BurstLib_getAccountTransactionIds_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getAccountTransactionIds");
	burstLib->getAccountPublicKey = (BurstLib_getAccountPublicKey_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getAccountPublicKey");
	burstLib->getAccountTransactions = (BurstLib_getAccountTransactions_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getAccountTransactions");
	burstLib->setAccountInfo = (BurstLib_setAccountInfo_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_setAccountInfo");
	burstLib->getAlias = (BurstLib_getAlias_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getAlias");
	burstLib->setAlias = (BurstLib_setAlias_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_setAlias");
	burstLib->getAliases = (BurstLib_getAliases_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getAliases");
	burstLib->buyAlias = (BurstLib_buyAlias_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_buyAlias");
	burstLib->sellAlias = (BurstLib_sellAlias_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_sellAlias");
	burstLib->getBalance = (BurstLib_getBalance_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getBalance");
	burstLib->getGuaranteedBalance = (BurstLib_getGuaranteedBalance_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getGuaranteedBalance");
	burstLib->getTransaction = (BurstLib_getTransaction_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getTransaction");
	burstLib->getUnconfirmedTransactionsIds = (BurstLib_getUnconfirmedTransactionsIds_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getUnconfirmedTransactionsIds");
	burstLib->getUnconfirmedTransactions = (BurstLib_getUnconfirmedTransactions_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getUnconfirmedTransactions");
	burstLib->parseTransaction = (BurstLib_parseTransaction_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_parseTransaction");
	burstLib->getTransactionBytes = (BurstLib_getTransactionBytes_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getTransactionBytes");
	burstLib->sendMoney = (BurstLib_sendMoney_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_sendMoney");
	burstLib->sendMoneyMulti = (BurstLib_sendMoneyMulti_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_sendMoneyMulti");
	burstLib->sendMoneyMultiSame = (BurstLib_sendMoneyMultiSame_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_sendMoneyMultiSame");
	burstLib->readMessage = (BurstLib_readMessage_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_readMessage");
	burstLib->sendMessage = (BurstLib_sendMessage_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_sendMessage");
	burstLib->suggestFee = (BurstLib_suggestFee_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_suggestFee");
	burstLib->getRewardRecipient = (BurstLib_getRewardRecipient_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getRewardRecipient");
	burstLib->setRewardRecipient = (BurstLib_setRewardRecipient_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_setRewardRecipient");
	burstLib->getBlock = (BurstLib_getBlock_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getBlock");
	burstLib->getBlockId = (BurstLib_getBlockId_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getBlockId");
	burstLib->getBlockchainStatus = (BurstLib_getBlockchainStatus_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getBlockchainStatus");
	burstLib->getBlocks = (BurstLib_getBlocks_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getBlocks");
	burstLib->getConstants = (BurstLib_getConstants_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getConstants");
	burstLib->getECBlock = (BurstLib_getECBlock_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getECBlock");
	burstLib->getMiningInfo = (BurstLib_getMiningInfo_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getMiningInfo");
	burstLib->getMyInfo = (BurstLib_getMyInfo_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getMyInfo");
	burstLib->getPeer = (BurstLib_getPeer_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getPeer");
	burstLib->getPeers = (BurstLib_getPeers_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getPeers");
	burstLib->getTime = (BurstLib_getTime_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getTime");
	burstLib->getState = (BurstLib_getState_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_getState");
	burstLib->longConvert = (BurstLib_longConvert_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_longConvert");
	burstLib->rsConvert = (BurstLib_rsConvert_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_rsConvert");

	return lib_handle;
};

#if defined(_WIN32) || defined(_WIN64)
void *BurstLib_LoadDLL(const char *libPath, BurstLib_FunctionHandles *burstLib)
{
	void *dll_handle = (HMODULE*)malloc(sizeof(HMODULE));
	*((HMODULE*)dll_handle) = InitFunctions(libPath, burstLib);
	if (!(burstLib->GetHandle)) {// check if function pointer is not 0
		//printf("Failed to load the functions from the BurstLib dynamic library. Library file not found?\n\n");
		free((HMODULE)dll_handle);
		return 0;
	}
	return dll_handle;
}
void BurstLib_UnloadDLL(void *dll_handle)
{   // close the dynamicly loaded library
	if (dll_handle)
	{
		HMODULE temp = *((HMODULE*)dll_handle);
		BOOL bRes = FreeLibrary(temp); // returns nonzero if successfull
		LPVOID lpMsgBuf;
		if (!bRes){
			DWORD dw = GetLastError();

			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				dw,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR)&lpMsgBuf,
				0, NULL);
		}
		free((HMODULE)dll_handle);
	}
}

#elif defined (__APPLE__)
void *BurstLib_LoadDLL(const char *libPath, BurstLib_FunctionHandles *burstLib)
{
	void *dll_handle = InitFunctions(libPath, burstLib);
	if (!dll_handle){
		//printf("Failed to load the functions from the BurstLib dynamic library. Library file not found?\n\n");
		return 0;
	}
	return dll_handle;
}
void BurstLib_UnloadDLL(void *dll_handle)
{   // close the dynamicly loaded library
	if (dll_handle)
		dlclose(dll_handle);
}

#elif defined (__linux__)
void *BurstLib_LoadDLL(const char *libPath, BurstLib_FunctionHandles *burstLib)
{
	void *dll_handle = InitFunctions(libPath, burstLib);
	if(!dll_handle)
	{ //printf("Failed to load the functions from the BurstLib dynamic library. Library file not found?\n\n");
		return 0;
	}
	return dll_handle;
}

void BurstLib_UnloadDLL(void *dll_handle)
{ // close the dynamicly loaded library
	if(dll_handle)
		dlclose(dll_handle);
}
#endif // platform
#endif // IMPORT EXPORT
#ifdef __cplusplus
}		// extern "C"
#endif	// __cplusplus

#endif // __BURSTKITC__
