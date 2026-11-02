@echo off
setlocal

:: Usage: package.bat [driver folder]
::   Builds XGMDriverSetup.exe around the signed driver in [driver folder],
::   which must hold XGMDriver.inf, XGMDriver.dll and xgmdriver.cat. It
::   defaults to x64\Release\SignedDriver, where build.bat puts it.
::   - If the folder also holds RootCA.cer, the driver is test signed (the
::     output of build.bat without arguments). The installer includes
::     RootCA.cer and adds it to the trusted root store.
::   - Otherwise the driver is taken to be attestation signed by Microsoft (the
::     signed driver downloaded from the hardware dashboard), and the installer
::     does not install a certificate.
::   devinstall.exe and AsusXGDriverInst.exe come from build.bat's output.
set "DRIVER_DIR=%~f1"
if "%~1"=="" set "DRIVER_DIR=%~dp0x64\Release\SignedDriver"
:: A trailing backslash would escape the closing quote of the NSIS define
if "%DRIVER_DIR:~-1%"=="\" set "DRIVER_DIR=%DRIVER_DIR:~0,-1%"
if not exist "%DRIVER_DIR%\XGMDriver.inf" goto missing_driver
if not exist "%DRIVER_DIR%\XGMDriver.dll" goto missing_driver
if not exist "%DRIVER_DIR%\xgmdriver.cat" goto missing_driver
if not exist "%~dp0devinstall\x64\Release\devinstall.exe" goto missing_build
if not exist "%~dp0x64\Release\AsusXGDriverInst.exe" goto missing_build

if exist "%DRIVER_DIR%\RootCA.cer" goto test_signed
echo No RootCA.cer in "%DRIVER_DIR%": packaging as attestation signed
set NSIS_SIGNING=
goto package

:test_signed
echo RootCA.cer found in "%DRIVER_DIR%": packaging as test signed
set NSIS_SIGNING=/DTEST_SIGNED

:package
"C:\Program Files (x86)\NSIS\Bin\makensis.exe" "/DDRIVER_DIR=%DRIVER_DIR%" %NSIS_SIGNING% "%~dp0Setup.nsi"
if errorlevel 1 goto failed
exit /b 0

:missing_driver
echo "%DRIVER_DIR%" must contain XGMDriver.inf, XGMDriver.dll and xgmdriver.cat.
echo Run build.bat for a test signed driver, or extract the attestation signed
echo driver there.
exit /b 1

:missing_build
echo devinstall.exe or AsusXGDriverInst.exe not found; run build.bat first
exit /b 1

:failed
echo Packaging failed!
exit /b 1
