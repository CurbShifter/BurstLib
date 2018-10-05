mode con cols=180 lines=80 >nul
cls

@echo off

set root=%~dp0..\..
echo %root%

:: -----------------------------------------------------------------------------------------------
echo Build binaries

call "%ProgramFiles(x86)%\Microsoft Visual Studio 12.0\VC\bin\vcvars32.bat"

msbuild.exe "BurstLib.sln" /t:rebuild /maxcpucount:8 /flp1:logfile=errors32.txt;errorsonly /t:"BurstLib" /p:Platform=Win32 /p:Configuration="Release" 
msbuild.exe "BurstLib.sln" /t:rebuild /maxcpucount:8 /flp1:logfile=errors32.txt;errorsonly /t:"BurstLib" /p:Platform=x64 /p:Configuration="Release"

msbuild.exe "BurstLib.sln" /t:rebuild /maxcpucount:8 /flp1:logfile=errors32.txt;errorsonly /t:"TestClient" /p:Platform=Win32 /p:Configuration="Release" 
msbuild.exe "BurstLib.sln" /t:rebuild /maxcpucount:8 /flp1:logfile=errors32.txt;errorsonly /t:"TestClient" /p:Platform=x64 /p:Configuration="Release"

:: -----------------------------------------------------------------------------------------------
echo Create package

DEL /q "%root%\pkg\BurstLib\"
del "%root%\pkg\BurstLib-Win.zip"

xcopy /y "%root%\Source\BurstLib.h" "%root%\pkg\BurstLib\C\BurstLib.h*"
xcopy /y "%root%\Source\BurstLib.c" "%root%\pkg\BurstLib\C\BurstLib.c*"
xcopy /y "%root%\Builds\VisualStudio2013\Release\BurstLib.dll" "%root%\pkg\BurstLib\bin\Win32\BurstLib.dll*"
xcopy /y "%root%\Builds\VisualStudio2013\x64\Release\BurstLib.dll" "%root%\pkg\BurstLib\bin\Win64\BurstLib.dll*"
xcopy /y "%root%\Builds\VisualStudio2013\Release\TestClient.exe" "%root%\pkg\BurstLib\bin\Win32\TestClient.exe*"
xcopy /y "%root%\Builds\VisualStudio2013\x64\Release\TestClient.exe" "%root%\pkg\BurstLib\bin\Win64\TestClient.exe*"

:: install 7zip to use this last step
if exist "C:\Program Files\7-Zip\7z.exe" "C:\Program Files\7-Zip\7z.exe" a -r "%root%\pkg\BurstLib.zip" "%root%\pkg\BurstLib"

pause
