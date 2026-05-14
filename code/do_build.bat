@echo off
setlocal
call "D:\Visual Studio\VC\Auxiliary\Build\vcvarsall.bat" x64
cd /d "c:\Users\18149\Desktop\Learning_Application_Contest\Learning-Application-Contest\code_competition_verson"
cl /nologo /EHsc /std:c++17 /source-charset:utf-8 /execution-charset:gbk /MD /DUNICODE /D_UNICODE /I"D:\Visual Studio\VC\Auxiliary\VS\include" main.cpp PalletAlgorithm.cpp PalletizerController.cpp DimDB.cpp OrderLoader.cpp Visualizer.cpp /Fe:main.exe /link /LIBPATH:"D:\Visual Studio\VC\Auxiliary\VS\lib\x64" user32.lib gdi32.lib shell32.lib EasyXw.lib
echo === Build exit code: %errorlevel% ===
endlocal
