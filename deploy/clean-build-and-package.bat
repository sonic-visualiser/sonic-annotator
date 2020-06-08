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

call .\deploy\win64\build-both.bat
if %errorlevel% neq 0 exit /b %errorlevel%

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
copy build_win32\release\Qt5Core.dll %dir%\
copy build_win32\release\Qt5Network.dll %dir%\
copy build_win32\release\Qt5Xml.dll %dir%\
copy build_win32\release\libgcc_s_dw2-1.dll %dir%\
copy build_win32\release\libstdc++-6.dll %dir%\
copy build_win32\release\libwinpthread-1.dll %dir%\
copy build_win32\release\sonic-annotator.exe %dir%\
deploy\win\zip %pkg%.zip %pkg%

@echo Zipping up 64-bit package
set pkg=sonic-annotator-%VERSION%-win64
set dir=%pkg%\%pkg%
mkdir %pkg%
mkdir %dir%
copy CHANGELOG %dir%\CHANGELOG.txt
copy CITATION %dir%\CITATION.txt
copy COPYING %dir%\COPYING.txt
copy README.md %dir%\README.txt
copy build_win64\release\Qt5Core.dll %dir%\
copy build_win64\release\Qt5Network.dll %dir%\
copy build_win64\release\Qt5Xml.dll %dir%\
copy build_win64\release\libsndfile-1.dll %dir%\
copy build_win64\release\sonic-annotator.exe %dir%\
deploy\win\zip %pkg%.zip %pkg%

@echo Done
