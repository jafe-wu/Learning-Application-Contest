@echo off
call "%~dp0build.bat"
if errorlevel 1 (
    echo.
    pause
    exit /b 1
)

echo.
echo ==== Running main.exe ====
"%~dp0main.exe"

echo.
pause
