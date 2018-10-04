![](https://github.com/CurbShifter/BurstLib/blob/master/BurstExtensionsLogo.png)

# BurstLib
A cross platform dynamic library to make developing applications and tools compatible with the Burst blockchain easier. Providing secure access to the BRS functionality even on external nodes. Because the library signs transactions and encrypts messages locally. And gives direct access to Burst Extensions such as CloudBurst and BurstCoupon.

It is a bridge between the BRS (Burst Reference Software) REST API and a standard C interface. Making it ready to be called from a wide range of programming languages such as C, C++, C#, ruby, python, java, scala, php, lua, ect. The exported functions are listed in the BurstLib.h header.


Building
-
Build the library from the projects in the 'Builds' folder. And the TestClient which demonstrates a commandline version of CloudBurst. The TestClient loads BurstLib during run time. The binaries should be placed next to each other.


- Windows MSVC2013 - open project file and add your boost++ include folder to the 'Additional include directories' 
- MacOS XCode project file. - open build settings and add your boost++ include folder to the 'Header search paths' 
- Linux - have libcurl and boost++ installed. Open the LinuxMakeFile folder and execute `make CONFIG=Release`

All sources and libraries except boost++ (and libcurl for linux) are included in this repository. (download it at boost.org and add the include folder to your build).

Libraries used
-

	https://juce.com
	https://www.boost.org/
	https://github.com/kokke/tiny-AES-c
	https://github.com/B-Con/crypto-algorithms
	https://github.com/mko-x/SharedEcc25519/tree/master/Sources/Curve25519/curve25519_i64
	https://curl.haxx.se/libcurl/ (for linux)

----


Acknowledgments
-
[Andy Prock](https://github.com/aprock "BurstKit") and ["Daniel Jones"](https://github.com/nixops "nixops") 


Value for value
-
Please support the developers of the software you use 

BURST-WN56-VW53-7B6V-9YAFW

contact: [Discord](https://discord.gg/KsFf3jb "https://discord.gg/KsFf3jb"), 
[twitter](https://twitter.com/BurstExtensions) or curbshifter at protonmail

----------

Released under GPL v3 License - Copyright (c) 2018 CurbShifter
