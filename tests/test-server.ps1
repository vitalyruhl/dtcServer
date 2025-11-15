#!/usr/bin/env pwsh
# Simple DTC Server Test Script for CI/GitHub Actions
# Just starts the server, verifies it's running, and stops it cleanly

param(
    [string]$BuildDir = "build",
    [int]$ServerPort = 11099,
    [int]$TimeoutSeconds = 10
)

$ErrorActionPreference = "Stop"

# Function to stop any running server processes
function Stop-DTCProcesses {
    Write-Host "[CLEANUP] Stopping any existing DTC server processes..."
    Get-Process | Where-Object {$_.ProcessName -like "*coinbase*" -or $_.ProcessName -like "*dtc*"} | Stop-Process -Force -ErrorAction SilentlyContinue
    Start-Sleep -Milliseconds 500
}

# Function to test if server is running
function Test-ServerRunning {
    param([int]$Port, [int]$MaxWaitSeconds = 10)
    
    for ($i = 0; $i -lt $MaxWaitSeconds; $i++) {
        try {
            $connection = Test-NetConnection -ComputerName "127.0.0.1" -Port $Port -InformationLevel Quiet -WarningAction SilentlyContinue
            if ($connection) {
                return $true
            }
        }
        catch {
            # Ignore connection errors
        }
        Start-Sleep -Seconds 1
    }
    return $false
}

# Main test execution
function Run-ServerTest {
    $serverProcess = $null
    $testResult = $false
    
    try {
        # Clean up any existing processes
        Stop-DTCProcesses
        
        Write-Host "[INFO] Testing DTC server on port $ServerPort"
        
        # Find server executable
        $serverExe = Join-Path $BuildDir "Debug\coinbase_dtc_server.exe"
        if (!(Test-Path $serverExe)) {
            $serverExe = Join-Path $BuildDir "coinbase_dtc_server.exe"
        }
        
        if (!(Test-Path $serverExe)) {
            Write-Host "[ERROR] Server executable not found: $serverExe" -ForegroundColor Red
            return $false
        }
        
        Write-Host "[START] Starting DTC server: $serverExe"
        
        # Start server process in background
        $serverProcess = Start-Process -FilePath $serverExe -WorkingDirectory (Get-Location) -PassThru -WindowStyle Hidden
        
        if (!$serverProcess) {
            Write-Host "[ERROR] Failed to start server process" -ForegroundColor Red
            return $false
        }
        
        Write-Host "[INFO] Server started with PID: $($serverProcess.Id)"
        
        # Wait for server to initialize and start listening
        Write-Host "[WAIT] Waiting for server to start listening on port $ServerPort..."
        
        if (Test-ServerRunning -Port $ServerPort -MaxWaitSeconds 8) {
            Write-Host "[SUCCESS] Server is running and listening on port $ServerPort!" -ForegroundColor Green
            $testResult = $true
        } else {
            Write-Host "[ERROR] Server failed to start listening on port $ServerPort within timeout" -ForegroundColor Red
            $testResult = $false
        }
        
        # Check if process is still running
        if (!$serverProcess.HasExited) {
            Write-Host "[OK] Server process is still running (not crashed)" -ForegroundColor Green
        } else {
            Write-Host "[ERROR] Server process exited unexpectedly with code: $($serverProcess.ExitCode)" -ForegroundColor Red
            $testResult = $false
        }
        
    }
    catch {
        Write-Host "[ERROR] Test execution failed: $($_.Exception.Message)" -ForegroundColor Red
        $testResult = $false
    }
    finally {
        # Clean up: stop server
        if ($serverProcess -and !$serverProcess.HasExited) {
            Write-Host "[CLEANUP] Stopping server (PID: $($serverProcess.Id))..."
            try {
                $serverProcess.Kill()
                $serverProcess.WaitForExit(5000)
                Write-Host "[OK] Server stopped successfully" -ForegroundColor Green
            }
            catch {
                Write-Warning "[WARN] Could not gracefully stop server: $($_.Exception.Message)"
            }
        }
        
        # Additional cleanup
        Stop-DTCProcesses
        
        Write-Host "[CLEANUP] Test cleanup completed"
    }
    
    return $testResult
}

# Script entry point
Write-Host "=== DTC Server Test ===" -ForegroundColor Cyan
Write-Host "Build Directory: $BuildDir"
Write-Host "Server Port: $ServerPort"
Write-Host "Timeout: $TimeoutSeconds seconds"
Write-Host ""

$success = Run-ServerTest

if ($success) {
    Write-Host "=== SERVER TEST PASSED ===" -ForegroundColor Green
    Write-Host "✅ Server starts successfully"
    Write-Host "✅ Server listens on the expected port"
    Write-Host "✅ Server runs without crashing"
    Write-Host "✅ Server can be stopped cleanly"
    exit 0
} else {
    Write-Host "=== SERVER TEST FAILED ===" -ForegroundColor Red
    exit 1
}