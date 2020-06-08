rem  Run this from within the top-level dir: deploy\win\build-and-package.bat

set STARTPWD=%CD%

if not exist "C:\Program Files (x86)\SMLNJ\bin" (
@   echo Could not find SML/NJ, required for Repoint
@   exit /b 2
)

set ORIGINALPATH=%PATH%
set PATH=C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Bin;%PATH%

@echo ""
@echo Rebuilding 32-bit

cd %STARTPWD%
del /q /s build_win32
call .\deploy\win\build-32.bat
if %errorlevel% neq 0 exit /b %errorlevel%

@echo Rebuilding 64-bit

cd %STARTPWD%
del /q /s build_win64
call .\deploy\win\build-64.bat
if %errorlevel% neq 0 exit /b %errorlevel%

set PATH=%ORIGINALPATH%
cd %STARTPWD%

@echo Done

