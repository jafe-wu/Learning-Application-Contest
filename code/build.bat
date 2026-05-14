@echo off
setlocal

REM ================================================================
REM  Cigarette Pallet - MSVC Build Script
REM
REM  Build prerequisites:
REM    - Run this script from "Developer Command Prompt for VS", OR
REM    - Let the script auto-detect VS via vswhere, OR
REM    - Manually set EASYX_DIR below
REM ================================================================

REM ==== Initialize MSVC environment via vswhere auto-detect ====
set "VCVARS="
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do (
    set "VCVARS=%%i\VC\Auxiliary\Build\vcvarsall.bat"
)
if defined VCVARS (
    call "%VCVARS%" x64 >nul
) else (
    echo [WARNING] Could not auto-detect Visual Studio.
    echo           Please run from Developer Command Prompt, or set EASYX_DIR below.
)

cd /d "%~dp0"

REM ==== EasyX path — adjust if your installation differs ====
REM  Default: D:\Visual Studio (custom install path)
set "EASYX_DIR=D:\Visual Studio\VC\Auxiliary\VS"

REM ==== Compile ====
cl /nologo /EHsc /std:c++17 /source-charset:utf-8 /execution-charset:gbk /MD /DUNICODE /D_UNICODE ^
   /I"%EASYX_DIR%\include" ^
   main.cpp PalletAlgorithm.cpp PalletizerController.cpp DimDB.cpp OrderLoader.cpp Visualizer.cpp ^
   /Fe:main.exe ^
   /link /LIBPATH:"%EASYX_DIR%\lib\x64" user32.lib gdi32.lib shell32.lib EasyXw.lib

if errorlevel 1 (
    echo.
    echo [ERROR] Build failed.
    exit /b 1
)

echo.
echo [OK] Build succeeded: main.exe
endlocal
