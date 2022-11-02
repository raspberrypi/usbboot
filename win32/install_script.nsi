!include "x64.nsh"

;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"

;--------------------------------
;General

  ;Name and file
  Name "Raspberry Pi USB boot"
  OutFile "rpiboot_setup.exe"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\Raspberry Pi"

  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\Raspberry Pi" ""

  ;Request application privileges for Windows Vista
  RequestExecutionLevel admin

;--------------------------------

;Interface Settings

  ShowInstDetails show
  !define MUI_FINISHPAGE_NOAUTOCLOSE
  !define MUI_ABORTWARNING
  !define MUI_ICON "Raspberry_Pi_Logo.ico"
  !define MUI_UNICON "Raspberry_Pi_Logo.ico"

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_WELCOME
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
; Initialisation functions
Function .onInit

  ReadRegStr $R0 HKCU "Software\Compute Module Boot" ""
  StrCmp $R0 "" done

  MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
  "'Compute Module Boot' is already installed. $\n$\nClick `OK` to remove the \
  previous version or `Cancel` to cancel this upgrade." \
  IDOK uninst
  Abort

;Run the uninstaller
uninst:
  ClearErrors

  ; Remove the left over usb_driver directory
  RmDir /r /REBOOTOK $R0\usb_driver

  ExecWait '$R0\Uninstall.exe _?=$R0'

  IfErrors no_remove_uninstaller done
    ;You can either use Delete /REBOOTOK in the uninstaller or add some code
    ;here to remove the uninstaller. Use a registry key to check
    ;whether the user has chosen to uninstall. If you are using an uninstaller
    ;components page, make sure all sections are uninstalled.
  no_remove_uninstaller:

done:

  RmDir /r /REBOOTOK $R0 

FunctionEnd

;--------------------------------
;Installer Sections

Section "Raspberry Pi USB Boot" Sec_rpiboot

  SetOutPath "$INSTDIR"
  File /r redist
  
  SetOutPath "$INSTDIR\msd"
  File /r /x bootcode4.bin ..\msd\*.*
  File ..\bootcode4.bin 
  
  SetOutPath "$INSTDIR\recovery"
  File /r /x bootcode4.bin ..\recovery\*.*
  File /oname=bootcode4.bin ..\recovery.bin
  
  SetOutPath "$INSTDIR\mass-storage-gadget"
  File /r /x bootcode4.bin ..\mass-storage-gadget\*.*
  File ..\bootcode4.bin 
   
  SetOutPath "$INSTDIR\tools"
  File /r ..\tools\*.*

  SetOutPath "$INSTDIR"
  DetailPrint "Installing BCM2708 driver..."
  ExecWait '"$INSTDIR\redist\wdi-simple.exe" -n "Raspberry Pi USB boot" -v 0x0a5c -p 0x2763 -t 0' $0 
  DetailPrint "Driver install returned $0"
  
  DetailPrint "Installing BCM2710 driver..."
  ExecWait '"$INSTDIR\redist\wdi-simple.exe" -n "Raspberry Pi USB boot" -v 0x0a5c -p 0x2764 -t 0' $0 
  DetailPrint "Driver install returned $0"
  
  DetailPrint "Installing BCM2711 driver..."
  ExecWait '"$INSTDIR\redist\wdi-simple.exe" -n "Raspberry Pi USB boot" -v 0x0a5c -p 0x2711 -t 0' $0 
  DetailPrint "Driver install returned $0"
  
  File cyggcc_s-1.dll
  File cygusb-1.0.dll
  File cygwin1.dll
  File ..\rpiboot.exe
  File rpi-mass-storage-gadget.bat
  
  CreateDirectory "$SMPROGRAMS\Raspberry Pi"
  CreateShortcut "$SMPROGRAMS\Raspberry Pi\rpiboot.lnk" "$INSTDIR\rpiboot.exe"
  CreateShortcut "$SMPROGRAMS\Raspberry Pi\Raspberry Pi - Mass Storage Gadget.lnk" "$INSTDIR\rpi-mass-storage-gadget.bat" 
  CreateShortcut "$SMPROGRAMS\Raspberry Pi\Uninstall rpiboot.lnk" "$INSTDIR\Uninstall.exe"

  ;Store installation folder
  WriteRegStr HKCU "Software\Raspberry Pi" "" $INSTDIR

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

SectionEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecDummy ${LANG_ENGLISH} "Install drivers for flashing Compute Module."

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${Sec_rpiboot} $(DESC_SecDummy)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  RmDir /r /REBOOTOK $INSTDIR\redist
  RmDir /r /REBOOTOK $INSTDIR\mass-storage-gadget
  RmDir /r /REBOOTOK $INSTDIR\msd
  RmDir /r /REBOOTOK $INSTDIR\recovery
  RmDir /r /REBOOTOK $INSTDIR\tools
  RmDir /r /REBOOTOK $INSTDIR\usb_driver

  Delete $INSTDIR\Uninstall.exe
  Delete $INSTDIR\cyggcc_s-1.dll
  Delete $INSTDIR\cygusb-1.0.dll
  Delete $INSTDIR\cygwin1.dll
  Delete $INSTDIR\rpiboot.exe

  RmDir /REBOOTOK $INSTDIR

  RmDir /r "$SMPROGRAMS\Raspberry Pi"

  DeleteRegKey /ifempty HKCU "Software\Raspberry Pi"

SectionEnd
