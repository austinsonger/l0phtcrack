@ECHO OFF
SETLOCAL

REM #############################################
IF NOT EXIST "%1" (
    echo QT directory not specified or does not exist. run 'setup.bat C:\Qt' or similar depending on where Qt is located.
    echo The supported version of Qt can be installed from: https://download.qt.io/official_releases/qt/5.12/5.12.11/qt-opensource-windows-x86-5.12.11.exe
    echo Choose the MSVC 2017 32-bit and 64-bit builds and include sources for debugging purposes
    goto end
)
SET QTENV_WINDOWS_X64=%1\5.12.11\msvc2017_64\bin\qtenv2.bat
SET QTENV_WINDOWS_X86=%1\5.12.11\msvc2017\bin\qtenv2.bat

REM #############################################

PUSHD %~dp0
SET ROOTDIR=%CD%
POPD

IF NOT DEFINED ProgramFiles(x86) (
    echo This script requires a 64-bit Windows Installation. Exiting.
    goto end
)

for /f "usebackq tokens=*" %%i in (`cmake\vswhere -version "[15.0,16.0)" -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    rem set VSCMD_DEBUG=3
    set VSDEVCMD=%%i\Common7\Tools\vsdevcmd.bat
)

IF NOT EXIST "%VSDEVCMD%" (
    echo Visual Studio 2017 is not installed correctly. Exiting.
    echo You can get the community version from Microsoft with the Dev Essentials Program:
    echo https://my.visualstudio.com/Downloads?q=visual%%20studio%%202017&wt.mc_id=o~msft~vscom~older-downloads
    goto end
)

FOR %%X IN (cmake.exe) DO (SET CMAKE_FOUND=%%~$PATH:X)
IF NOT DEFINED CMAKE_FOUND (
    echo CMake is required but it's not installed. Install CMake 3.10 or higher. Ensure it is in your path. Aborting.
    echo Cmake is available here: https://cmake.org/download/
    goto end
)

FOR %%X IN (makensis.exe) DO (SET NSIS_FOUND=%%~$PATH:X)
IF NOT DEFINED NSIS_FOUND (
    echo NSIS is required but it's not installed. Install NSIS 3.08 or higher. Ensure it is in your path. Aborting.
    echo NSIS is available here: https://nsis.sourceforge.io/Download
)

rem ------------ 64-bit Build ------------
SETLOCAL
CALL "%VSDEVCMD%" -arch=x64 -host_arch=x64
call "%QTENV_WINDOWS_X64%"

CALL "%ROOTDIR%\external\openssl\_build_openssl.bat" 64

MKDIR %ROOTDIR%\build_win64 2>NUL >NUL
PUSHD %ROOTDIR%\build_win64
cmake -G "Visual Studio 15 2017 Win64" -DOPENSSL_ROOT_DIR="%ROOTDIR%\build_win64\openssl" .. 
POPD

ENDLOCAL

rem ------------ 32-bit Build ------------
SETLOCAL
CALL "%VSDEVCMD%" -arch=x86 -host_arch=x86
call "%QTENV_WINDOWS_X86%"

CALL "%ROOTDIR%\external\openssl\_build_openssl.bat" 32

MKDIR %ROOTDIR%\build_win32 2>NUL >NUL
PUSHD %ROOTDIR%\build_win32
cmake -G "Visual Studio 15 2017" -DOPENSSL_ROOT_DIR="%ROOTDIR%\build_win32\openssl" ..
POPD

ENDLOCAL

:end
ENDLOCAL
