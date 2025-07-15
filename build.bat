@echo off
echo Building Galaxy CPU project with vcpkg...

:: Set vcpkg root path
set VCPKG_ROOT=C:\vcpkg

:: Check if vcpkg exists
if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    echo Error: vcpkg not found at %VCPKG_ROOT%
    echo Please make sure vcpkg is installed at C:\vcpkg
    echo You can clone it from: https://github.com/Microsoft/vcpkg.git
    echo And run: .\bootstrap-vcpkg.bat
    pause
    exit /b 1
)

:: Create build directory
if not exist "build" mkdir build

:: Configure with CMake (vcpkg manifest mode)
echo Configuring with CMake...
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows

if errorlevel 1 (
    echo Error: CMake configuration failed
    pause
    exit /b 1
)

:: Build the project
echo Building the project...
cmake --build build --config Release

if errorlevel 1 (
    echo Error: Build failed
    pause
    exit /b 1
)

echo Build completed successfully!

:: Copy shaders to output directory
echo Copying shaders to build/bin/Release...
for %%f in (src\shaders\*.vert src\shaders\*.frag src\shaders\*.tese src\shaders\*.tesc) do (
    xcopy /Y /F /I %%f build\bin\Release\
)
echo Executable location: build\bin\Release\GalaxyApp.exe
pause
