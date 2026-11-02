Name "XGM Driver Installer"
OutFile "XGMDriverSetup.exe"
SetOverwrite on
RequestExecutionLevel admin
Unicode True
InstallDir $PROGRAMFILES64\XGMDriver

!include LogicLib.nsh
!include Integration.nsh
!include x64.nsh
!include Sections.nsh
!include MUI2.nsh
!include FileFunc.nsh

; Set by package.bat: the folder holding the signed driver, and TEST_SIGNED
; when it also holds the test root certificate
!ifndef DRIVER_DIR
  !error "Build the installer with package.bat <driver folder>"
!endif


Page custom fnc_SpoofGPU_Show fnc_SpoofGPU_Leave
!insertmacro MUI_PAGE_COMPONENTS
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

; must follow the MUI page macros
!insertmacro MUI_LANGUAGE English

Var GpuModel
Var ASUSDRIVERDIR

; XGM disconnect loop workaround
;
; ARMOURY CRATE SE Service 3.1.3 loads ThrottleMonitorPlugin\MonitorPlugin.dll
; into its user session helper. When any display adapter is added or removed,
; its GPUControlLibrary.dll terminates AMD ADLX and then releases ADLX objects,
; crashing the helper in amdadlx64.dll. The helper also runs the eGPU switch,
; so activation is interrupted and loops. Load.conf lists the DLLs the plugin
; loads; removing the MonitorPlugin.dll line stops the crash, at the cost of
; Armoury Crate's hardware monitor.
;
; State, kept in $INSTDIR:
;   ACSE_LOAD_CONF_BACKUP   Load.conf as it was right before we changed it
;   ACSE_LOAD_CONF_PATCHED  exactly what we wrote. It exists only while we own
;                           the change, and Load.conf is compared against it to
;                           tell whether an Armoury Crate update replaced it.
!define ACSE_LOAD_CONF "$PROGRAMFILES64\ASUS\ARMOURY CRATE SE Service\ThrottleMonitorPlugin\Load.conf"
!define ACSE_LOAD_CONF_BACKUP "$INSTDIR\ThrottleMonitorPlugin_Load.conf.orig"
!define ACSE_LOAD_CONF_PATCHED "$INSTDIR\ThrottleMonitorPlugin_Load.conf.patched"

!macro AcseFunctions UN
; Push <line as returned by FileRead>; Call; Pop <1 if it names MonitorPlugin.dll>
Function ${UN}AcseIsMonitorPluginLine
  Exch $0
  Push $1
  ${Do}
    StrCpy $1 $0 1 -1
    ${If} $1 == "$\r"
    ${OrIf} $1 == "$\n"
    ${OrIf} $1 == " "
    ${OrIf} $1 == "$\t"
      StrCpy $0 $0 -1
    ${Else}
      ${ExitDo}
    ${EndIf}
  ${Loop}
  ; LogicLib == is case-insensitive
  ${If} $0 == "MonitorPlugin.dll"
    StrCpy $0 1
  ${Else}
    StrCpy $0 0
  ${EndIf}
  Pop $1
  Exch $0
FunctionEnd

; Push <file>; Call; Pop <1 if one of its lines is MonitorPlugin.dll>
Function ${UN}AcseListsMonitorPlugin
  Exch $0
  Push $1
  Push $2
  Push $3
  StrCpy $3 0
  ClearErrors
  FileOpen $1 $0 r
  ${IfNot} ${Errors}
    ${Do}
      ClearErrors
      FileRead $1 $2
      ${If} ${Errors}
        ${ExitDo}
      ${EndIf}
      Push $2
      Call ${UN}AcseIsMonitorPluginLine
      Pop $2
      ${If} $2 == 1
        StrCpy $3 1
        ${ExitDo}
      ${EndIf}
    ${Loop}
    FileClose $1
  ${EndIf}
  StrCpy $0 $3
  Pop $3
  Pop $2
  Pop $1
  Exch $0
FunctionEnd

