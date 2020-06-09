
rem  Using Qt Base module thus:
rem  .\configure -static -static-runtime -release -platform win32-msvc -no-opengl -no-angle -nomake examples -prefix C:\Qt\5.14.1-static

set QTDIR=C:\Qt\5.14.1-static-msvc2015
if not exist %QTDIR% (
@   echo Could not find 64-bit Qt in %QTDIR%
@   exit /b 2
)

rem  Not 2019! Its APIs are too new for use in our static build
rem set vcvarsall="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat"
set vcvarsall="C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"

if not exist %vcvarsall% (
@   echo "Could not find MSVC vars batch file"
@   exit /b 2
)

set SMLNJDIR=C:\Program Files (x86)\SMLNJ
if not exist "%SMLNJDIR%\bin" (
@   echo Could not find SML/NJ, required for Repoint
@   exit /b 2
)

call %vcvarsall% amd64
if %errorlevel% neq 0 exit /b %errorlevel%

set ORIGINALPATH=%PATH%
set PATH=%PATH%;%SMLNJDIR%\bin;%QTDIR%\bin
set NAME=Open Source Developer, Christopher Cannam

set ARG=%1
shift
if "%ARG%" == "sign" (
@   echo NOTE: sign option specified, will attempt to codesign exe and msi
@   echo NOTE: starting by codesigning an unrelated executable, so we know
@   echo NOTE: whether it'll work before doing the entire build
copy "%SMLNJDIR%\bin\.run\run.x86-win32.exe" signtest.exe
signtool sign /v /n "%NAME%" /t http://time.certum.pl /fd sha1 /a signtest.exe
if errorlevel 1 exit /b %errorlevel%
signtool verify /pa signtest.exe
if errorlevel 1 exit /b %errorlevel%
del signtest.exe
@   echo NOTE: success
) else (
@   echo NOTE: sign option not specified, will not codesign anything
)

cd %STARTPWD%

call .\repoint install
if %errorlevel% neq 0 exit /b %errorlevel%

mkdir build_win64_static
cd build_win64_static

qmake -spec win32-msvc -r -tp vc ..\sonic-annotator.pro
if %errorlevel% neq 0 exit /b %errorlevel%

mkdir o

msbuild sonic-annotator.sln /t:Build /p:Configuration=Release
if %errorlevel% neq 0 exit /b %errorlevel%

rem Sadly this static build is not totally static
copy ..\sv-dependency-builds\win64-msvc\lib\libsndfile-1.dll .\release

.\release\test-svcore-base
.\release\test-svcore-system

.\release\sonic-annotator -v

set PATH=%ORIGINALPATH%
cd %STARTPWD%
