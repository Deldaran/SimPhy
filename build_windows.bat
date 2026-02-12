@echo off
echo Building Galaxy CPU project (Windows)...

:: 1. Configuration VCPKG (Adapter le chemin si besoin)
if not defined VCPKG_ROOT set VCPKG_ROOT=C:\vcpkg

if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    echo Error: vcpkg not found at %VCPKG_ROOT%
    echo Please install vcpkg or set VCPKG_ROOT correctly.
    pause
    exit /b 1
)

:: 2. Build Directory
if not exist "build" mkdir build

:: 3. Configure CMake
echo Configuring...
cmake -B build -S . ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
    -DVCPKG_TARGET_TRIPLET=x64-windows

if errorlevel 1 (
    echo CMake Configuration Failed!
    pause
    exit /b 1
)

:: 4. Build
echo Building...
cmake --build build --config Release

if errorlevel 1 (
    echo Build Failed!
    pause
    exit /b 1
)

echo Build Success! Executable in build\bin\Release\GalaxyApp.exe
pause
