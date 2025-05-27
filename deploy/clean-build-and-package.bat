@rem  Run this from within the top-level dir: deploy\clean-build-and-package
@echo on

@echo(
@set YN=y
@set /p YN="Proceed to clean, rebuild, package, and sign [Yn] ?"

@if "%YN%" == "Y" set YN=y
@if "%YN%" neq "y" exit /b 3

@echo Proceeding

del /q /s build_win64

@echo Rebuilding 64-bit

call .\deploy\win\build-64.bat
if %errorlevel% neq 0 exit /b %errorlevel%

@echo on

@echo Signing

set NAME=Christopher Cannam

@echo Signing executables and libraries
signtool sign /v /n "%NAME%" /t http://time.certum.pl /fd sha1 /a build_win64\*.exe build_win64\*.dll
if %errorlevel% neq 0 exit /b %errorlevel%

@echo Zipping up 64-bit package
set pkg=sonic-annotator-win64
set dir=%pkg%\%pkg%
mkdir %pkg%
mkdir %dir%
copy CHANGELOG %dir%\CHANGELOG.txt
copy CITATION %dir%\CITATION.txt
copy COPYING %dir%\COPYING.txt
copy README.md %dir%\README.txt
copy build_win64\Qt6Core.dll %dir%
copy build_win64\Qt6Network.dll %dir%
copy build_win64\Qt6Xml.dll %dir%
copy build_win64\Qt6Test.dll %dir%
copy build_win64\libsndfile-1.dll %dir%
copy build_win64\sonic-annotator.exe %dir%

set runtime="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Redist\MSVC\14.29.30133\x64\Microsoft.VC142.CRT"
               
copy %runtime%\msvcp140.DLL %dir%
copy %runtime%\vcruntime140.DLL %dir%
copy %runtime%\vcruntime140_1.DLL %dir%

del %pkg%.zip
powershell -NoProfile -ExecutionPolicy Bypass -Command "& 'deploy\win\zip.ps1' %pkg%.zip %pkg%"
del /q /s %pkg%

@echo Done
