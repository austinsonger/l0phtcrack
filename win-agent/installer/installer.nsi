Name "L0phtCrack 7 Remote Agent"

# General Symbol Definitions
!define REGKEY "SOFTWARE\L0phtCrack 7 Remote Agent"
!define COMPANY "L0pht Holdings, LLC"
!define URL www.l0phtcrack.com
!define VERSION_NUMBER 7.0.0.0
!define VERSION_STRING "7.0.0"


# MUI Symbol Definitions
!define MUI_ICON ..\..\lc7\resources\lc7.ico
!define MUI_FINISHPAGE_NOAUTOCLOSE
#!define MUI_STARTMENUPAGE_REGISTRY_ROOT HKLM
#!define MUI_STARTMENUPAGE_NODISABLE
#!define MUI_STARTMENUPAGE_REGISTRY_KEY "${REGKEY}"
#!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME StartMenuGroup
#!define MUI_STARTMENUPAGE_DEFAULTFOLDER "L0phtCrack 7 Remote Agent"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall-colorful.ico"
!define MUI_UNFINISHPAGE_NOAUTOCLOSE

# Included files
!include Sections.nsh
!include LogicLib.nsh
!include MUI2.nsh
!include x64.nsh
!include nsProcess.nsh
!include nsDialogs.nsh
!include WinVer.nsh

# Require admin
RequestExecutionLevel admin ;Require admin rights on NT6+ (When UAC is turned on)

# Reserved Files
ReserveFile "${NSISDIR}\Plugins\x86-ansi\nsProcess.dll"


# Custom dialog for firewall
Var Dialog
Var Label
Var RadioButtonYES
Var RadioButtonNO

Function nsDialogFirewallPage

    nsDialogs::Create 1018
    Pop $Dialog

    ${If} $Dialog == error
        Abort
    ${EndIf}

    ${NSD_CreateLabel} 0 0 100% 24% "Your firewall rules and system configuration must allow the LC7 Agent to be reached remotely. Only administrators will be able to reach this service."
    Pop $Label

    ${NSD_CreateRadioButton} 0 25% 80% 6% "Yes, modify the firewall rules now automatically."
    Pop $RadioButtonYES
    ${NSD_AddStyle} $RadioButtonYES ${SS_CENTER}

    ${NSD_CreateRadioButton} 0 40% 80% 6% "No, I will modify the rules myself after reading the documentation."
    Pop $RadioButtonNO
    ${NSD_AddStyle} $RadioButtonNO ${SS_CENTER}

    ${NSD_Check} $RadioButtonYES

    nsDialogs::Show

FunctionEnd


Function nsDialogFirewallPageLeave

    ${NSD_GetState} $RadioButtonYES $0

    ${If} $0 == ${BST_CHECKED}

        ${If} ${RunningX64}
            ${DisableX64FSRedirection}
        ${EndIf}

        ExecWait '"$WINDIR\lc7agent.exe" /configuresmb'

    ${EndIf}

FunctionEnd

# Variables
#Var StartMenuGroup
!define StartMenuGroup "L0phtCrack 7 Remote Agent"

# Installer pages
!insertmacro MUI_PAGE_WELCOME
#!insertmacro MUI_PAGE_STARTMENU Application $StartMenuGroup
!insertmacro MUI_PAGE_INSTFILES
Page custom nsDialogFirewallPage nsDialogFirewallPageLeave
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

# Installer languages
!insertmacro MUI_LANGUAGE English

# Installer attributes
OutFile ..\..\dist\common\common\lcplugins\lc7importwin{17324176-3fa7-4c1a-9204-3f391b6b3599}\lcagent\lc7remoteagent.exe

CRCCheck on
XPStyle on
ShowInstDetails show
VIProductVersion "${VERSION_NUMBER}"
VIAddVersionKey ProductName "L0phtCrack 7 Remote Agent"
VIAddVersionKey ProductVersion "${VERSION_STRING}"
VIAddVersionKey CompanyName "${COMPANY}"
VIAddVersionKey CompanyWebsite "${URL}"
VIAddVersionKey FileVersion "${VERSION_STRING}"
VIAddVersionKey FileDescription ""
VIAddVersionKey LegalCopyright ""
InstallDirRegKey HKLM "${REGKEY}" Path
ShowUninstDetails show

# Fixed start menu group

