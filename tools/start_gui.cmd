@echo off
REM DTC GUI Startup Script
REM Location: tools/start_gui.cmd

echo === DTC GUI Startup ===

REM Change to Release directory
cd /d "%~dp0..\build\Release"

REM Ensure config directory exists and logging.ini present
if not exist "config" mkdir config
if not exist "config\logging.ini" (
    echo [WARNING] logging.ini not found in Release config; copying default...
    if exist "%~dp0..\config\logging.ini" (
        copy "%~dp0..\config\logging.ini" "config\logging.ini" >nul
        echo [SUCCESS] Copied logging.ini for GUI
    ) else (
        echo [ERROR] Missing config\logging.ini; GUI will start but logging may be default
    )
)

REM Ensure logs directory exists
if not exist "logs" mkdir logs

echo Starting DTC Test Client GUI...
echo GUI logs will be in: logs\dtc_gui_client.log

echo.
start "DTC GUI" /wait dtc_test_client_gui.exe

echo.
echo GUI has stopped.
exit /b 0
