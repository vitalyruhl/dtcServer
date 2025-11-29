@echo off
REM DTC Server Startup Script
REM Copies configuration files to Release directory and starts server
REM Location: tools/start_server.cmd

echo === DTC Server Startup ===

REM Change to Release directory
cd /d "%~dp0..\build\Release"

REM Create config directory for clean separation
if not exist "config" mkdir config

REM Copy configuration files to config subdirectory
echo Copying configuration files...
copy "%~dp0..\config\logging.ini" "config\logging.ini"
if errorlevel 1 (
    echo [ERROR] Failed to copy logging.ini - file may not exist
    echo Source: %~dp0..\config\logging.ini
    pause
    exit /b 1
)

copy "%~dp0..\secrets\coinbase\cdp_api_key_ECDSA.json" "config\cdp_api_key_ECDSA.json"
if errorlevel 1 (
    echo [ERROR] Failed to copy credentials - file may not exist
    echo Source: %~dp0..\secrets\coinbase\cdp_api_key_ECDSA.json
    pause
    exit /b 1
)

REM Verify files were copied
if exist "config\logging.ini" (
    echo [SUCCESS] Logging configuration copied
) else (
    echo [ERROR] Failed to copy logging.ini
    pause
    exit /b 1
)

if exist "config\cdp_api_key_ECDSA.json" (
    echo [SUCCESS] Coinbase credentials copied
) else (
    echo [ERROR] Failed to copy credentials
    pause
    exit /b 1
)

echo Starting DTC Server...
echo Server logs will be in: logs\dtc_server.log
echo Press Ctrl+C to stop server or close this window
echo.

REM Start server with config files in config/ subdirectory
REM Use start command to run in separate process so script can exit properly
start "DTC Server" /wait coinbase_dtc_server.exe --credentials "config\cdp_api_key_ECDSA.json" --log-config "config\logging.ini" --log-level verbose

echo.
echo Server has stopped.
REM Exit without pause so script terminates properly
exit /b 0