# Installer sections
Section -Main SEC0000

    ${If} ${RunningX64} 
        ${DisableX64FSRedirection}
        SetOutPath "$PROGRAMFILES64\L0phtCrack 7 Remote Agent"
    ${Else}
        SetOutPath "$PROGRAMFILES\L0phtCrack 7 Remote Agent"
    ${EndIf}

    SetOverwrite on

    # Uninstall old binary if one exists
    ${If} ${FileExists} "$WINDIR\lc7agent.exe"
        ExecWait '"$WINDIR\lc7agent.exe" /remove'
    ${EndIf}
    
    # Copy files
    ${If} ${RunningX64}
        CopyFiles "$EXEDIR\lc7agent64.exe" "$WINDIR\lc7agent.exe"
        CopyFiles "$EXEDIR\lc7dump64.dll" "$WINDIR\lc7dump.dll"
    ${Else}
        CopyFiles "$EXEDIR\lc7agent.exe" "$WINDIR\lc7agent.exe"
        CopyFiles "$EXEDIR\lc7dump.dll" "$WINDIR\lc7dump.dll"
    ${EndIf}

    WriteRegStr HKLM "${REGKEY}\Components" Main 1
SectionEnd 

Section -post SEC0001
    WriteRegStr HKLM "${REGKEY}" Path $INSTDIR

    ${If} ${RunningX64}
        ${DisableX64FSRedirection}
        SetOutPath "$PROGRAMFILES64\L0phtCrack 7 Remote Agent"
    ${Else}
        SetOutPath "$PROGRAMFILES\L0phtCrack 7 Remote Agent"
    ${EndIf}
    WriteUninstaller $OUTDIR\uninstall.exe

    SetOutPath "$SMPROGRAMS\${StartMenuGroup}"
    CreateShortcut "$SMPROGRAMS\${StartMenuGroup}\Uninstall $(^Name).lnk" $INSTDIR\uninstall.exe

    ${If} ${RunningX64}
        ${DisableX64FSRedirection}
        SetOutPath "$PROGRAMFILES64\L0phtCrack 7 Remote Agent"
    ${Else}
        SetOutPath "$PROGRAMFILES\L0phtCrack 7 Remote Agent"
    ${EndIf}
    
    # Run service installer
    ExecWait '"$WINDIR\lc7agent.exe" /install'

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

    ${If} ${RunningX64}
        ${DisableX64FSRedirection}
    ${EndIf}

    ExecWait '"$WINDIR\lc7agent.exe" /remove'

    RmDir /r /REBOOTOK $INSTDIR
    Delete /REBOOTOK "$WINDIR\lc7agent.exe"
    Delete /REBOOTOK "$WINDIR\lc7dump.dll"

    DeleteRegValue HKLM "${REGKEY}\Components" Main
SectionEnd

Section -un.post UNSEC0001
    DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)"
    Delete /REBOOTOK "$SMPROGRAMS\${StartMenuGroup}\$(^Name).lnk"
    Delete /REBOOTOK "$SMPROGRAMS\${StartMenuGroup}\Uninstall $(^Name).lnk"
    Delete /REBOOTOK $INSTDIR\uninstall.exe
    DeleteRegValue HKLM "${REGKEY}" StartMenuGroup
    DeleteRegValue HKLM "${REGKEY}" Path
    DeleteRegKey /IfEmpty HKLM "${REGKEY}\Components"
    DeleteRegKey /IfEmpty HKLM "${REGKEY}"
    RmDir /REBOOTOK "$SMPROGRAMS\${StartMenuGroup}"
    RmDir /REBOOTOK $INSTDIR
SectionEnd

# Installer functions
Function .onInit
    UserInfo::GetAccountType
    pop $0
    ${If} $0 != "admin" ;Require admin rights on NT4+
        MessageBox mb_iconstop "Administrator rights required!"
        SetErrorLevel 740 ;ERROR_ELEVATION_REQUIRED
        Quit
    ${EndIf}

    ${If} ${RunningX64}
        ${DisableX64FSRedirection}
        StrCpy $INSTDIR "$PROGRAMFILES64\L0phtCrack 7 Remote Agent"
    ${Else}
        StrCpy $INSTDIR "$PROGRAMFILES\L0phtCrack 7 Remote Agent"
    ${EndIf}

FunctionEnd

# Uninstaller functions
Function un.onInit
    ReadRegStr $INSTDIR HKLM "${REGKEY}" Path
#    !insertmacro MUI_STARTMENU_GETFOLDER Application ${StartMenuGroup}
    !insertmacro SELECT_UNSECTION Main ${UNSEC0000}
FunctionEnd