; Push <file A>; Push <file B>; Call; Pop <1 if both exist and are byte-identical>
Function ${UN}AcseFilesEqual
  Exch $1
  Exch
  Exch $0
  Push $2
  Push $3
  Push $4
  Push $5
  Push $6
  StrCpy $6 0
  ClearErrors
  FileOpen $2 $0 r
  ${IfNot} ${Errors}
    FileOpen $3 $1 r
    ${IfNot} ${Errors}
      ${Do}
        ClearErrors
        FileReadByte $2 $4
        ${If} ${Errors}
          ; end of A: equal only if B ends here too
          ClearErrors
          FileReadByte $3 $5
          ${If} ${Errors}
            StrCpy $6 1
          ${EndIf}
          ${ExitDo}
        ${EndIf}
        FileReadByte $3 $5
        ${If} ${Errors}
        ${OrIf} $4 != $5
          ${ExitDo}
        ${EndIf}
      ${Loop}
      FileClose $3
    ${EndIf}
    FileClose $2
  ${EndIf}
  StrCpy $0 $6
  Pop $6
  Pop $5
  Pop $4
  Pop $3
  Pop $2
  Exch
  Pop $1
  Exch $0
FunctionEnd

; Armoury Crate keeps the virtual HID device open, which vetoes removing it.
; Push nothing; Call; Pop <1 if the service was running and is now stopped>
Function ${UN}AcseStopService
  Push $0
  NsExec::ExecToLog '"$SYSDIR\net.exe" stop ArmouryCrateSEService'
  Pop $0
  ${If} $0 == 0
    StrCpy $0 1
  ${Else}
    StrCpy $0 0
  ${EndIf}
  Exch $0
FunctionEnd

Function ${UN}AcseStartService
  Push $0
  DetailPrint "Starting ARMOURY CRATE SE Service..."
  NsExec::ExecToLog '"$SYSDIR\net.exe" start ArmouryCrateSEService'
  Pop $0
  Pop $0
FunctionEnd

; Armoury Crate SE reads Load.conf when the service starts
Function ${UN}AcseRestartService
  Push $0
  DetailPrint "Restarting ARMOURY CRATE SE Service..."
  NsExec::ExecToLog '"$SYSDIR\net.exe" stop ArmouryCrateSEService'
  Pop $0
  NsExec::ExecToLog '"$SYSDIR\net.exe" start ArmouryCrateSEService'
  Pop $0
  Pop $0
FunctionEnd

; Undo the workaround if we applied it. Load.conf is restored only if it is
; still exactly what we wrote; if it was replaced since (normally by an
; Armoury Crate update), it is newer than the backup and is left alone.
Function ${UN}AcseRemovePatch
  Push $0
  ${If} ${FileExists} "${ACSE_LOAD_CONF_PATCHED}"
    Push "${ACSE_LOAD_CONF}"
    Push "${ACSE_LOAD_CONF_PATCHED}"
    Call ${UN}AcseFilesEqual
    Pop $0
    ${If} $0 == 1
      DetailPrint "Restoring Armoury Crate SE Load.conf..."
      CopyFiles /SILENT "${ACSE_LOAD_CONF_BACKUP}" "${ACSE_LOAD_CONF}"
      Push "${ACSE_LOAD_CONF}"
      Push "${ACSE_LOAD_CONF_BACKUP}"
      Call ${UN}AcseFilesEqual
      Pop $0
      ${If} $0 == 1
        Call ${UN}AcseRestartService
      ${Else}
        ; keep the state so a later run can try again
        DetailPrint "Could not restore Load.conf; the backup is kept in $INSTDIR"
        Pop $0
        Return
      ${EndIf}
    ${Else}
      DetailPrint "Load.conf was replaced or removed since the workaround was applied; not restoring the backup"
    ${EndIf}
    Delete "${ACSE_LOAD_CONF_BACKUP}"
    Delete "${ACSE_LOAD_CONF_PATCHED}"
  ${EndIf}
  Pop $0
FunctionEnd
!macroend
!insertmacro AcseFunctions ""
!insertmacro AcseFunctions "un."

