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

del /q /s build_win32
del /q /s build_win64_static

@echo Rebuilding 32-bit

call .\deploy\win\build-32.bat
if %errorlevel% neq 0 exit /b %errorlevel%

@echo Rebuilding 64-bit

call .\deploy\win\build-64-static.bat
if %errorlevel% neq 0 exit /b %errorlevel%

@echo on

@echo Signing

set NAME=Open Source Developer, Christopher Cannam

@echo Signing 32-bit executables and libraries
signtool sign /v /n "%NAME%" /t http://time.certum.pl /fd sha1 /a build_win32\release\*.exe build_win32\release\*.dll

@echo Signing 64-bit executables and libraries
signtool sign /v /n "%NAME%" /t http://time.certum.pl /fd sha1 /a build_win64_static\release\*.exe build_win64_static\release\*.dll

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
copy build_win32\release\libwinpthread-1.dll %dir%
copy build_win32\release\libstd*.dll %dir%
copy build_win32\release\sonic-annotator.exe %dir%

del %pkg%.zip
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
copy build_win64_static\release\libsndfile-1.dll %dir%
copy build_win64_static\release\sonic-annotator.exe %dir%

del %pkg%.zip
powershell -NoProfile -ExecutionPolicy Bypass -Command "& 'deploy\win\zip.ps1' %pkg%.zip %pkg%"
del /q /s %pkg%

@echo Done
