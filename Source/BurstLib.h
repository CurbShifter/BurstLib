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
#define Args_GetLastError (const burstlibPtr handle, char *returnStr, int &returnBytes)
#define Args_SetNode (const burstlibPtr handle, const char *hostUrl, const int hostUrlBytes)
#define Args_GetNode (const burstlibPtr handle, char *returnStr, int &returnBytes)
#define Args_SetSecretPhrase (const burstlibPtr handle, const char *passphrase, const int passphraseBytes)
#define Args_SetAccount (const burstlibPtr handle, const char *account, const int accountBytes)
#define Args_GetAccount (const burstlibPtr handle, char *returnStr, int &returnBytes)
#define Args_GetJSONvalue (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *jsonStr, const int jsonBytes, const char *keyStr, const int keyBytes)

// Ext
#define Args_CloudDownloadStart (const burstlibPtr handle, const char *cloudID, const int cloudIDBytes, const char *dlFolder, const int dlFolderBytes)
#define Args_CloudDownloadFinished (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *cloudID, const int cloudIDBytes, char *dlFilename, int &dlFilenameSize, void *dlData, int &dlDataSize, unsigned long long &epoch, float &progress)
#define Args_CloudUploadStart (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *message, const int messageBytes, const char *fileToUpload, const int fileToUploadBytes, const unsigned long long stackSize, const unsigned long long fee)
#define Args_CloudUploadFinished (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *jobId, const int jobIdBytes, unsigned long long &txFeePlancks, unsigned long long &feePlancks, unsigned long long &burnPlancks, unsigned long long &costsNQT, unsigned long long &confirmTime, float &progress)
#define Args_CloudCalcCostsStart (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *message, const int messageBytes, const char *fileToUpload, const int fileToUploadBytes, const unsigned long long stackSize, const unsigned long long fee)
#define Args_CloudCalcCostsFinished (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *jobId, const int jobIdBytes, unsigned long long &txFeePlancks, unsigned long long &feePlancks, unsigned long long &burnPlancks, unsigned long long &costsNQT, unsigned long long &confirmTime, float &progress)
#define Args_CloudCancel (const burstlibPtr handle, char *returnStr, int &returnBytes)

#define Args_CreateCoupon (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *txSignedHexStr, const int txSignedHexBytes, const char *passwordStr, const int passwordBytes)
#define Args_RedeemCoupon (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *couponHexStr, const int couponHexBytes, const char *passwordStr, const int passwordBytes)
#define Args_ValidateCoupon (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *couponHexStr, const int couponHexBytes, const char *passwordStr, const int passwordBytes, bool *valid)

