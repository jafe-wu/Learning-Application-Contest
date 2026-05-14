@echo off
setlocal
call "D:\Visual Studio\VC\Auxiliary\Build\vcvars64.bat" >/dev/null 2>&1
if errorlevel 1 (
    echo [ERROR] vcvars64.bat failed
    exit /b 1
)
cd /d "c:\Users\18149\Desktop\Learning_Application_Contest\Learning-Application-Contest\code_competition_verson"
cl /nologo /EHsc /std:c++17 /source-charset:utf-8 /execution-charset:gbk /MD /DUNICODE /D_UNICODE /I"D:\Visual Studio\VC\Auxiliary\VS\include" main.cpp PalletAlgorithm.cpp PalletizerController.cpp DimDB.cpp OrderLoader.cpp Visualizer.cpp /Fe:main.exe /link /LIBPATH:"D:\Visual Studio\VC\Auxiliary\VS\lib\x64" user32.lib gdi32.lib shell32.lib EasyXw.lib
if errorlevel 1 (
    echo [ERROR] Build failed
    exit /b 1
)
echo [OK] Build succeeded
endlocal
