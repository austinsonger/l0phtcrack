Name "L0phtCrack 7 (Win32)"
SetCompressor /SOLID lzma

# General Symbol Definitions
!define REGKEY "SOFTWARE\L0pht Holdings LLC\L0phtCrack 7 (Win32)"
!define COMPANY "L0pht Holdings, LLC"
!define URL www.l0phtcrack.com

# MUI Symbol Definitions
!define MUI_ICON ..\..\..\..\lc7\resources\lc7.ico

# Included files
!include Sections.nsh
!include LogicLib.nsh
!include MUI2.nsh
!include x64.nsh
!include nsProcess.nsh

# Require admin
RequestExecutionLevel admin ;Require admin rights on NT6+ (When UAC is turned on)

# Reserved Files
#ReserveFile "${NSISDIR}\Plugins\x86-ansi\AdvSplash.dll"
ReserveFile "${NSISDIR}\Plugins\x86-ansi\nsProcess.dll"
ReserveFile "${NSISDIR}\Plugins\x86-ansi\ShellExecAsUser.dll"

# Variables
Var /GLOBAL retCode

# Installer pages
!insertmacro MUI_PAGE_INSTFILES

# Installer languages
!insertmacro MUI_LANGUAGE English

# Installer attributes
OutFile ${OUTFILE}
InstallDir "$PROGRAMFILES32\L0phtCrack 7"
InstallDirRegKey HKLM "${REGKEY}" Path
CRCCheck on
XPStyle on
ShowInstDetails hide
AutoCloseWindow true
VIProductVersion "${VERSION_NUMBER}"
VIAddVersionKey ProductName "L0phtCrack 7 (Win32)"
VIAddVersionKey ProductVersion "${VERSION_STRING}"
VIAddVersionKey CompanyName "${COMPANY}"
VIAddVersionKey CompanyWebsite "${URL}"
VIAddVersionKey FileVersion "${VERSION_STRING}"
VIAddVersionKey FileDescription ""
VIAddVersionKey LegalCopyright ""

# Installer sections
Section -Main SEC0000

checkagain:
    ${nsProcess::FindProcess} "lc7.exe" $retcode
    IntCmp $retcode 0 0 noWait noWait
    Sleep 1000
    Goto checkagain
noWait:

    SetOutPath $INSTDIR
    SetOverwrite on

    !if ${FULL} == 1
        # Clean up old installation, but don't remove any installed plugins
        Delete "$INSTDIR\*.*"
        RmDir /r "$INSTDIR\help"
        RmDir /r "$INSTDIR\imageformats"
        RmDir /r "$INSTDIR\platforms"
        RmDir /r "$INSTDIR\printsupport"
        RmDir /r "$INSTDIR\sqldrivers"
        RmDir /r "$INSTDIR\wordlists"
        RmDir /r "$INSTDIR\lcplugins\lc7base{2f63a714-8518-4ab6-ba7d-5440888dfc8a}"
        RmDir /r "$INSTDIR\lcplugins\lc7importunix{62a9e7aa-58f4-45ba-8fd7-5ebeb3d80551}"
        RmDir /r "$INSTDIR\lcplugins\lc7importwin{17324176-3fa7-4c1a-9204-3f391b6b3599}"
        RmDir /r "$INSTDIR\lcplugins\lc7jtr{9846c8cf-1db1-467e-83c2-c5655aa81936}"
        RmDir /r "$INSTDIR\lcplugins\lc7password{072cbbd8-84ad-481f-b0a6-398b121434db}"
        RmDir /r "$INSTDIR\lcplugins\lc7python{3c1a77e5-1c84-40ea-8a74-21ab0e1a8bbb}"
        RmDir /r "$INSTDIR\lcplugins\lc7wizard{43b6f871-7951-4b81-814b-f41ef64c993b}"
        RmDir /r "$LOCALAPPDATA\L0pht Holdings LLC\L0phtCrack 7 (Win32)\cache"

        File /r /x remove_me.txt /x *.ilk /x *.pdb /x Thumbs.db /x *.lib /x *.exp /x *.PreARM /x *.manifest /x *.bin ${INPUTDIR}\*

        ExecWait "$INSTDIR\vcredist_x86.exe /quiet /norestart"
        
    !else

        File /r /x remove_me.txt /x *.ilk /x *.pdb /x Thumbs.db /x *.lib /x *.exp /x *.PreARM /x *.manifest /x *.bin /x Qt* /x imageformats\*.* /x platforms\*.* /x printsupport\*.* /x sqldrivers\*.* /x icu* /x Crash* /x vcredist* /x dna.dll /x wordlists\*.* ${INPUTDIR}\*

    !endif

	ReadEnvStr $R0 "ALLUSERSPROFILE"
	CreateDirectory "$R0\L0phtCrack 7"

    WriteRegStr HKLM "${REGKEY}\Components" Main 1

SectionEnd

Section -post SEC0001
    WriteRegStr HKLM "${REGKEY}" Path $INSTDIR
    SetOutPath $INSTDIR
    
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)" DisplayName "$(^Name)"
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)" DisplayVersion "${VERSION_STRING}"
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)" Publisher "${COMPANY}"
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)" URLInfoAbout "${URL}"
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)" DisplayIcon $INSTDIR\uninstall.exe
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)" UninstallString $INSTDIR\uninstall.exe
    WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)" NoModify 1
    WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)" NoRepair 1

    ShellExecAsUser::ShellExecAsUser "" '"$INSTDIR\lc7.exe"' '--cleanup "$EXEPATH"'

SectionEnd

# Installer functions
Function .onInit
    SetRegView 32

    UserInfo::GetAccountType
    pop $0
    ${If} $0 != "admin" ;Require admin rights on NT4+
        MessageBox mb_iconstop "Administrator rights required!"
        SetErrorLevel 740 ;ERROR_ELEVATION_REQUIRED
        Quit
    ${EndIf}

FunctionEnd

