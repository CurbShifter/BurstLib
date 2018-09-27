//int main (int argc, char* argv[])
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

--------------------------------------------
Simple example showing the usage of BurstLib
*/

#define PASSPHRASE ""

#include "BurstLib.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <math.h>
#include <time.h>
#include <vector>

#ifndef UNICODE  
typedef std::string String;
#else
typedef std::wstring String;
#endif

#if defined(_WIN32) || defined(_WIN64)
#include <stdio.h>
#include <tchar.h>
#include <direct.h>
#define GetCurrentDir _getcwd
#endif

void SetNode(const burstlibPtr &apiHandle, BurstLib_FunctionHandles &burstLib)
{
	const char *walletUrl = "https://wallet.dev.burst-test.net/"; // TESTNET
	//const char *walletUrl = "https://wallet1.burst-team.us:2083/";
	//const char *walletUrl = "https://wallet.burst.cryptoguru.org:8125/";
	if (burstLib.SetNode)
		burstLib.SetNode(apiHandle, walletUrl, strlen(walletUrl));
}

void suggestFee(const burstlibPtr &apiHandle, BurstLib_FunctionHandles &burstLib)
{
	std::cout << ("suggestFee") << std::endl;
	char suggestFee[1024];
	int suggestFeeSize = 1024;
	memset(&suggestFee[0], 0, suggestFeeSize);
	burstLib.suggestFee(apiHandle, suggestFee, suggestFeeSize);

	char suggestFeeCheap[1024];
	int suggestFeeCheapSize = 1024;
	memset(&suggestFeeCheap[0], 0, suggestFeeCheapSize);
	{
		char *key = "cheap";
		burstLib.GetJSONvalue(apiHandle, &suggestFeeCheap[0], suggestFeeCheapSize, &suggestFee[0], suggestFeeSize, key, strlen(key));
		std::cout << ("cheap: ") << std::string(&suggestFeeCheap[0], suggestFeeCheapSize) << std::endl;
	}
	char suggestFeeStandard[1024];
	int suggestFeeStandardSize = 1024;
	memset(&suggestFeeStandard[0], 0, suggestFeeStandardSize);
	{
		char *key = "standard";
		burstLib.GetJSONvalue(apiHandle, &suggestFeeStandard[0], suggestFeeStandardSize, &suggestFee[0], suggestFeeSize, key, strlen(key));
		std::cout << ("standard: ") << std::string(&suggestFeeStandard[0], suggestFeeStandardSize) << std::endl;
	}
	char suggestFeePriority[1024];
	int suggestFeePrioritySize = 1024;
	memset(&suggestFeePriority[0], 0, suggestFeePrioritySize);
	{
		char *key = "priority";
		burstLib.GetJSONvalue(apiHandle, &suggestFeePriority[0], suggestFeePrioritySize, &suggestFee[0], suggestFeeSize, key, strlen(key));
		std::cout << ("priority: ") << std::string(&suggestFeePriority[0], suggestFeePrioritySize) << std::endl;
	}
}

void testDecryptMessage(const burstlibPtr &apiHandle, BurstLib_FunctionHandles &burstLib)
{
	const char *walletUrl = "https://wallet1.burst-team.us:2083/";
	if (burstLib.SetNode)
		burstLib.SetNode(apiHandle, walletUrl, strlen(walletUrl));

	char account[] = "16922903237994405232";
	char recipientSecretPhrase[] = "IWontTellYou";
	char encrypted[] = "d5a1958d12ce96ce30dbce5b6c8ead7ecbc0f59d857dc8e8fbeec10ae440e0e74e9120fef3b0fa586d4c63fde0f289340e709b30ae528e3c2d740b11e3ae3fdb5e5d5c63f724cf16157c75dabec31eaf";
	char nonce[] = "7cefa6f66d5b71604e2ef56a18319b3f48a38e8aa5cf610369b294f1d40e0f8e";
	char messageIsText[] = "true";

	char decrypted[1024 * 2];
	int decryptedSize = 1024 * 2;
	memset(&decrypted[0], 0, 1024 * 2);

	if (burstLib.decryptFrom(
		apiHandle,
		&decrypted[0], decryptedSize,
		&account[0], strlen(&account[0]),
		&encrypted[0], strlen(&encrypted[0]),
		&nonce[0], strlen(&nonce[0]),
		&messageIsText[0], strlen(&messageIsText[0]),
		&recipientSecretPhrase[0], strlen(&recipientSecretPhrase[0])))
		std::cout << ("test decrypt : ") << decrypted << std::endl << std::endl; // "This is a message encrypted using \"encryptTo\"."
}

