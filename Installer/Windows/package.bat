@echo off

SET GGDIR=%UserProfile%\Builds\GayageumSynth
SET ACS_JSON=.\metadata.json

echo Doing regular Windows signing
%SIGNTOOL_PATH% sign /v /debug /fd SHA256 /tr "http://timestamp.acs.microsoft.com" /td SHA256 /dlib %ACS_DLIB% /dmdf %ACS_JSON% "..\..\Builds\VisualStudio2022\x64\Release\Standalone Plugin\GayageumSynth.exe"
echo "============================================================"
if %errorlevel% neq 0 GOTO END
%SIGNTOOL_PATH% sign /v /debug /fd SHA256 /tr "http://timestamp.acs.microsoft.com" /td SHA256 /dlib %ACS_DLIB% /dmdf %ACS_JSON% "..\..\Builds\VisualStudio2022\Win32\Release\Standalone Plugin\GayageumSynth.exe"
echo "============================================================"
if %errorlevel% neq 0 GOTO END
%SIGNTOOL_PATH% sign /v /debug /fd SHA256 /tr "http://timestamp.acs.microsoft.com" /td SHA256 /dlib %ACS_DLIB% /dmdf %ACS_JSON% "..\..\Builds\VisualStudio2022\x64\Release\VST3\GayageumSynth.vst3\Contents\x86_64-win\GayageumSynth.vst3"
echo "============================================================"
if %errorlevel% neq 0 GOTO END
%SIGNTOOL_PATH% sign /v /debug /fd SHA256 /tr "http://timestamp.acs.microsoft.com" /td SHA256 /dlib %ACS_DLIB% /dmdf %ACS_JSON% "..\..\Builds\VisualStudio2022\Win32\Release\VST3\GayageumSynth.vst3\Contents\x86-win\GayageumSynth.vst3"
echo "============================================================"
if %errorlevel% neq 0 GOTO END

"C:\Program Files (x86)\Inno Setup 6\iscc" "%GGDIR%\Installer\Windows\GayageumSynth 64-bit.iss"
if %errorlevel% neq 0 GOTO END
"C:\Program Files (x86)\Inno Setup 6\iscc" "%GGDIR%\Installer\Windows\GayageumSynth 32-bit.iss"
if %errorlevel% neq 0 GOTO END

echo Signing the installer packages

for /f %%i in ('dir /b/a-d/od/t:c "%GGDIR%\Installer\Windows\Output\*-64bit.exe"') do set LAST64=%%i
for /f %%i in ('dir /b/a-d/od/t:c "%GGDIR%\Installer\Windows\Output\*-32bit.exe"') do set LAST32=%%i

%SIGNTOOL_PATH% sign /v /debug /fd SHA256 /tr "http://timestamp.acs.microsoft.com" /td SHA256 /dlib %ACS_DLIB% /dmdf %ACS_JSON% "%GGDIR%\Installer\Windows\Output\%LAST64%"
echo "============================================================"
if %errorlevel% neq 0 GOTO END
%SIGNTOOL_PATH% sign /v /debug /fd SHA256 /tr "http://timestamp.acs.microsoft.com" /td SHA256 /dlib %ACS_DLIB% /dmdf %ACS_JSON% "%GGDIR%\Installer\Windows\Output\%LAST32%"
echo "============================================================"
if %errorlevel% neq 0 GOTO END

cd Output

del GayageumSynth-Windows-64bit.zip
tar.exe -a -c -f GayageumSynth-Windows-64bit.zip %LAST64%
del GayageumSynth-Windows-32bit.zip
tar.exe -a -c -f GayageumSynth-Windows-32bit.zip %LAST32%
echo "============================================================"

copy "GayageumSynth-Windows-64bit.zip" "C:\Users\dhilo\Dropbox\Public\Builds\GayageumSynth\"
copy "GayageumSynth-Windows-32bit.zip" "C:\Users\dhilo\Dropbox\Public\Builds\GayageumSynth\"
cd ..

:END

cd "%GGDIR%\Installer\Windows"

if %errorlevel% neq 0 exit /b %errorlevel%

@echo on

pause

ENDLOCAL
