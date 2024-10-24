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
!insertmacro MUI_LANGUAGE English


Page custom fnc_SpoofGPU_Show fnc_SpoofGPU_Leave
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

Var GpuModel
Var ASUSDRIVERDIR

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
FunctionEnd

Function un.onInit
  !insertmacro EnsureAdminRights
  ${GetParent} $WINDIR $R0
  StrCpy $ASUSDRIVERDIR "$R0\eSupport\eDriver\Software\Peripherals\ExternalGraphics"
FunctionEnd

Section "Driver"
  SectionIn RO
  SetOutPath $INSTDIR

  ; Write driver
  File "x64\Release\XGMDriver\xgmdriver.cat"
  File "x64\Release\XGMDriver\XGMDriver.dll"
  File "x64\Release\XGMDriver\XGMDriver.inf"
  File "RootCA.cer"

  ; Write devinstall
  File "devinstall\x64\Release\devinstall.exe"

  ; Install certificates
  ${DisableX64FSRedirection}
  NsExec::ExecToLog '"$WINDIR\system32\certutil.exe" -addstore root "$INSTDIR\RootCA.cer"'
  Pop $1
  ${EnableX64FSRedirection}

  ; Install driver
  DetailPrint "Removing any existing device..."
  NsExec::ExecToLog '"$INSTDIR\devinstall.exe" remove "ROOT\FakeXGMDevice"'
  Pop $1
  DetailPrint "Installing driver..."
  NsExec::ExecToLog '"$INSTDIR\devinstall.exe" install "$INSTDIR\XGMDriver.inf" "ROOT\FakeXGMDevice" "VID_0B05&PID_1A9A"'
  Pop $1

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

  ; Write model to registry
  SetRegView 64
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion\WUDF\Services\XGMDriver\Parameters" "Model" "$GpuModel"
  SetRegView lastused

  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XGMDriver" "DisplayName" "Fake XGM Device"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XGMDriver" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XGMDriver" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XGMDriver" "NoRepair" 1
  WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

Section -Uninstall
  DetailPrint "Uninstalling driver..."
  NsExec::ExecToLog '"$INSTDIR\devinstall.exe" remove "ROOT\FakeXGMDevice"'
  Pop $1

  ${If} $1 != 0
    MessageBox MB_OK|MB_ICONEXCLAMATION "Driver failed to uninstall. You must manually delete the device."
  ${EndIf}

  ${DisableX64FSRedirection}
  NsExec::ExecToLog '"$WINDIR\system32\pnputil.exe" /delete-driver "$INSTDIR\XGMDriver.inf" /uninstall'
  Pop $1
  ${EnableX64FSRedirection}

  ; Uninstall certificates
  ${DisableX64FSRedirection}
  NsExec::ExecToLog '"$WINDIR\system32\certutil.exe" -delstore root "Fake XGM Device Publisher"'
  Pop $1
  ${EnableX64FSRedirection}

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
    StrCpy $GpuModel "GC32L"
  ${Else}
    StrCpy $GpuModel "GC31R"
  ${EndIf}
FunctionEnd