Function AcseApplyPatch
  Push $0
  Push $1
  Push $2
  Push $3

  ${IfNot} ${FileExists} "${ACSE_LOAD_CONF}"
    DetailPrint "Armoury Crate SE ThrottleMonitorPlugin not found; skipping the XGM disconnect loop workaround"
    Goto done
  ${EndIf}

  ; Applied by an earlier run, and Load.conf is unchanged since
  Push "${ACSE_LOAD_CONF}"
  Push "${ACSE_LOAD_CONF_PATCHED}"
  Call AcseFilesEqual
  Pop $0
  ${If} $0 == 1
    DetailPrint "The XGM disconnect loop workaround is already applied"
    Goto done
  ${EndIf}

  Push "${ACSE_LOAD_CONF}"
  Call AcseListsMonitorPlugin
  Pop $0
  ${If} $0 != 1
    ; Not removed by us (or Armoury Crate no longer lists it): leave Load.conf
    ; alone and drop any stale state so uninstall will not touch it either
    DetailPrint "Armoury Crate SE does not load MonitorPlugin.dll; nothing to do"
    Delete "${ACSE_LOAD_CONF_BACKUP}"
    Delete "${ACSE_LOAD_CONF_PATCHED}"
    Goto done
  ${EndIf}

  ; Load.conf is the original, or a newer one from an Armoury Crate update
  ; that undid the workaround. Either way it is what uninstall should restore.
  DetailPrint "Applying the XGM disconnect loop workaround..."
  Delete "${ACSE_LOAD_CONF_BACKUP}"
  CopyFiles /SILENT "${ACSE_LOAD_CONF}" "${ACSE_LOAD_CONF_BACKUP}"
  Push "${ACSE_LOAD_CONF}"
  Push "${ACSE_LOAD_CONF_BACKUP}"
  Call AcseFilesEqual
  Pop $0
  ${If} $0 != 1
    DetailPrint "Could not back up Load.conf; the workaround was not applied"
    Delete "${ACSE_LOAD_CONF_BACKUP}"
    Goto done
  ${EndIf}

  ; Copy every line except MonitorPlugin.dll, keeping the original line endings
  ClearErrors
  FileOpen $1 "${ACSE_LOAD_CONF_BACKUP}" r
  ${If} ${Errors}
    DetailPrint "Could not read the Load.conf backup; the workaround was not applied"
    Delete "${ACSE_LOAD_CONF_BACKUP}"
    Goto done
  ${EndIf}
  FileOpen $2 "${ACSE_LOAD_CONF_PATCHED}" w
  ${If} ${Errors}
    FileClose $1
    DetailPrint "Could not write $INSTDIR; the workaround was not applied"
    Delete "${ACSE_LOAD_CONF_BACKUP}"
    Goto done
  ${EndIf}
  ${Do}
    ClearErrors
    FileRead $1 $3
    ${If} ${Errors}
      ${ExitDo}
    ${EndIf}
    Push $3
    Call AcseIsMonitorPluginLine
    Pop $0
    ${If} $0 != 1
      FileWrite $2 $3
    ${EndIf}
  ${Loop}
  FileClose $2
  FileClose $1

  CopyFiles /SILENT "${ACSE_LOAD_CONF_PATCHED}" "${ACSE_LOAD_CONF}"
  Push "${ACSE_LOAD_CONF}"
  Push "${ACSE_LOAD_CONF_PATCHED}"
  Call AcseFilesEqual
  Pop $0
  ${If} $0 != 1
    DetailPrint "Could not write Load.conf; the workaround was not applied"
    CopyFiles /SILENT "${ACSE_LOAD_CONF_BACKUP}" "${ACSE_LOAD_CONF}"
    Delete "${ACSE_LOAD_CONF_BACKUP}"
    Delete "${ACSE_LOAD_CONF_PATCHED}"
    Goto done
  ${EndIf}
  Call AcseRestartService

done:
  Pop $3
  Pop $2
  Pop $1
  Pop $0
FunctionEnd

!macro EnsureAdminRights
  UserInfo::GetAccountType
  Pop $0
  ${If} $0 != "admin" ; Require admin rights on WinNT4+
    MessageBox MB_IconStop "Administrator rights required!"
    SetErrorLevel 740 ; ERROR_ELEVATION_REQUIRED
    Quit
  ${EndIf}
!macroend

Function .onInit
  !insertmacro EnsureAdminRights
  ${GetParent} $WINDIR $R0
  StrCpy $ASUSDRIVERDIR "$R0\eSupport\eDriver\Software\Peripherals\ExternalGraphics"
  ; A silent install skips the selection page, so start from its preselected
  ; choice; the model must name one of the hardware IDs in XGMDriver.inf
  StrCpy $GpuModel "GC33Y"
  Call AcsePreselect
FunctionEnd

Function un.onInit
  !insertmacro EnsureAdminRights
  ${GetParent} $WINDIR $R0
  StrCpy $ASUSDRIVERDIR "$R0\eSupport\eDriver\Software\Peripherals\ExternalGraphics"
FunctionEnd

