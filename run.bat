@echo off
echo Running Galaxy CPU application...

if exist "build\bin\Release\GalaxyApp.exe" (
    echo Starting application...
    "build\bin\Release\GalaxyApp.exe"
) else if exist "build\bin\Debug\GalaxyApp.exe" (
    echo Starting application (Debug)...
    "build\bin\Debug\GalaxyApp.exe"
) else (
    echo Error: Application not found. Please build the project first using build.bat
    pause
    exit /b 1
)

pause
