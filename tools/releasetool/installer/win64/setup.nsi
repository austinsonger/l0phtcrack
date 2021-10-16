Name "L0phtCrack 7 (Win64)"
SetCompressor /SOLID lzma

# General Symbol Definitions
!define REGKEY "SOFTWARE\L0pht Holdings LLC\L0phtCrack 7 (Win64)"
!define COMPANY "L0pht Holdings, LLC"
!define URL www.l0phtcrack.com

# MUI Symbol Definitions
!define MUI_ICON ..\..\..\..\lc7\resources\lc7.ico
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_STARTMENUPAGE_REGISTRY_ROOT HKLM
!define MUI_STARTMENUPAGE_NODISABLE
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${REGKEY}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME StartMenuGroup
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "L0phtCrack 7"
!define MUI_FINISHPAGE_RUN "$WINDIR\explorer.exe" 
!define MUI_FINISHPAGE_RUN_PARAMETERS "$INSTDIR\lc7.exe"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall-colorful.ico"
!define MUI_UNFINISHPAGE_NOAUTOCLOSE

# Included files
!include Sections.nsh
!include LogicLib.nsh
!include MUI2.nsh
!include x64.nsh
!include nsProcess.nsh
!include WinVer.nsh

# Require admin
RequestExecutionLevel admin ;Require admin rights on NT6+ (When UAC is turned on)

# Reserved Files
ReserveFile "${NSISDIR}\Plugins\x86-ansi\AdvSplash.dll"
ReserveFile "${NSISDIR}\Plugins\x86-ansi\nsProcess.dll"

# Variables
Var StartMenuGroup
Var /GLOBAL retCode

# Installer attributes
OutFile ${OUTFILE}
InstallDir "$PROGRAMFILES64\L0phtCrack 7"
InstallDirRegKey HKLM "${REGKEY}" Path
CRCCheck on
XPStyle on
ShowInstDetails hide
ShowUninstDetails hide
VIProductVersion "${VERSION_NUMBER}"
VIAddVersionKey ProductName "L0phtCrack 7 (Win64)"
VIAddVersionKey ProductVersion "${VERSION_STRING}"
VIAddVersionKey CompanyName "${COMPANY}"
VIAddVersionKey CompanyWebsite "${URL}"
VIAddVersionKey FileVersion "${VERSION_STRING}"
VIAddVersionKey FileDescription ""
VIAddVersionKey LegalCopyright ""

# Installer pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\license.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_STARTMENU Application "$StartMenuGroup"
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

# Installer languages
!insertmacro MUI_LANGUAGE English

# Installer sections
Section -Main SEC0000

    SetOutPath $INSTDIR
    SetOverwrite on

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
    RmDir /r "$LOCALAPPDATA\L0pht Holdings LLC\L0phtCrack 7 (Win64)\cache"

    File /r /x remove_me.txt /x *.ilk /x *.pdb /x Thumbs.db /x *.lib /x *.exp /x *.PreARM /x *.manifest /x *.bin ${INPUTDIR}\*
    WriteRegStr HKLM "${REGKEY}\Components" Main 1

	ReadEnvStr $R0 "ALLUSERSPROFILE"
	CreateDirectory "$R0\L0phtCrack 7"

    ExecWait "$INSTDIR\vcredist_x64.exe /quiet /norestart"
SectionEnd 

Section -post SEC0001
    WriteRegStr HKLM "${REGKEY}" Path $INSTDIR
    
    SetOutPath $INSTDIR
    WriteUninstaller $INSTDIR\uninstall.exe
    
    !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
    CreateDirectory "$SMPROGRAMS\$StartMenuGroup"
    CreateShortcut "$SMPROGRAMS\$StartMenuGroup\Uninstall $(^Name).lnk" "$INSTDIR\uninstall.exe"
    CreateShortcut "$SMPROGRAMS\$StartMenuGroup\$(^Name).lnk" "$INSTDIR\lc7.exe"
    !insertmacro MUI_STARTMENU_WRITE_END

    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)" DisplayName "$(^Name)"
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)" DisplayVersion "${VERSION_STRING}"
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)" Publisher "${COMPANY}"
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)" URLInfoAbout "${URL}"
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)" DisplayIcon $INSTDIR\uninstall.exe
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)" UninstallString $INSTDIR\uninstall.exe
    WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)" NoModify 1
    WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)" NoRepair 1
SectionEnd