// BRS API
#define Args_broadcastTransaction (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *signedTransactionBytesStr, const int signedTransactionBytesStrBytes)
#define Args_calculateFullHash (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *unsignedTransactionBytesStr, const int unsignedTransactionBytesStrBytes, const char *signatureHash, const int signatureHashBytes)
#define Args_decryptFrom (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *accountStr, const int accountBytes, const char *dataStr, int dataBytes, const char *nonceStr, const int nonceBytes, const char *decryptedMessageIsTextStr, const int decryptedMessageIsTextBytes, const char *secretPhraseStr, const int secretPhraseBytes)
#define Args_encryptTo (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *recipientStr, const int recipientBytes, const char *messageToEncryptStr, const int messageToEncryptBytes, const char *messageToEncryptIsTextStr, const int messageToEncryptIsTextBytes, const char *secretPhraseStr, const int secretPhraseBytes)
#define Args_getAccount (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *account_RS_or_IDStr, const int account_RS_or_IDBytes)
#define Args_getAccountBlockIds (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *accountStr, const int accountBytes, const char *timestampStr, const int timestampBytes, const char *firstIndexStr, const int firstBytes, const char *lastIndexStr, const int lastBytes)
#define Args_getAccountBlocks (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *accountStr, const int accountBytes, const char *timestampStr, const int timestampBytes, const char *firstIndexStr, const int firstBytes, const char *lastIndexStr, const int lastBytes, const char *includeTransactions, const int includeTransactionsBytes)
#define Args_getAccountId (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *pubKey_64HEX, const int pubKey_64HEXBytes)
#define Args_getAccountTransactionIds (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *account, const int accountBytes, const char *timestampStr, const int timestampBytes, const char *type, const int typeBytes, const char *subtype, const int subtypeBytes, const char *firstIndex, const int firstIndexBytes, const char *lastIndex, const int lastIndexBytes, const char *numberOfConfirmations, const int numberOfConfirmationsBytes)
#define Args_getAccountPublicKey (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *account, const int accountBytes)
#define Args_getAccountTransactions (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *account, const int accountBytes, const char *timestamp, const int timestampBytes, const char *type, const int typeBytes, const char *subtype, const int subtypeBytes, const char *firstIndex, const int firstIndexBytes, const char *lastIndex, const int lastIndexBytes, const char *numberOfConfirmations, const int numberOfConfirmationsBytes)
#define Args_setAccountInfo (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *name, const int nameBytes, const char *description, const int descriptionBytes, const unsigned long long feeNQT, const unsigned long long deadlineMinutes, const bool broadcast)
#define Args_getAlias (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *alias, const int aliasBytes, const char *aliasName, const int aliasNameBytes)
#define Args_setAlias (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *aliasName, const int aliasNameBytes, const char *aliasURI, const int aliasURIBytes, const unsigned long long feeNQT, const unsigned long long deadlineMinutes, const bool broadcast)
#define Args_getAliases (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *timestamp, const int timestampBytes, const char *account, const int accountBytes, const char *firstIndex, const int firstIndexBytes, const char *lastIndex, const int lastIndexBytes)
#define Args_buyAlias (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *alias, const int aliasBytes, const char *aliasName, const int aliasNameBytes, const char *amountNQT, const int amountNQTBytes, const char *recipient, const int recipientBytes, const unsigned long long feeNQT, const unsigned long long deadlineMinutes, const bool broadcast)
#define Args_sellAlias (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *alias, const int aliasBytes, const char *aliasName, const int aliasNameBytes, const char *priceNQT, const int priceNQTBytes, const char *recipient, const int recipientBytes, const unsigned long long feeNQT, const unsigned long long deadlineMinutes, const bool broadcast)
#define Args_getBalance (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *account, const int accountBytes, const char *includeEffectiveBalance, const int includeEffectiveBalanceBytes, const char *height, const int heightBytes, const char *requireBlock, const int requireBlockBytes, const char *requireLastBlock, const int requireLastBlockBytes)
#define Args_getGuaranteedBalance (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *account, const int accountBytes, const char *numberOfConfirmations, const int numberOfConfirmationsBytes)
#define Args_getTransaction (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *transactionID, const int transactionIDBytes, const char *fullHash, const int fullHashBytes)
#define Args_getUnconfirmedTransactionsIds (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *account, const int accountBytes)
#define Args_getUnconfirmedTransactions (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *account, const int accountBytes)
#define Args_parseTransaction (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *transactionBytes, const int transactionBytesBytes, const char *transactionJSON, const int transactionJSONBytes)
#define Args_getTransactionBytes (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *transactionID, const int transactionIDBytes)
#define Args_sendMoney (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *recipient, const int recipientBytes, const unsigned long long amountNQT, const unsigned long long feeNQT, const unsigned long long deadlineMinutes, const char *referencedTransactionFullHash, const int referencedTransactionFullHashBytes, const bool broadcast)
#define Args_sendMoneyMulti (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *recipients, const int recipientsBytes, const char *amountsNQT, const int amountsNQTBytes, const unsigned long long feeNQT, const unsigned long long deadlineMinutes, const bool broadcast)
#define Args_sendMoneyMultiSame (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *recipients, const int recipientsBytes, const unsigned long long amountNQT, const unsigned long long feeNQT, const unsigned long long deadlineMinutes, const bool broadcast)
#define Args_readMessage (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *transactionID, const int transactionIDBytes)
#define Args_sendMessage (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *recipient, const int recipientBytes, const char *message, const int messageBytes, const char *messageIsText, const int messageIsTextBytes, const char *messageToEncrypt, const int messageToEncryptBytes, const char *messageToEncryptIsText, const int messageToEncryptIsTextBytes, const char *encryptedMessageData, const int encryptedMessageDataBytes, const char *encryptedMessageNonce, const int encryptedMessageNonceBytes, const char *messageToEncryptToSelf, const int messageToEncryptToSelfBytes, const char *messageToEncryptToSelfIsText, const int messageToEncryptToSelfIsTextBytes, const char *encryptToSelfMessageData, const int encryptToSelfMessageDataBytes, const char *encryptToSelfMessageNonce, const int encryptToSelfMessageNonceBytes, const char *recipientPublicKey, const int recipientPublicKeyBytes, const char *referencedTransactionFullHash, const int referencedTransactionFullHashBytes, const unsigned long long feeNQT, const unsigned long long deadlineMinutes, const bool broadcast)
#define Args_suggestFee (const burstlibPtr handle, char *returnStr, int &returnBytes)
#define Args_getRewardRecipient (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *account, const int accountBytes)
#define Args_setRewardRecipient (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *recipient, const int recipientBytes, const unsigned long long feeNQT, const unsigned long long deadlineMinutes, const bool broadcast)
#define Args_getBlock (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *block, const int blockBytes, const char *height, const int heightBytes, const char *timestamp, const int timestampBytes, const char *includeTransactions, const int includeTransactionsBytes)
#define Args_getBlockId (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *height, const int heightBytes)
#define Args_getBlockchainStatus (const burstlibPtr handle, char *returnStr, int &returnBytes)
#define Args_getBlocks (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *firstIndex, const int firstIndexBytes, const char *lastIndex, const int lastIndexBytes, const char *includeTransactions, const int includeTransactionsBytes)
#define Args_getConstants (const burstlibPtr handle, char *returnStr, int &returnBytes)
#define Args_getECBlock (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *timestamp, const int timestampBytes)
#define Args_getMiningInfo (const burstlibPtr handle, char *returnStr, int &returnBytes)
#define Args_getMyInfo (const burstlibPtr handle, char *returnStr, int &returnBytes)
#define Args_getPeer (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *peer, const int peerBytes)
#define Args_getPeers (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *active, const int activeBytes, const char *state, const int stateBytes)
#define Args_getTime (const burstlibPtr handle, char *returnStr, int &returnBytes)
#define Args_getState (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *includeCounts, const int includeCountsBytes)
#define Args_longConvert (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *id, const int idBytes)
#define Args_rsConvert (const burstlibPtr handle, char *returnStr, int &returnBytes, const char *account, const int accountBytes)


