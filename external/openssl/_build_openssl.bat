@echo off
SETLOCAL
IF "%ROOTDIR%"=="" (
    echo This batch file is not to be called directly. Run 'setup.bat' at the root of this repository.
    goto end
)

REM Use our perl for openssl build
SET PATH=%ROOTDIR%\external\win_perl\bin;%PATH%

IF "%1"=="64" (

    IF EXIST %ROOTDIR%\build_win64\openssl GOTO end
    
    rd /s /q c:\openssl.build64 >NUL 2>&1
    mkdir c:\openssl.build64
    mkdir c:\openssl.build64\release
    mkdir c:\openssl.build64\debug
    rd /s /q %ROOTDIR%\build_win64\openssl >NUL 2>&1
    mkdir %ROOTDIR%\build_win64\openssl

    xcopy /E %ROOTDIR%\external\openssl\src\* c:\openssl.build64\release
    xcopy /E %ROOTDIR%\external\openssl\src\* c:\openssl.build64\debug

    cd /d c:\openssl.build64\debug
    perl Configure debug-VC-WIN64A no-asm --prefix="%ROOTDIR%\build_win64\openssl" -wd4005
    call ms\do_win64a
    cd /d c:\openssl.build64\debug
    nmake -f ms\ntdll.mak
    nmake -f ms\ntdll.mak install
    ren %ROOTDIR%\build_win64\openssl\bin bin-dll-Debug
    ren %ROOTDIR%\build_win64\openssl\lib\libeay32.lib libeay32MDd.lib
    ren %ROOTDIR%\build_win64\openssl\lib\engines engines-Debug
    ren %ROOTDIR%\build_win64\openssl\lib\ssleay32.lib ssleay32MDd.lib
    if errorlevel 1 goto error
    nmake -f ms\nt.mak
    nmake -f ms\nt.mak install
    ren %ROOTDIR%\build_win64\openssl\bin bin-static-Debug
    ren %ROOTDIR%\build_win64\openssl\lib\libeay32.lib libeay32MTd.lib
    ren %ROOTDIR%\build_win64\openssl\lib\ssleay32.lib ssleay32MTd.lib
    if errorlevel 1 goto error

    cd /d c:\openssl.build64\release
    perl Configure VC-WIN64A no-asm --prefix="%ROOTDIR%\build_win64\openssl" -wd4005
    call ms\do_win64a
    cd /d c:\openssl.build64\release
    nmake -f ms\ntdll.mak
    nmake -f ms\ntdll.mak install
    ren %ROOTDIR%\build_win64\openssl\bin bin-dll-RelWithDebInfo
    ren %ROOTDIR%\build_win64\openssl\lib\libeay32.lib libeay32MD.lib
    ren %ROOTDIR%\build_win64\openssl\lib\engines engines-RelWithDebInfo
    ren %ROOTDIR%\build_win64\openssl\lib\ssleay32.lib ssleay32MD.lib
    if errorlevel 1 goto error
    nmake -f ms\nt.mak
    nmake -f ms\nt.mak install
    ren %ROOTDIR%\build_win64\openssl\bin bin-static-RelWithDebInfo
    ren %ROOTDIR%\build_win64\openssl\lib\libeay32.lib libeay32MT.lib
    ren %ROOTDIR%\build_win64\openssl\lib\ssleay32.lib ssleay32MT.lib
    if errorlevel 1 goto error

    cd /d %ROOTDIR%

) ELSE IF "%1"=="32" (

    IF EXIST %ROOTDIR%\build_win32\openssl GOTO end

    rd /s /q c:\openssl.build32 >NUL 2>&1
    mkdir c:\openssl.build32
    mkdir c:\openssl.build32\release
    mkdir c:\openssl.build32\debug
    rd /s /q %ROOTDIR%\build_win32\openssl >NUL 2>&1
    mkdir %ROOTDIR%\build_win32\openssl

    xcopy /E %ROOTDIR%\external\openssl\src\* c:\openssl.build32\release
    xcopy /E %ROOTDIR%\external\openssl\src\* c:\openssl.build32\debug
    
    cd /d c:\openssl.build32\debug
    perl Configure debug-VC-WIN32 no-asm --prefix="%ROOTDIR%\build_win32\openssl" -wd4005
    call ms\do_ms
    cd /d c:\openssl.build32\debug
    nmake -f ms\ntdll.mak
    nmake -f ms\ntdll.mak install
    ren %ROOTDIR%\build_win32\openssl\bin bin-dll-Debug
    ren %ROOTDIR%\build_win32\openssl\lib\libeay32.lib libeay32MDd.lib
    ren %ROOTDIR%\build_win32\openssl\lib\engines engines-Debug
    ren %ROOTDIR%\build_win32\openssl\lib\ssleay32.lib ssleay32MDd.lib
    if errorlevel 1 goto error
    nmake -f ms\nt.mak
    nmake -f ms\nt.mak install
    ren %ROOTDIR%\build_win32\openssl\bin bin-static-Debug
    ren %ROOTDIR%\build_win32\openssl\lib\libeay32.lib libeay32MTd.lib
    ren %ROOTDIR%\build_win32\openssl\lib\ssleay32.lib ssleay32MTd.lib
    if errorlevel 1 goto error

    cd /d c:\openssl.build32\release
    perl Configure VC-WIN32 no-asm --prefix="%ROOTDIR%\build_win32\openssl" -wd4005
    call ms\do_ms
    cd /d c:\openssl.build32\release
    nmake -f ms\ntdll.mak
    nmake -f ms\ntdll.mak install
    ren %ROOTDIR%\build_win32\openssl\bin bin-dll-RelWithDebInfo
    ren %ROOTDIR%\build_win32\openssl\lib\libeay32.lib libeay32MD.lib
    ren %ROOTDIR%\build_win32\openssl\lib\engines engines-RelWithDebInfo
    ren %ROOTDIR%\build_win32\openssl\lib\ssleay32.lib ssleay32MD.lib
    if errorlevel 1 goto error
    nmake -f ms\nt.mak
    nmake -f ms\nt.mak install
    ren %ROOTDIR%\build_win32\openssl\bin bin-static-RelWithDebInfo
    ren %ROOTDIR%\build_win32\openssl\lib\libeay32.lib libeay32MT.lib
    ren %ROOTDIR%\build_win32\openssl\lib\ssleay32.lib ssleay32MT.lib
    if errorlevel 1 goto error

    
    cd /d %ROOTDIR%
)

goto end

:error
cd /d %ROOTDIR%
rd /s/q build_win32\openssl >NUL 2>&1
rd /s/q build_win64\openssl >NUL 2>&1
exit /b 1

:end
ENDLOCAL