# Macro for selecting uninstaller sections
!macro SELECT_UNSECTION SECTION_NAME UNSECTION_ID
    Push $R0
    ReadRegStr $R0 HKLM "${REGKEY}\Components" "${SECTION_NAME}"
    StrCmp $R0 1 0 next${UNSECTION_ID}
    !insertmacro SelectSection "${UNSECTION_ID}"
    GoTo done${UNSECTION_ID}
next${UNSECTION_ID}:
    !insertmacro UnselectSection "${UNSECTION_ID}"
done${UNSECTION_ID}:
    Pop $R0
!macroend

# Uninstaller sections
Section /o -un.Main UNSEC0000

	${nsProcess::FindProcess} "lc7.exe" $retcode
	IntCmp $retcode 0 0 notRunning notRunning
	    MessageBox MB_OK|MB_ICONEXCLAMATION "L0phtCrack 7 is running. Please close it first." /SD IDOK
	    Abort
notRunning:

    RmDir /r "$LOCALAPPDATA\L0pht Holdings LLC\L0phtCrack 7 (Win64)\cache"
    MessageBox MB_YESNO|MB_DEFBUTTON2 "Do you want to completely remove all L0phtCrack application data? This will remove any historical data, registry entries and settings used by the program. This will not remove your active license from this machine if it is activated." /SD IDNO IDNO noDeleteData
        DeleteRegKey HKCU "${REGKEY}"
        RmDir /r "$APPDATA\L0pht Holdings LLC\L0phtCrack 7 (Win64)"
        RmDir "$APPDATA\L0pht Holdings LLC"
        RmDir /r "$LOCALAPPDATA\L0pht Holdings LLC\L0phtCrack 7 (Win64)"
        RmDir "$LOCALAPPDATA\L0pht Holdings LLC"
        
noDeleteData:

    RmDir /r /REBOOTOK $INSTDIR
    DeleteRegValue HKLM "${REGKEY}\Components" Main
SectionEnd

Section -un.post UNSEC0001
    DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)"
    Delete /REBOOTOK "$SMPROGRAMS\$StartMenuGroup\$(^Name).lnk"
    Delete /REBOOTOK "$SMPROGRAMS\$StartMenuGroup\Uninstall $(^Name).lnk"
    Delete /REBOOTOK "$INSTDIR\uninstall.exe"
    DeleteRegValue HKLM "${REGKEY}" StartMenuGroup
    DeleteRegValue HKLM "${REGKEY}" Path
    DeleteRegKey /IfEmpty HKLM "${REGKEY}\Components"
    DeleteRegKey /IfEmpty HKLM "${REGKEY}"
    RmDir /REBOOTOK "$SMPROGRAMS\$StartMenuGroup"
    RmDir /REBOOTOK $INSTDIR
SectionEnd

# Installer functions
Function .onInit
    SetRegView 64

    UserInfo::GetAccountType
    pop $0
    ${If} $0 != "admin" ;Require admin rights on NT4+
        MessageBox mb_iconstop "Administrator rights required!"
        SetErrorLevel 740 ;ERROR_ELEVATION_REQUIRED
        Quit
    ${EndIf}

    InitPluginsDir
    Push $R1
    File /oname=$PLUGINSDIR\spltmp.bmp ..\..\..\..\lc7\resources\lc7icon_256.png
    advsplash::show 1000 600 400 -1 $PLUGINSDIR\spltmp
    Pop $R1
    Pop $R1

    ${If} ${RunningX64}
        # Ensure are Windows Vista or greater
        ${IfNot} ${AtLeastWinVista}
            MessageBox MB_OK|MB_ICONEXCLAMATION "L0phtCrack 7 requires at least Windows Vista or Windows Server 2008." /SD IDOK
            Abort                
        ${EndIf}
    ${Else}
        MessageBox MB_OK|MB_ICONEXCLAMATION "This version of L0phtCrack 7 requires a 64-bit operating system. A 32-bit version exists for download though, but you have downloaded the 64-bit version." /SD IDOK
        Abort
    ${EndIf}       

    # Ensure LC7 isn't running yet
    ${nsProcess::FindProcess} "lc7.exe" $retcode
    IntCmp $retcode 0 0 notRunning notRunning
        MessageBox MB_OK|MB_ICONEXCLAMATION "L0phtCrack 7 is running. Please close it first." /SD IDOK
        Abort
notRunning:

FunctionEnd

# Uninstaller functions
Function un.onInit
    SetRegView 64
    ReadRegStr $INSTDIR HKLM "${REGKEY}" Path
    !insertmacro MUI_STARTMENU_GETFOLDER Application "$StartMenuGroup"
    !insertmacro SELECT_UNSECTION Main ${UNSEC0000}
FunctionEnd