// IMPORT EXPORT =======================================================================================
#if defined(IMPORT_LIB) || defined(EXPORT_LIB)

PREFIX_PORT burstlibPtr SHOW BurstLib_GetHandle Args_GetHandle;
PREFIX_PORT bool SHOW BurstLib_DeleteHandle Args_DeleteHandle;
PREFIX_PORT int SHOW BurstLib_GetBurstLibVersionNumber Args_GetBurstLibVersionNumber;
PREFIX_PORT bool SHOW BurstLib_GetLastError Args_GetLastError;
PREFIX_PORT bool SHOW BurstLib_SetNode Args_SetNode;
PREFIX_PORT bool SHOW BurstLib_GetNode Args_GetNode;
PREFIX_PORT bool SHOW BurstLib_SetSecretPhrase Args_SetSecretPhrase;
PREFIX_PORT bool SHOW BurstLib_SetAccount Args_SetAccount;
PREFIX_PORT bool SHOW BurstLib_GetAccount Args_GetAccount;
PREFIX_PORT bool SHOW BurstLib_GetJSONvalue Args_GetJSONvalue;

// Ext
PREFIX_PORT bool SHOW BurstLib_CloudDownloadStart Args_CloudDownloadStart;
PREFIX_PORT bool SHOW BurstLib_CloudDownloadFinished Args_CloudDownloadFinished;
PREFIX_PORT bool SHOW BurstLib_CloudCalcCostsStartFinished Args_CloudCalcCostsStart;
PREFIX_PORT bool SHOW BurstLib_CloudCalcCostsFinished Args_CloudCalcCostsFinished;
PREFIX_PORT bool SHOW BurstLib_CloudUploadStart Args_CloudUploadStart;
PREFIX_PORT bool SHOW BurstLib_CloudUploadFinished Args_CloudUploadFinished;
PREFIX_PORT bool SHOW BurstLib_CloudCancel Args_CloudCancel;

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
typedef bool(*BurstLib_GetLastError_Ptr) Args_GetLastError;
typedef bool(*BurstLib_SetNode_Ptr) Args_SetNode;
typedef bool(*BurstLib_GetNode_Ptr) Args_GetNode;
typedef bool(*BurstLib_SetSecretPhrase_Ptr) Args_SetSecretPhrase;
typedef bool(*BurstLib_SetAccount_Ptr) Args_SetAccount;
typedef bool(*BurstLib_GetAccount_Ptr) Args_GetAccount;
typedef bool(*BurstLib_GetJSONvalue_Ptr) Args_GetJSONvalue;

// Ext
typedef bool(*BurstLib_CloudDownloadStart_Ptr) Args_CloudDownloadStart;
typedef bool(*BurstLib_CloudDownloadFinished_Ptr) Args_CloudDownloadFinished;
typedef bool(*BurstLib_CloudCalcCostsStart_Ptr) Args_CloudCalcCostsStart;
typedef bool(*BurstLib_CloudCalcCostsFinished_Ptr) Args_CloudCalcCostsFinished;
typedef bool(*BurstLib_CloudUploadStart_Ptr) Args_CloudUploadStart;
typedef bool(*BurstLib_CloudUploadFinished_Ptr) Args_CloudUploadFinished;
typedef bool(*BurstLib_CloudCancel_Ptr) Args_CloudCancel;

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
	BurstLib_GetLastError_Ptr GetLastError;
	BurstLib_SetNode_Ptr SetNode;
	BurstLib_GetNode_Ptr GetNode;
	BurstLib_SetSecretPhrase_Ptr SetSecretPhrase;
	BurstLib_SetAccount_Ptr SetAccount;
	BurstLib_GetAccount_Ptr GetAccount;
	BurstLib_GetJSONvalue_Ptr GetJSONvalue;

	// Ext
	BurstLib_CloudDownloadStart_Ptr CloudDownloadStart;
	BurstLib_CloudDownloadFinished_Ptr CloudDownloadFinished;
	BurstLib_CloudCalcCostsStart_Ptr CloudCalcCostsStart;
	BurstLib_CloudCalcCostsFinished_Ptr CloudCalcCostsFinished;
	BurstLib_CloudUploadStart_Ptr CloudUploadStart;
	BurstLib_CloudUploadFinished_Ptr CloudUploadFinished;
	BurstLib_CloudCancel_Ptr CloudCancel;

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

#endif // IMPORT EXPORT
#ifdef __cplusplus
}		// extern "C"
#endif	// __cplusplus

#endif // __BURSTKITC__
