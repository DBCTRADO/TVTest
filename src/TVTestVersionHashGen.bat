@echo off
cd /d %~dp0
set headerfile=TVTestVersionHash.h
for /f "usebackq tokens=*" %%i in (`git rev-parse --short HEAD`) do set hash=%%i
if not "%hash%"=="" (
	echo #define VERSION_HASH_A "%hash%">%headerfile%
) else if exist %headerfile% (
	del %headerfile%
)
exit /b 0
