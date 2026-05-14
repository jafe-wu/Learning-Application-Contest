@echo off
setlocal

REM ==== Initialize MSVC x64 environment ====
call "D:\Visual Studio\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 (
    echo [ERROR] vcvars64.bat initialization failed.
    exit /b 1
)

cd /d "%~dp0"

REM ==== Compile ====
REM /EHsc  : standard C++ exception handling
REM /std:c++17 : enable C++17
REM /utf-8 : source encoding is UTF-8 (main.cpp contains Chinese)
REM /MD    : dynamic CRT (matches EasyXw.lib)
REM /Fe    : output exe name
cl /nologo /EHsc /std:c++17 /source-charset:utf-8 /execution-charset:gbk /MD /DUNICODE /D_UNICODE ^
   /I"D:\Visual Studio\VC\Auxiliary\VS\include" ^
   main.cpp PalletAlgorithm.cpp PalletizerController.cpp DimDB.cpp OrderLoader.cpp Visualizer.cpp ^
   /Fe:main.exe ^
   /link /LIBPATH:"D:\Visual Studio\VC\Auxiliary\VS\lib\x64" ^
         user32.lib gdi32.lib shell32.lib EasyXw.lib

if errorlevel 1 (
    echo.
    echo [ERROR] Build failed.
    exit /b 1
)

echo.
echo [OK] Build succeeded: main.exe
endlocal
