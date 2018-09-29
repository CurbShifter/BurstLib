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

#ifdef __cplusplus
extern "C"
{
#endif	// __cplusplus

#if defined(WIN32) || defined(WIN64)
#include <windows.h>
#define GETHANDLE_FUNC GetProcAddress

#if defined(UNICODE) || defined(NOUNICODE)
wchar_t *convertCharArrayToLPCWSTR(const char* charArray)
{
	wchar_t* wString = new wchar_t[4096];
	MultiByteToWideChar(CP_ACP, 0, charArray, -1, wString, 4096);
	return wString;
}
#endif // defined(UNICODE) || defined(NOUNICODE)
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
	if ((errorSO = dlerror()) != 0)
	{ //printf("%s\n", errorSO);
		return 0;
	}
	return ambigious;
}
#endif

#if defined(WIN32) || defined(WIN64)
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
	if (!lib_handle)
	{ //printf("%s\n", dlerror()); // #include <iostream>
		return 0;
	}
#endif
	// returns the address of the DLL functions, otherwise NULL
	burstLib->GetHandle = (BurstLib_GetHandle_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_GetHandle");
	burstLib->DeleteHandle = (BurstLib_DeleteHandle_Ptr)GETHANDLE_FUNC(lib_handle, "BurstLib_DeleteHandle");
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
	if (!dll_handle)
	{ //printf("Failed to load the functions from the BurstLib dynamic library. Library file not found?\n\n");
		return 0;
	}
	return dll_handle;
}

void BurstLib_UnloadDLL(void *dll_handle)
{ // close the dynamicly loaded library
	if (dll_handle)
		dlclose(dll_handle);
}

#endif // platform

#ifdef __cplusplus
}		// extern "C"
#endif	// __cplusplus