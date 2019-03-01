![](https://github.com/CurbShifter/BurstLib/blob/master/CExt.png)

# [CryptoExtensions](https://twitter.com/BurstExtensions) #

# BurstLib
A cross platform dynamic library to make developing applications and tools compatible with the Burstcoin blockchain easier. Providing secure access to the BRS functionality even on external nodes. Because the library signs transactions and encrypts messages locally. It also gives direct access to Burst Extensions such as CloudBurst and BurstCoupon.

It is a bridge between the BRS (Burst Reference Software) RPC API and a standard C interface. Making it ready to be called from a wide range of programming languages such as C, C++, C#, ruby, python, java, scala, php, lua, ect. The exported functions are listed in the BurstLib.h header.

Use it
-
Use the TestClient example [Main.cpp](https://github.com/CurbShifter/BurstLib/blob/master/TestClient/Source/Main.cpp " BurstLib/TestClient/Source/Main.cpp ") as a guideline. It is a simple command line program through which you can up and download cloudburst files and messages. It is written in C++ and uses the c-interface to the library. However the same calls can be made from other languages. For example [Pinvoke](https://msdn.microsoft.com/en-us/library/55d3thsc.aspx?f=255&MSPPError=-2147217396) for  .NET / C#).

For C/C++ The bare minimum would be to include the BurstLib.c and .h files in your project. Then you first create a handle to the binary with _BurstLib_LoadDLL_ (and matching _BurstLib_UnloadDLL_ !). Next you create a handle to the BurstLib itself with *GetHandle* (+ *DeleteHandle*). And then call *SetNode* to set the url of your node.

After that you can call all the lib functions, they are named just like the [The Burst API](https://burstwiki.org/wiki/The_Burst_API) functions and use the same arguments. With an exception for for the secret pass phrase, which you set with *BurstLib_SetSecretPhrase*. Then only when needed your transactions are locally signed and parsed before being (optionally) broadcast. So it doesn't publicly sling your keys over the internet and ensures validity by parsing the transaction.

There are 3 layers to this lib;

1. The BurstLib is the C wrapper to BurstExt.
2. BurstExt includes the extensions CloudBurst and BurstCoupon. Which in itself is inherited from BurstKit
3. BurstKit is the main bridge to the BRS API and includes the crypto + 3rd party libraries.

Building
-
You can build the library from the projects in the [Builds folder](https://github.com/CurbShifter/BurstLib/tree/master/Builds "BurstLib/Builds/"). And separately the TestClient, which demonstrates a commandline version of CloudBurst. The 2 binaries should be placed next to each other. Because the TestClient will check the current working dir for the BurstLib binary. 

- Windows MSVC (2013)
	- Open the BurstLib.sln file and add your boost++ include folder to the 'Additional include directories' in the BurstLib project.
- MacOS XCode
	- Open the BurstLib.xcodeproj build settings and add your boost++ include folder to the 'Header search paths' 
- Linux make
	- Open the LinuxMakeFile folder and execute `make CONFIG=Release` (have libcurl and boost++ installed)


Repeat the same steps to build the [TestClient](https://github.com/CurbShifter/BurstLib/tree/master/TestClient/Builds "BurstLib/TestClient/Builds/") but no extra libraries or headers are needed. 

---

on Ubuntu all you would need is:   
 
    sudo add-apt-repository ppa:the13thfloorelevators/burstcoinppa 
    sudo apt-get update 
    sudo apt-get install burstlib

----

Libraries used
-
All sources and libraries except boost++ (and libcurl for linux) are included in this repository.

	https://juce.com
	https://www.boost.org/
	https://github.com/kokke/tiny-AES-c
	https://github.com/B-Con/crypto-algorithms
	https://github.com/mko-x/SharedEcc25519/tree/master/Sources/Curve25519/curve25519_i64
	https://curl.haxx.se/libcurl/ (for linux)

----


Acknowledgments
-
[Andy Prock](https://github.com/aprock "BurstKit") and ["Daniel Jones"](https://github.com/nixops "nixops") and many thanks to [umbrellacorp03](https://github.com/umbrellacorp03) for the [The Burst API](https://burstwiki.org/wiki/The_Burst_API). And [RokyErickson](https://github.com/RokyErickson) for the Unbuntu packaging.

----
Please support the developers of the software you use. With value for value. 

BURST-WN56-VW53-7B6V-9YAFW

contact: [Discord](https://discord.gg/KsFf3jb "https://discord.gg/KsFf3jb"), 
[twitter](https://twitter.com/BurstExtensions) or curbshifter at protonmail

----------

Released under GPL v3 License - Copyright (c) 2018 CurbShifter
