rem  Run this from within the top-level SV dir: deploy\win64\build-32.bat
rem  To build from clean, delete the folder build_win32

set STARTPWD=%CD%

set QTDIR=C:\Qt\5.11.3\mingw53_32
if not exist %QTDIR% (
@   echo Could not find 32-bit Qt
@   exit /b 2
)

set ORIGINALPATH=%PATH%
set PATH=%PATH%;C:\Program Files (x86)\SMLNJ\bin;%QTDIR%\bin;C:\Qt\Tools\QtCreator\bin;C:\Qt\Tools\mingw530_32\bin

cd %STARTPWD%

call .\repoint install
if %errorlevel% neq 0 exit /b %errorlevel%

mkdir build_win32
cd build_win32

qmake -spec win32-g++ -r ..\sonic-annotator.pro
if %errorlevel% neq 0 exit /b %errorlevel%

mingw32-make
if %errorlevel% neq 0 exit /b %errorlevel%

copy %QTDIR%\bin\Qt5Core.dll .\release
copy %QTDIR%\bin\Qt5Network.dll .\release
copy %QTDIR%\bin\Qt5Xml.dll .\release
copy %QTDIR%\bin\Qt5Test.dll .\release
copy %QTDIR%\bin\libgcc_s_dw2-1.dll .\release
copy %QTDIR%\bin\"libstdc++-6.dll" .\release
copy %QTDIR%\bin\libwinpthread-1.dll .\release
copy %QTDIR%\plugins\platforms\qminimal.dll .\release
copy %QTDIR%\plugins\platforms\qwindows.dll .\release

.\release\test-svcore-base
.\release\test-svcore-system

.\release\sonic-annotator -v

set PATH=%ORIGINALPATH%
cd %STARTPWD%
