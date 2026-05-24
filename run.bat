@echo off
chcp 65001 >nul

set _ROOT=%~dp0

call "E:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul

echo ====== 编译 ======
cmake -S "%_ROOT%code" -B "%_ROOT%code\out\build\x64-Debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug >nul
cmake --build "%_ROOT%code\out\build\x64-Debug"
if %errorlevel% neq 0 (
    echo.
    echo 编译失败！
    pause
    exit /b 1
)

echo.
echo ====== 运行 ======
cd /d "%_ROOT%code\out\build\x64-Debug"
main.exe