void makeCoupon(const burstlibPtr &apiHandle, BurstLib_FunctionHandles &burstLib)
{
	char recipient[] = "BURST-MXG8-YZAD-J9S8-C29YM";
	char couponPassword[] = "test";

	char walletPassPhrase[] = PASSPHRASE;
	burstLib.SetSecretPhrase(apiHandle, &walletPassPhrase[0], strlen(walletPassPhrase));

	char signedTX[1024 * 2];
	int signedTXSize = 1024 * 2;
	memset(&signedTX[0], 0, 1024 * 2);

	if (burstLib.sendMoney(apiHandle,
		&signedTX[0], signedTXSize,
		&recipient[0], strlen(recipient),
		1,
		735000,
		1440,
		false))
	{
		char couponHEX[1024 * 2];
		int couponHEXSize = 1024 * 2;
		memset(&couponHEX[0], 0, 1024 * 2);

		if (burstLib.CreateCoupon(apiHandle,
			&couponHEX[0], couponHEXSize,
			&signedTX[0], signedTXSize,
			&couponPassword[0], strlen(&couponPassword[0])))
		{
			std::cout << ("Coupon code:\n") << std::string(&couponHEX[0], couponHEXSize) << std::endl << std::endl;
		}
		else std::cout << "failed to create valid coupon" << std::endl;
	}
	else std::cout << "failed to create transaction" << std::endl;
}

void cloudDownload(const burstlibPtr &apiHandle, BurstLib_FunctionHandles &burstLib)
{
	char cloudID[] = "CLOUD-JF9S-RC66-VUKV-6WYP8";// "CLOUD-KRFE-ZXS9-N28U-7DXUL";
	char dlFolder[FILENAME_MAX];
	GetCurrentDir(dlFolder, sizeof(dlFolder)); // get execution dir

	char *dlData = new char[1024 * 1024]; // 1mb
	int dlDataSize = 1024 * 1024;
	char *dlFilename = new char[1024]; // 1mb
	int dlFilenameSize = 1024;
	char msg[1024 * 2];
	int msgSize = 1024 * 2;
	memset(&msg[0], 0, 1024 * 2);

	if (burstLib.CloudDownload(apiHandle,
		&msg[0], msgSize,
		&cloudID[0], strlen(cloudID),
		dlFolder, strlen(dlFolder),
		dlFilename, dlFilenameSize,
		dlData, dlDataSize))
	{
		std::cout << std::string(&cloudID[0], strlen(cloudID)) << std::endl;
		std::cout << "Message: " << std::string(&msg[0], msgSize) << std::endl;
		if (strlen(dlFilename) > 0)
			std::cout << "Downloaded to: " << std::string(&dlFolder[0], strlen(dlFolder)) << "\\" << std::string(dlFilename, strlen(dlFilename)) << std::endl;
	}
	else std::cout << "failed to download" << std::endl;
	if (dlData)
		delete dlData;
}

