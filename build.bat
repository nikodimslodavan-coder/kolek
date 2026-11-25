@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"

where g++ >nul 2>nul
if errorlevel 1 (
    echo [ERROR] g++ not found. Install MSYS2 UCRT64 and mingw-w64-ucrt-x86_64-gcc.
    exit /b 1
)

where pkg-config >nul 2>nul
if errorlevel 1 (
    echo [ERROR] pkg-config not found.
    exit /b 1
)

set SDLFLAGS=
for /f "delims=" %%i in ('pkg-config --cflags --libs sdl2 2^>nul') do set SDLFLAGS=%%i
if "%SDLFLAGS%"=="" (
    echo [ERROR] SDL2 dev files not found via pkg-config.
    echo         In MSYS2: pacman -S mingw-w64-ucrt-x86_64-SDL2
    exit /b 1
)

rem -mwindows прячет консоль; убираем его, чтобы видеть ошибки при запуске
set SDLFLAGS=%SDLFLAGS:-mwindows=%

set SOURCES=
for /r src %%f in (*.cpp) do set SOURCES=!SOURCES! "%%f"
g++ -std=c++17 -O2 -Wall -Wextra -Isrc !SOURCES! -o mansion.exe %SDLFLAGS%
if errorlevel 1 (
    echo.
    echo [ERROR] Build failed.
    exit /b 1
)

for /f "delims=" %%i in ('pkg-config --variable=prefix sdl2') do set SDLPREFIX=%%i
if exist "%SDLPREFIX%\bin\SDL2.dll" copy /Y "%SDLPREFIX%\bin\SDL2.dll" . >nul

echo.
echo Build OK: mansion.exe
