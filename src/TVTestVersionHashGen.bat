@echo off
cd /d %~dp0
set headerfile=TVTestVersionHash.h
for /f "usebackq tokens=*" %%i in (`git rev-parse --short HEAD`) do set hash=%%i
if "%hash%"=="" (
	if exist %headerfile% del %headerfile%
	exit /b 0
)
find "%hash%" %headerfile% >nul
if %errorlevel% neq 0 (
	echo #define VERSION_HASH_A "%hash%">%headerfile%
)
exit /b 0
