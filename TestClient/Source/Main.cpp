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

---------------------------------------------------------------------
Examples showing the usage of BurstLib
*/

#define PASSPHRASE ""
#define STR_SIZE 2048

#include "BurstLib.h"
#include "BurstLib.c"



#include <ctime>
#include <string>
#include <iostream>
#include <vector>

#if defined(__linux__)
#include <cstring>
#endif

#if defined(_WIN32) || defined(_WIN64)
#include <tchar.h>
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

void SleepMS(int sleepMs)
{
#if defined(_WIN32) || defined(_WIN64)
	Sleep(sleepMs);
#else
	usleep(sleepMs * 1000);   // usleep takes sleep time in us (1 millionth of a second)
#endif
}

// ---------------------------------------------------------------------------------------------------------
bool LoadLib(void **dll_handle, burstlibPtr &apiHandle, BurstLib_FunctionHandles &burstLib)
{ // create a full path the to library file to load it at run time.
	char library_path[FILENAME_MAX];
	GetCurrentDir(library_path, sizeof(library_path)); // get execution dir
#if defined(_WIN64) || defined(_WIN32)
	strcat_s(library_path, FILENAME_MAX, "\\BurstLib.dll");
#elif defined(__APPLE__)
	strcat(library_path, "/BurstLib.dylib");
#elif defined(__linux__)
	strcat(library_path, "/BurstLib.so");
#endif
	std::cout << "Loading BurstLib library " << library_path << std::endl;
	*dll_handle = BurstLib_LoadDLL(library_path, &burstLib);// Call the library functions
	if (!dll_handle || burstLib.GetHandle == nullptr || burstLib.GetBurstLibVersionNumber == nullptr)
	{
		std::cout << std::endl << "BurstLib library not found!" << std::endl;
		return false;
	}
	apiHandle = burstLib.GetHandle();
	std::cout << ("version ") << burstLib.GetBurstLibVersionNumber(apiHandle) << std::endl << std::endl;
	return (apiHandle != 0);
}

