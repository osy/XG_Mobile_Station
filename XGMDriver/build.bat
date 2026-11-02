@echo off
setlocal

:: Usage: build.bat [subject]
::   With no argument, the driver is signed with a self-signed test certificate
::   (RootCA.pfx, generated on first use) and copied, together with
::   RootCA.cer, to x64\Release\SignedDriver.
::   With a subject name, the driver is signed with the EV code signing
::   certificate of that subject from the certificate store and packed into
::   x64\Release\Attestation\XGMDriver.cab for attestation signing in the
::   Partner Center hardware dashboard. Extract the signed driver it returns
::   to x64\Release\SignedDriver.
::   Then build the installer with package.bat, which packages
::   x64\Release\SignedDriver by default.
set "SIGN_SUBJECT=%~1"
set "TIMESTAMP=/tr http://timestamp.digicert.com /td sha256"
set "SIGNED_DRIVER=x64\Release\SignedDriver"

:: Set up environment
CALL "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"

:: Build the solution in Release mode
MsBuild XGMDriver.sln /p:Configuration=Release /p:Platform=x64
IF %ERRORLEVEL% NEQ 0 goto failed
echo Build succeeded!

IF DEFINED SIGN_SUBJECT goto production_signing

:: Check if RootCA.pfx exists
if not exist "RootCA.pfx" (
    echo Generating new Root CA...
    makecert -r -pe -sv RootCA.pvk -a sha256 -len 4096 -n "CN=Fake XGM Device Publisher" -eku 1.3.6.1.5.5.7.3.3 RootCA.cer
    pvk2pfx -pvk RootCA.pvk -spc RootCA.cer -pfx RootCA.pfx -pi password
)
set SIGN_CERT=/f RootCA.pfx /p password
call :sign_driver || goto failed

if exist "%SIGNED_DRIVER%" rmdir /s /q "%SIGNED_DRIVER%"
mkdir "%SIGNED_DRIVER%" || goto failed
for %%F in (XGMDriver.inf XGMDriver.dll xgmdriver.cat) do (
    copy /y "x64\Release\XGMDriver\%%F" "%SIGNED_DRIVER%\" >nul || goto failed
)
copy /y RootCA.cer "%SIGNED_DRIVER%\" >nul || goto failed
echo.
echo Test signed driver: %SIGNED_DRIVER%
echo Build the installer with: package.bat
exit /b 0

:production_signing
set SIGN_CERT=/n "%SIGN_SUBJECT%"
call :sign_driver || goto failed
:: Drop any test signed driver so package.bat cannot pick it up by mistake
if exist "%SIGNED_DRIVER%" rmdir /s /q "%SIGNED_DRIVER%"

:: Attestation submission CAB. The driver package must be in a subfolder, not
:: at the root of the CAB. Microsoft uses the catalog only to verify the
:: submitter and replaces it, and uses the PDB for crash analysis.
set "OUT=x64\Release\Attestation"
if exist "%OUT%" rmdir /s /q "%OUT%"
mkdir "%OUT%" || goto failed
(
    echo .OPTION EXPLICIT
    echo .Set CabinetFileCountThreshold=0
    echo .Set FolderFileCountThreshold=0
    echo .Set FolderSizeThreshold=0
    echo .Set MaxCabinetSize=0
    echo .Set MaxDiskFileCount=0
    echo .Set MaxDiskSize=0
    echo .Set CompressionType=MSZIP
    echo .Set Cabinet=on
    echo .Set Compress=on
    echo .Set InfFileName=NUL
    echo .Set RptFileName=NUL
    echo .Set DiskDirectoryTemplate=%OUT%
    echo .Set CabinetNameTemplate=XGMDriver.cab
    echo .Set DestinationDir=XGMDriver
    echo x64\Release\XGMDriver\XGMDriver.inf
    echo x64\Release\XGMDriver\XGMDriver.dll
    echo x64\Release\XGMDriver\xgmdriver.cat
    echo x64\Release\XGMDriver.pdb
) > "%OUT%\XGMDriver.ddf"
makecab /F "%OUT%\XGMDriver.ddf" >nul || goto failed
del "%OUT%\XGMDriver.ddf"
echo Signing submission CAB...
SignTool sign %SIGN_CERT% /fd sha256 %TIMESTAMP% "%OUT%\XGMDriver.cab" || goto failed
echo.
echo Attestation submission: %OUT%\XGMDriver.cab
echo Submit it in the Partner Center hardware dashboard, then download the
echo signed driver and extract the folder holding XGMDriver.inf to
echo %SIGNED_DRIVER%. Build the installer with: package.bat
exit /b 0

:sign_driver
echo Signing driver...
SignTool sign %SIGN_CERT% /fd sha256 %TIMESTAMP% x64\Release\XGMDriver\XGMDriver.dll || exit /b 1
Inf2cat /driver:x64\Release\XGMDriver\ /os:10_X64 || exit /b 1
SignTool sign %SIGN_CERT% /fd sha256 %TIMESTAMP% x64\Release\XGMDriver\xgmdriver.cat || exit /b 1
exit /b 0

:failed
echo Build failed!
exit /b 1
