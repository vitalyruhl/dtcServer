@echo off
REM DTC Server Debug Startup Script
REM Location: tools/start_server_debug.cmd

echo === DTC Server Debug Startup ===

REM Change to Debug directory
cd /d "%~dp0..\build\Debug"

REM Create config directory
if not exist "config" mkdir config

REM Copy configuration files
echo Copying configuration files...
copy "%~dp0..\config\logging.ini" "config\logging.ini"
if errorlevel 1 (
    echo [ERROR] Failed to copy logging.ini
    pause
    exit /b 1
)

copy "%~dp0..\secrets\coinbase\cdp_api_key_ECDSA.json" "config\cdp_api_key_ECDSA.json"
if errorlevel 1 (
    echo [ERROR] Failed to copy credentials
    pause
    exit /b 1
)

echo [SUCCESS] Files copied to Debug build directory

echo Starting DTC Server in Debug mode...
echo Server logs will be in: logs\dtc_server.log
echo.

REM Run directly without start command to see error output
coinbase_dtc_server.exe --credentials "config\cdp_api_key_ECDSA.json" --loglevel verbose

echo.
echo Server has stopped with exit code: %errorlevel%
ping localhost -n 5 > nul
@REM pause