@echo off
rem generates build_info.hpp from git: build number = commit count, plus short hash
rem usage: gen_build_info.bat <output dir>

setlocal
set "OUT_DIR=%~1"
set "OUT=%OUT_DIR%build_info.hpp"
set "TMP_OUT=%OUT_DIR%build_info.hpp.tmp"

set BUILD_NUM=0
set BUILD_HASH=nogit
for /f %%i in ('git -C "%~dp0." rev-list --count HEAD 2^>nul') do set BUILD_NUM=%%i
for /f %%i in ('git -C "%~dp0." rev-parse --short HEAD 2^>nul') do set BUILD_HASH=%%i

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

> "%TMP_OUT%" (
	echo #pragma once
	echo.
	echo #define BUILD_NUMBER %BUILD_NUM%
	echo #define BUILD_HASH "%BUILD_HASH%"
)

rem only replace when changed so files including it don't rebuild every time
fc /b "%TMP_OUT%" "%OUT%" >nul 2>&1 && (del "%TMP_OUT%") || (move /y "%TMP_OUT%" "%OUT%" >nul)

exit /b 0