Section "Driver" SecDriver
  SectionIn RO
  SetOutPath $INSTDIR

  ; Write driver
  File "${DRIVER_DIR}\xgmdriver.cat"
  File "${DRIVER_DIR}\XGMDriver.dll"
  File "${DRIVER_DIR}\XGMDriver.inf"
!ifdef TEST_SIGNED
  File "${DRIVER_DIR}\RootCA.cer"
!endif

  ; Write devinstall
  File "devinstall\x64\Release\devinstall.exe"

!ifdef TEST_SIGNED
  ; Install certificates; an attestation-signed driver needs none
  ${DisableX64FSRedirection}
  NsExec::ExecToLog '"$WINDIR\system32\certutil.exe" -addstore root "$INSTDIR\RootCA.cer"'
  Pop $1
  ${EnableX64FSRedirection}
!else
  ; Remove the test certificate that test-signed installs trusted, if present.
  ; The result is ignored: it may be missing or fail to delete.
  DetailPrint "Removing any test certificate..."
  ${DisableX64FSRedirection}
  NsExec::ExecToLog '"$WINDIR\system32\certutil.exe" -delstore root "Fake XGM Device Publisher"'
  Pop $1
  ${EnableX64FSRedirection}
!endif

  ; Install driver
  ; removeall also removes devices that are not currently present, which
  ; would otherwise keep their old configuration and could come back. The
  ; wildcard matches the bare ID used by older installers and every
  ; per-model ID, so changing the model replaces the device.
  DetailPrint "Stopping ARMOURY CRATE SE Service..."
  Call AcseStopService
  Pop $R9
  DetailPrint "Removing any existing device..."
  NsExec::ExecToLog '"$INSTDIR\devinstall.exe" removeall "ROOT\FakeXGMDevice*"'
  Pop $1
  ${If} $1 == 1
    SetRebootFlag true
  ${EndIf}
  DetailPrint "Removing any existing driver package..."
  NsExec::ExecToLog '"$INSTDIR\devinstall.exe" deletedriver "xgmdriver.inf"'
  Pop $1
  ; The model is encoded in the hardware ID (see XGMDriver.inf)
  DetailPrint "Installing driver for $GpuModel..."
  NsExec::ExecToLog '"$INSTDIR\devinstall.exe" install "$INSTDIR\XGMDriver.inf" "ROOT\FakeXGMDevice_$GpuModel" "VID_0B05&PID_1A9A"'
  Pop $1

  ${If} $1 != 0
    ; install creates the device before installing the driver on it; do not
    ; leave a device without a driver behind
    NsExec::ExecToLog '"$INSTDIR\devinstall.exe" removeall "ROOT\FakeXGMDevice_$GpuModel"'
    Pop $2
  ${EndIf}
  ${If} $R9 == 1
    Call AcseStartService
  ${EndIf}

  ${If} $1 != 0
    MessageBox MB_OK|MB_ICONEXCLAMATION "Driver failed to install. Check the log file."
    Delete $INSTDIR\xgmdriver.cat
    Delete $INSTDIR\XGMDriver.dll
    Delete $INSTDIR\XGMDriver.inf
    Delete $INSTDIR\devinstall.exe
    Delete $INSTDIR\RootCA.cer
    RMDir "$INSTDIR"
    Abort
  ${EndIf}

  ; Write XGM driver popup interposer
  SetOutPath $ASUSDRIVERDIR
  File "x64\Release\AsusXGDriverInst.exe"

  ; Remove the model value older installers wrote; the driver no longer reads it
  SetRegView 64
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion\WUDF\Services\XGMDriver\Parameters"
  SetRegView lastused

  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XGMDriver" "DisplayName" "Fake XGM Device"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XGMDriver" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XGMDriver" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XGMDriver" "NoRepair" 1
  WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

Section /o "XGM disconnect loop workaround" SecAcseWorkaround
  Call AcseApplyPatch
SectionEnd

; Unticking the workaround on a later run removes it again
Section "-AcseWorkaroundRemove"
  ${IfNot} ${SectionIsSelected} ${SecAcseWorkaround}
    Call AcseRemovePatch
  ${EndIf}
SectionEnd

; The workaround is opt-in, but a re-run that finds it applied keeps it
; selected, so re-running the installer (for example to change the model,
; or silently) does not undo it
Function AcsePreselect
  ${If} ${FileExists} "${ACSE_LOAD_CONF_PATCHED}"
    !insertmacro SelectSection ${SecAcseWorkaround}
  ${EndIf}
FunctionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDriver} "Fake XGM device to allow system to treat eGPU as an XG Mobile."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecAcseWorkaround} "Check only if you see an endless loop of connect and disconnect after attaching the XGM."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Section -Uninstall
  DetailPrint "Stopping ARMOURY CRATE SE Service..."
  Call un.AcseStopService
  Pop $R9
  DetailPrint "Uninstalling driver..."
  NsExec::ExecToLog '"$INSTDIR\devinstall.exe" removeall "ROOT\FakeXGMDevice*"'
  Pop $1

  ${If} $1 == 1
    SetRebootFlag true
  ${ElseIf} $1 != 0
    MessageBox MB_OK|MB_ICONEXCLAMATION "Driver failed to uninstall. You must manually delete the device."
  ${EndIf}

  DetailPrint "Deleting driver package..."
  NsExec::ExecToLog '"$INSTDIR\devinstall.exe" deletedriver "xgmdriver.inf"'
  Pop $1

  ${If} $1 != 0
    MessageBox MB_OK|MB_ICONEXCLAMATION "Driver package failed to delete. You can remove it with pnputil /delete-driver."
  ${EndIf}

  ${If} $R9 == 1
    Call un.AcseStartService
  ${EndIf}

  Call un.AcseRemovePatch

  ; Uninstall certificates
  ${DisableX64FSRedirection}
  NsExec::ExecToLog '"$WINDIR\system32\certutil.exe" -delstore root "Fake XGM Device Publisher"'
  Pop $1
  ${EnableX64FSRedirection}

  SetRegView 64
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion\WUDF\Services\XGMDriver\Parameters"
  SetRegView lastused

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XGMDriver"
  Delete $INSTDIR\xgmdriver.cat
  Delete $INSTDIR\XGMDriver.dll
  Delete $INSTDIR\XGMDriver.inf
  Delete $INSTDIR\devinstall.exe
  Delete $INSTDIR\RootCA.cer
  Delete $INSTDIR\uninstall.exe
  RMDir "$INSTDIR"
  Delete $ASUSDRIVERDIR\AsusXGDriverInst.exe
SectionEnd

; Custom page

Var hCtl_SpoofGPU
Var hCtl_SpoofGPU_GPUSpoofDescription
Var hCtl_SpoofGPU_GPUSpoofNVIDIA
Var hCtl_SpoofGPU_GPUSpoofAMD

; dialog create function
Function fnc_SpoofGPU_Create
  
  ; === SpoofGPU (type: Dialog) ===
  nsDialogs::Create 1018
  Pop $hCtl_SpoofGPU
  ${If} $hCtl_SpoofGPU == error
    Abort
  ${EndIf}
  !insertmacro MUI_HEADER_TEXT "GPU Vendor" ""
  
  ; === GPUSpoofDescription (type: Label) ===
  ${NSD_CreateLabel} 8u 6u 280u 97u "Select the vendor to spoof when an eGPU is connected."
  Pop $hCtl_SpoofGPU_GPUSpoofDescription
  
  ; === GPUSpoofNVIDIA (type: RadioButton) ===
  ${NSD_CreateRadioButton} 8u 105u 280u 12u "NVIDIA"
  Pop $hCtl_SpoofGPU_GPUSpoofNVIDIA
  ${NSD_AddStyle} $hCtl_SpoofGPU_GPUSpoofNVIDIA ${WS_GROUP}
  ${NSD_Check} $hCtl_SpoofGPU_GPUSpoofNVIDIA
  
  ; === GPUSpoofAMD (type: RadioButton) ===
  ${NSD_CreateRadioButton} 8u 121u 280u 12u "AMD"
  Pop $hCtl_SpoofGPU_GPUSpoofAMD
  
FunctionEnd

; dialog show function
Function fnc_SpoofGPU_Show
  Call fnc_SpoofGPU_Create
  nsDialogs::Show
FunctionEnd

Function fnc_SpoofGPU_Leave
  ${NSD_GetState} $hCtl_SpoofGPU_GPUSpoofNVIDIA $0
  ${If} $0 == 1
    StrCpy $GpuModel "GC33Y"
  ${Else}
    StrCpy $GpuModel "GC32L"
  ${EndIf}
FunctionEnd
