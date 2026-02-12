@echo off
echo Running Galaxy CPU application...

if exist "build\bin\GalaxyApp.exe" (
    echo Starting application...
    cd build\bin
    start GalaxyApp.exe
    cd ..\..
) else if exist "build\bin\Release\GalaxyApp.exe" (
    echo Starting application...
    cd build\bin\Release
    start GalaxyApp.exe
    cd ..\..\..
) else (
    echo Error: Application not found.
    echo Please run build.bat first.
    pause
    exit /b 1
)

pause
