@rem  Run this from within the top-level dir: deploy\clean-build-and-package
@echo on

@set /p VERSION=<version.h
@set VERSION=%VERSION:#define RUNNER_VERSION "=%
set VERSION=%VERSION:"=%

@echo(
@set YN=y
@set /p YN="Proceed to clean, rebuild, package, and sign version %VERSION% [Yn] ?"

@if "%YN%" == "Y" set YN=y
@if "%YN%" neq "y" exit /b 3

@echo Proceeding

call .\deploy\win\build-both.bat
if %errorlevel% neq 0 exit /b %errorlevel%

@echo on

set NAME=Open Source Developer, Christopher Cannam

@echo Signing 32-bit executables and libraries
signtool sign /v /n "%NAME%" /t http://time.certum.pl /fd sha1 /a build_win32\release\*.exe build_win32\release\*.dll

@echo Signing 64-bit executables and libraries
signtool sign /v /n "%NAME%" /t http://time.certum.pl /fd sha1 /a build_win64\release\*.exe build_win64\release\*.dll

@echo Zipping up 32-bit package
set pkg=sonic-annotator-%VERSION%-win32
set dir=%pkg%\%pkg%
mkdir %pkg%
mkdir %dir%
copy CHANGELOG %dir%\CHANGELOG.txt
copy CITATION %dir%\CITATION.txt
copy COPYING %dir%\COPYING.txt
copy README.md %dir%\README.txt
copy build_win32\release\Qt5Core.dll %dir%
copy build_win32\release\Qt5Network.dll %dir%
copy build_win32\release\Qt5Xml.dll %dir%
copy build_win32\release\libgcc_s_dw2-1.dll %dir%
copy build_win32\release\libstdc++-6.dll %dir%
copy build_win32\release\libwinpthread-1.dll %dir%
copy build_win32\release\sonic-annotator.exe %dir%

powershell -NoProfile -ExecutionPolicy Bypass -Command "& 'deploy\win\zip.ps1' %pkg%.zip %pkg%"
del /q /s %pkg%

@echo Zipping up 64-bit package
set pkg=sonic-annotator-%VERSION%-win64
set dir=%pkg%\%pkg%
mkdir %pkg%
mkdir %dir%
copy CHANGELOG %dir%\CHANGELOG.txt
copy CITATION %dir%\CITATION.txt
copy COPYING %dir%\COPYING.txt
copy README.md %dir%\README.txt
copy build_win64\release\Qt5Core.dll %dir%
copy build_win64\release\Qt5Network.dll %dir%
copy build_win64\release\Qt5Xml.dll %dir%
copy build_win64\release\libsndfile-1.dll %dir%
copy build_win64\release\sonic-annotator.exe %dir%

copy "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Redist\MSVC\14.25.28508\x64\Microsoft.VC142.CRT\concrt140.DLL" %dir%
copy "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Redist\MSVC\14.25.28508\x64\Microsoft.VC142.CRT\msvcp140.DLL" %dir%
copy "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Redist\MSVC\14.25.28508\x64\Microsoft.VC142.CRT\vccorlib140.DLL" %dir%
copy "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Redist\MSVC\14.25.28508\x64\Microsoft.VC142.CRT\vcruntime140.DLL" %dir%
copy "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Redist\MSVC\14.25.28508\x64\Microsoft.VC142.CRT\vcruntime140_1.DLL" %dir%

powershell -NoProfile -ExecutionPolicy Bypass -Command "& 'deploy\win\zip.ps1' %pkg%.zip %pkg%"
del /q /s %pkg%

@echo Done