void cloudCalcCosts(const burstlibPtr &apiHandle, BurstLib_FunctionHandles &burstLib)
{
	char msg[] = "testing 123";
	char fileToUpload[] = "C:\\Users\\Jorn\\Desktop\\TEST\\test.txt";
	long long stackSize = 10;
	long long fee = 735000;

	long long addressesNum = 0;
	long long txFeePlancks = 0;
	long long feePlancks = 0;
	long long burnPlancks = 0;

	long long costsNQT = 0;

	char returnStr[1024 * 2];
	int returnSize = 1024 * 2;
	memset(&returnStr[0], 0, 1024 * 2);

	if (burstLib.CloudCalcCosts(apiHandle,
		&returnStr[0], returnSize,
		&msg[0], strlen(msg),
		&fileToUpload[0], strlen(fileToUpload),
		stackSize,
		fee,
		addressesNum,
		txFeePlancks,
		feePlancks,
		burnPlancks,
		costsNQT))
	{
		std::cout << "costs to upload :" << std::string(&fileToUpload[0], strlen(fileToUpload)) << std::endl << costsNQT << " NQT" << std::endl << std::endl;
	}
	else std::cout << "failed to calc costs" << std::endl << std::endl;
}

void cloudUpload(const burstlibPtr &apiHandle, BurstLib_FunctionHandles &burstLib)
{
	char walletPassPhrase[] = PASSPHRASE;
	burstLib.SetSecretPhrase(apiHandle, &walletPassPhrase[0], strlen(walletPassPhrase));

	char msg[] = "testing 123";
	char fileToUpload[] = "";
	long long stackSize = 10;
	long long fee = 735000 * 2;

	long long addressesNum = 0;
	long long txFeePlancks = 0;
	long long feePlancks = 0;
	long long burnPlancks = 0;

	long long costsNQT = 0;

	char result[1024 * 2];
	int resultSize = 1024 * 2;
	memset(&result[0], 0, 1024 * 2);

	if (burstLib.CloudUpload(apiHandle,
		&result[0], resultSize,
		&msg[0], strlen(msg),
		&fileToUpload[0], strlen(fileToUpload),
		stackSize,
		fee,
		addressesNum,
		txFeePlancks,
		feePlancks,
		burnPlancks,
		costsNQT))
	{
		std::cout << "upload result: " << std::string(&result[0], resultSize) << std::endl << std::endl;
	}
	else std::cout << "failed to upload" << std::endl << std::endl;
}


#if defined(_WIN32) || defined(_WIN64)
int _tmain(int argc, _TCHAR* argv[])
#else
#include <unistd.h>
#define GetCurrentDir getcwd

int main(int argc, char* argv[])
#endif
{ // create a full path the to library file to load it at run time.
	char library_path[FILENAME_MAX];
	GetCurrentDir(library_path, sizeof(library_path)); // get execution dir
#if defined(_WIN64) || defined(_WIN32)
	strcat_s(library_path, FILENAME_MAX, "\\BurstLib.dll");
#elif defined(__linux__)
	strcat(library_path, "/BurstLib.so");
#endif
	burstlibPtr apiHandle = 0;
	BurstLib_FunctionHandles burstLib;
	std::cout << "Loading BurstLib library " << library_path << std::endl;
	void *dll_handle = BurstLib_LoadDLL(library_path, &burstLib);// Call the library functions
	if (!dll_handle || burstLib.GetHandle == nullptr)
	{
		std::cout << std::endl << "BurstLib library not found!" << std::endl;
		goto error_end;
	}
	apiHandle = burstLib.GetHandle();
	if (!apiHandle)
		goto error_end;

	std::cout << ("version ") << burstLib.GetBurstLibVersionNumber(apiHandle) << std::endl << std::endl;

	SetNode(apiHandle, burstLib);	
	//suggestFee(apiHandle, burstLib);
	//makeCoupon(apiHandle, burstLib);
	//cloudDownload(apiHandle, burstLib);
	//cloudCalcCosts(apiHandle, burstLib);
	//cloudUpload(apiHandle, burstLib);

	testDecryptMessage(apiHandle, burstLib);	

error_end:
	std::cout << ("\n\nCleaning up memory\n");
	if (apiHandle && burstLib.DeleteHandle)
		burstLib.DeleteHandle(apiHandle);

	BurstLib_UnloadDLL(dll_handle);

	std::cout << "Press enter key to quit" << std::endl;
	std::cin.get();

	return 0;
}
