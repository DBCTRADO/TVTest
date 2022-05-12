@echo off
cd /d %~dp0
if exist "%Programfiles(x86)%\HTML Help Workshop\hhc.exe" (
    set HHC="%Programfiles(x86)%\HTML Help Workshop\hhc.exe"
) else (
    set HHC=hhc.exe
)
%HHC% TVTest.hhp
if not errorlevel 1 exit /b 1
exit /b 0