// ---------------------------------------------------------------------------------------------------------
void suggestFee(const burstlibPtr &apiHandle, BurstLib_FunctionHandles &burstLib, const char *walletUrl)
{
	burstLib.SetNode(apiHandle, walletUrl, strlen(walletUrl));

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

void testDecryptMessage(const burstlibPtr &apiHandle, BurstLib_FunctionHandles &burstLib, const char *walletUrl)
{
	burstLib.SetNode(apiHandle, walletUrl, strlen(walletUrl));

	char account[] = "16922903237994405232";
	char recipientSecretPhrase[] = "IWontTellYou";
	char encrypted[] = "d5a1958d12ce96ce30dbce5b6c8ead7ecbc0f59d857dc8e8fbeec10ae440e0e74e9120fef3b0fa586d4c63fde0f289340e709b30ae528e3c2d740b11e3ae3fdb5e5d5c63f724cf16157c75dabec31eaf";
	char nonce[] = "7cefa6f66d5b71604e2ef56a18319b3f48a38e8aa5cf610369b294f1d40e0f8e";
	char messageIsText[] = "true";

	char decrypted[STR_SIZE];
	int decryptedSize = STR_SIZE;
	memset(&decrypted[0], 0, STR_SIZE);

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

void makeCoupon(const burstlibPtr &apiHandle, BurstLib_FunctionHandles &burstLib, const char *walletUrl,
	const char *recipient,
	const char *couponPassword,
	const char *, //amountStr,
	const char *deadlineStr,
	const char *feeStr,
	const char *walletPassPhrase)
{
	//long long amount = atoi(amountStr);
	long long deadline = atoi(deadlineStr);
	long long fee = atoi(feeStr);

	burstLib.SetNode(apiHandle, walletUrl, strlen(walletUrl));
	burstLib.SetSecretPhrase(apiHandle, &walletPassPhrase[0], strlen(walletPassPhrase));

	char signedTX[STR_SIZE];
	int signedTXSize = STR_SIZE;
	memset(&signedTX[0], 0, STR_SIZE);

	if (burstLib.sendMoney(apiHandle,
		&signedTX[0], signedTXSize,
		&recipient[0], strlen(recipient),
		1,
		fee,
		deadline,
		0, 0,
		false))
	{
		char couponHEX[STR_SIZE];
		int couponHEXSize = STR_SIZE;
		memset(&couponHEX[0], 0, STR_SIZE);

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

void shabaltest(const burstlibPtr &apiHandle, BurstLib_FunctionHandles &burstLib)
{
	std::clock_t begin = std::clock();

	char inbuf[64];
	memset(&inbuf[0], 0, sizeof(char) * 64);
	sprintf(&inbuf[0], "12345678901234567890123456789012345678901234567890");
	int offset = 0;
	int len = 64;
	char dst[32];
	memset(&dst[0], 0, sizeof(char) * 32);

	//int t = 1024;// *128;
	//for (int i = 0; i < t; i++)
	{
		unsigned int cc = burstLib.Shabal256_ccID(apiHandle);
	//	burstLib.Shabal256_reset(apiHandle, cc);
		burstLib.Shabal256_update(apiHandle, cc, &inbuf[0], offset, len);
		burstLib.Shabal256_digest(apiHandle, cc, &dst[0]);
	}
	// https://hashes.org/gen.php -> SHABAL256
	// 1234 -> b1335c97df0159e5597378655791b7cf1f46f60835d5f35343eaa1ed63db3493  ±3\—ßYåYsxeW‘·ÏFö
	// $HEX[0000000000000000] -> 512fd2962b2b798bf331a074a93cb55d3fb4387119bfcee60fe291e812dbb7fc
	// $HEX[0123456789ABCDEF] -> 8473373a841654334022fe047d9f16d640162711d4c810b0989d6bc158600613
	//
	// "512fd2962b2b798bf331a074a93cb55d3fb4387119bfcee60fe291e812dbb7fc" -> d3deb90eecafce16ef8b45bf5c4af778728c5816b47aab0fbc9b0ad6bff6c381
	// $HEX[512fd2962b2b798bf331a074a93cb55d3fb4387119bfcee60fe291e812dbb7fc] -> 3f924da0ea209f428dd25dd2418f29324bf2b8be421dfb0a7a6954d0bea0db79
	//
	// $HEX[0000000000000000000000000000000000000000000000000000000000000000] -> 1a032cb661c199b6afa594ea6ba0d646c789b263c87af247e211ef14f0f21e0b
	// 00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 -> 2271f82522a7c60e05b2b61d454eb34b646fcd4756799c49fca1c27f3dd27e1b
	clock_t end = clock();
	double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;

	std::cout << "in :" << &inbuf[0] << std::endl;
//	std::cout << std::hex << 12;
	std::cout << "out:" << &dst[0] << std::endl;
//	std::cout << std::dec << 1;
	std::cout << "time sec: " << elapsed_secs << std::endl;
}

void cloudDownload(const burstlibPtr &apiHandle, BurstLib_FunctionHandles &burstLib,
	const char *walletUrl,
	const char *cloudID,
	const char *dlFolder)
{
	burstLib.SetNode(apiHandle, walletUrl, strlen(walletUrl));

	char *dlData = new char[1024 * 1024]; // 1mb
	int dlDataSize = 1024 * 1024;
	char *dlFilename = new char[1024]; // 1mb
	int dlFilenameSize = 1024;
	char msg[STR_SIZE];
	int msgSize = STR_SIZE;
	memset(&msg[0], 0, STR_SIZE);
	unsigned long long epoch = 0;

	if (burstLib.CloudDownloadStart(apiHandle,
		cloudID, strlen(cloudID),
		dlFolder, strlen(dlFolder)))
	{
		float progress = 0.f;
		while (burstLib.CloudDownloadFinished(apiHandle,
			&msg[0], msgSize,
			cloudID, strlen(cloudID),
			&dlFilename[0], dlFilenameSize,
			&dlData[0], dlDataSize,
			epoch,
			progress) == false)
		{
			std::cout << (int)(progress * 100.f) << "%" << std::endl;
			SleepMS(1000);
		}
		std::cout << std::string(&cloudID[0], strlen(cloudID)) << std::endl;
		std::cout << "Message: " << std::string(&msg[0], msgSize) << std::endl;
		if (strlen(dlFilename) > 0)
			std::cout << "Downloaded to: " << std::string(&dlFolder[0], strlen(dlFolder)) << "\\" << std::string(dlFilename, strlen(dlFilename)) << std::endl;
	}
	else std::cout << "failed to download" << std::endl;
	if (dlData)
		delete dlData;
}

void cloudUpload(const burstlibPtr &apiHandle, BurstLib_FunctionHandles &burstLib,
	const char *walletUrl,
	const char *msg,
	const char *fileToUpload,
	const char *stackSizeStr,
	const char *feeStr,
	const char *walletPassPhrase)
{
	long long stackSize = atoi(stackSizeStr);
	long long fee = atoi(feeStr);

	burstLib.SetNode(apiHandle, walletUrl, strlen(walletUrl));
	burstLib.SetSecretPhrase(apiHandle, walletPassPhrase, strlen(walletPassPhrase));

	char jobId[STR_SIZE];
	int jobIdSize = STR_SIZE;
	memset(&jobId[0], 0, STR_SIZE);

	if (burstLib.CloudUploadStart(apiHandle,
		&jobId[0], jobIdSize,
		&msg[0], strlen(msg),
		&fileToUpload[0], strlen(fileToUpload),
		stackSize,
		fee))
	{
		char uploadResult[STR_SIZE];
		int uploadResultSize = STR_SIZE;
		memset(&uploadResult[0], 0, STR_SIZE);

		unsigned long long txFeePlancks = 0;
		unsigned long long feePlancks = 0;
		unsigned long long burnPlancks = 0;
		unsigned long long costsNQT = 0;
		unsigned long long confirmTime = 0;

		float progress = 0.f;
		while (burstLib.CloudUploadFinished(apiHandle,
			&uploadResult[0], uploadResultSize,
			&jobId[0], jobIdSize,
			txFeePlancks,
			feePlancks,
			burnPlancks,
			costsNQT,
			confirmTime,
			progress) == false)
		{
			std::cout << (int)(progress * 100.f) << "%" << std::endl;
			SleepMS(1000);
		}

		std::cout << "costs NQT: " << costsNQT << std::endl;
		std::cout << "confirm seconds: " << confirmTime << std::endl;
		std::cout << "upload result: " << std::string(&uploadResult[0], uploadResultSize) << std::endl;
	}
	else std::cout << "failed to upload" << std::endl << std::endl;
}

void cloudCalcCosts(const burstlibPtr &apiHandle, BurstLib_FunctionHandles &burstLib,
	const char *walletUrl,
	const char *msg,
	const char *fileToCalcCosts,
	const char *stackSizeStr,
	const char *feeStr,
	const char *walletPassPhrase)
{
	long long stackSize = atoi(stackSizeStr);
	long long fee = atoi(feeStr);

	burstLib.SetNode(apiHandle, walletUrl, strlen(walletUrl));
	burstLib.SetSecretPhrase(apiHandle, walletPassPhrase, strlen(walletPassPhrase));

	char jobId[STR_SIZE];
	int jobIdSize = STR_SIZE;
	memset(&jobId[0], 0, STR_SIZE);

	if (burstLib.CloudCalcCostsStart(apiHandle,
		&jobId[0], jobIdSize,
		&msg[0], strlen(msg),
		&fileToCalcCosts[0], strlen(fileToCalcCosts),
		stackSize,
		fee))
	{
		char uploadResult[STR_SIZE];
		int uploadResultSize = STR_SIZE;
		memset(&uploadResult[0], 0, STR_SIZE);

		unsigned long long txFeePlancks = 0;
		unsigned long long feePlancks = 0;
		unsigned long long burnPlancks = 0;
		unsigned long long costsNQT = 0;
		unsigned long long confirmTime = 0;

		float progress = 0.f;
		while (burstLib.CloudCalcCostsFinished(apiHandle,
			&uploadResult[0], uploadResultSize,
			&jobId[0], jobIdSize,
			txFeePlancks,
			feePlancks,
			burnPlancks,
			costsNQT,
			confirmTime,
			progress) == false)
		{
			std::cout << (int)(progress * 100.f) << "%" << std::endl;
			SleepMS(1000);
		}
		std::cout << "costs NQT: " << costsNQT << std::endl;
		std::cout << "confirm seconds: " << confirmTime << std::endl;
		std::cout << "Calc costs result: " << std::string(&uploadResult[0], uploadResultSize) << std::endl << std::endl;
	}
	else std::cout << "failed to upload" << std::endl << std::endl;
}

// ---------------------------------------------------------------------------------------------------------
#if defined(_WIN32) || defined(_WIN64)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{
	std::vector<std::string> allArgs(argv, argv + argc);

	void *dll_handle = 0;
	burstlibPtr apiHandle = 0;
	BurstLib_FunctionHandles burstLib;
	if (LoadLib(&dll_handle, apiHandle, burstLib) && allArgs.size() > 1)
	{
		if (allArgs[1].compare("shabaltest") == 0)
			shabaltest(apiHandle, burstLib);

		else if (allArgs[1].compare("download") == 0 && allArgs.size() > 4)
			cloudDownload(apiHandle, burstLib, allArgs[2].c_str(), allArgs[3].c_str(), allArgs[4].c_str());

		else if (allArgs[1].compare("upload") == 0 && allArgs.size() > 7)
			cloudUpload(apiHandle, burstLib, allArgs[2].c_str(), allArgs[3].c_str(), allArgs[4].c_str(), allArgs[5].c_str(), allArgs[6].c_str(), allArgs[7].c_str());

		else if (allArgs[1].compare("calc") == 0 && allArgs.size() > 7)
			cloudCalcCosts(apiHandle, burstLib, allArgs[2].c_str(), allArgs[3].c_str(), allArgs[4].c_str(), allArgs[5].c_str(), allArgs[6].c_str(), allArgs[7].c_str());

		else if (allArgs[1].compare("suggestFee") == 0 && allArgs.size() > 2)
			suggestFee(apiHandle, burstLib, allArgs[2].c_str());

		else if (allArgs[1].compare("makeCoupon") == 0 && allArgs.size() > 8)
			makeCoupon(apiHandle, burstLib, allArgs[2].c_str(), allArgs[3].c_str(), allArgs[4].c_str(), allArgs[5].c_str(), allArgs[6].c_str(), allArgs[7].c_str(), allArgs[8].c_str());
		else
		{
			std::cout << "Downloading:" << std::endl;
			std::cout << "BurstClient download [wallet_url] [cloud_id] [download_folder_path optional]" << std::endl;
			std::cout << "example:\ndownload \"https://wallet.dev.burst-test.net/\" CLOUD-B77X-C2R9-UD7C-HPASK \"\"" << std::endl << std::endl;

			std::cout << "Uploading:" << std::endl;
			std::cout << "BurstClient upload [wallet_url] [message] [filepath_to_upload optional] [stackSize] [feeBase] [wallet_PassPhrase]" << std::endl;
			std::cout << "example:\nupload \"https://wallet.dev.burst-test.net/\" \"test message\" \"\" 10 735000 \"PASSPHRASE\"" << std::endl << std::endl;

			std::cout << "Calc upload cost:" << std::endl;
			std::cout << "BurstClient calc [wallet_url] [message] [filepath_to_upload optional] [stackSize] [feeBase] [wallet_PassPhrase optional]" << std::endl;
			std::cout << "example:\ncalc \"https://wallet.dev.burst-test.net/\" \"test message\" \"\" 10 735000 \"PASSPHRASE\"" << std::endl << std::endl;

			/* hosts
			 https://wallet1.burst-team.us:2083/
			 https://127.0.0.1:8125/
			
			download "https://wallet1.burst-team.us:2083/" CLOUD-2J7G-GATT-V3D7-BJE62 ""
			download "https://wallet.dev.burst-test.net/" CLOUD-B77X-C2R9-UD7C-HPASK ""
			calc "https://wallet.dev.burst-test.net/" "test message" "" 10 735000 ""
			upload "https://wallet.dev.burst-test.net/" "test message" "" 10 735000 "PASSPHRASE"
			*/
		}
		//testDecryptMessage(apiHandle, burstLib);
	}

	std::cout << "Cleaning up memory" << std::endl;
	if (apiHandle && burstLib.DeleteHandle)
		burstLib.DeleteHandle(apiHandle);
	BurstLib_UnloadDLL(dll_handle);

	std::cout << "Press enter key to quit" << std::endl;
	std::cin.get();

	return 0;
}


