@echo off
cd /d "E:\User\Desktop\code_competition_verson\22\Learning-Application-Contest\code"
call "E:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
cmake --build ../build
if %errorlevel% neq 0 (
    echo.
    echo 编译失败！
    pause
    exit /b 1
)
copy /y dims.csv ..\build\ >nul 2>&1
copy /y "包装来料数据.csv" ..\build\orders.tsv >nul 2>&1
cd /d "E:\User\Desktop\code_competition_verson\22\Learning-Application-Contest\build"
main.exe
