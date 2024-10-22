@echo off

:: Set up environment
CALL "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"

:: Build the solution in Release mode
MsBuild XGMDriver.sln /p:Configuration=Release /p:Platform=x64

:: Check for build errors
IF %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b %ERRORLEVEL%
) ELSE (
    echo Build succeeded!
)

:: Check if RootCA.pfx exists
if not exist "RootCA.pfx" (
    echo Generating new Root CA...
    makecert -r -pe -sv RootCA.pvk -a sha256 -len 4096 -n "CN=Fake XGM Device Publisher" -eku 1.3.6.1.5.5.7.3.3 RootCA.cer
    pvk2pfx -pvk RootCA.pvk -spc RootCA.cer -pfx RootCA.pfx -pi password
)

:: Sign the driver
echo Signing driver...
SignTool sign /f RootCA.pfx /p password /fd sha256 /tr http://timestamp.digicert.com /td sha256 x64\Release\XGMDriver\XGMDriver.dll
Inf2cat /driver:x64\Release\XGMDriver\ /os:10_X64
SignTool sign /f RootCA.pfx /p password /fd sha256 /tr http://timestamp.digicert.com /td sha256 x64\Release\XGMDriver\xgmdriver.cat

:: Build Setup
"C:\Program Files (x86)\NSIS\Bin\makensis.exe" Setup.nsi 

