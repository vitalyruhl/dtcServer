#!/usr/bin/env pwsh
# DTC Integration Test Script
# Starts server, runs client test, and cleans up properly for CI/CD

param(
    [string]$BuildDir = "build",
    [int]$ServerPort = 11099,
    [int]$TimeoutSeconds = 30
)

$ErrorActionPreference = "Stop"

# Function to find available port
function Find-AvailablePort {
    param([int]$StartPort = 11099)
    
    for ($port = $StartPort; $port -le ($StartPort + 100); $port++) {
        $tcpListener = $null
        try {
            $tcpListener = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Any, $port)
            $tcpListener.Start()
            $tcpListener.Stop()
            return $port
        }
        catch {
            continue
        }
        finally {
            if ($tcpListener) {
                $tcpListener.Stop()
            }
        }
    }
    throw "Could not find available port starting from $StartPort"
}

# Function to stop any running server processes
function Stop-DTCProcesses {
    Write-Host "[CLEANUP] Stopping any existing DTC server processes..."
    Get-Process | Where-Object {$_.ProcessName -like "*coinbase*" -or $_.ProcessName -like "*dtc*"} | Stop-Process -Force -ErrorAction SilentlyContinue
    Start-Sleep -Milliseconds 500
}

# Function to build the integration test
function Build-IntegrationTest {
    Write-Host "[BUILD] Building integration test..."
    
    if (!(Test-Path "$BuildDir\test_dtc_integration.exe")) {
        $compileCmd = "g++ -std=c++17 -I include -I src tests\integration\test_dtc_integration.cpp -o $BuildDir\test_dtc_integration.exe -lws2_32"
        Write-Host "[BUILD] Compiling: $compileCmd"
        Invoke-Expression $compileCmd
        
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to compile integration test"
        }
    }
    
    Write-Host "[BUILD] Integration test compiled successfully"
}

# Main test execution
function Run-IntegrationTest {
    $serverProcess = $null
    $testResult = $false
    
    try {
        # Clean up any existing processes
        Stop-DTCProcesses
        
        # Find available port
        $port = Find-AvailablePort $ServerPort
        Write-Host "[INFO] Using port: $port"
        
        # Build integration test
        Build-IntegrationTest
        
        # Start the DTC server
        Write-Host "[START] Starting DTC server on port $port..."
        $serverExe = Join-Path $BuildDir "Debug\coinbase_dtc_server.exe"
        if (!(Test-Path $serverExe)) {
            $serverExe = Join-Path $BuildDir "coinbase_dtc_server.exe"
        }
        
        if (!(Test-Path $serverExe)) {
            throw "Server executable not found: $serverExe"
        }
        
        # Start server process
        $serverProcess = Start-Process -FilePath $serverExe -WorkingDirectory (Get-Location) -PassThru -WindowStyle Hidden
        
        if (!$serverProcess) {
            throw "Failed to start server process"
        }
        
        Write-Host "[INFO] Server started with PID: $($serverProcess.Id)"
        
        # Wait for server to initialize
        Write-Host "[WAIT] Waiting for server to initialize..."
        Start-Sleep -Seconds 5
        
        # Check if server is still running
        if ($serverProcess.HasExited) {
            throw "Server process exited unexpectedly with code: $($serverProcess.ExitCode)"
        }
        
        # Test if server is listening on the port
        $portTest = Test-NetConnection -ComputerName "127.0.0.1" -Port $port -InformationLevel Quiet -WarningAction SilentlyContinue
        if (!$portTest) {
            Write-Warning "[WARN] Port test failed, but continuing with test..."
        } else {
            Write-Host "[OK] Server is listening on port $port"
        }
        
        # Run the integration test
        Write-Host "[TEST] Running integration test..."
        $testExe = Join-Path $BuildDir "test_dtc_integration.exe"
        
        $testProcess = Start-Process -FilePath $testExe -WorkingDirectory (Get-Location) -Wait -PassThru -NoNewWindow
        
        if ($testProcess.ExitCode -eq 0) {
            Write-Host "[SUCCESS] Integration test passed!" -ForegroundColor Green
            $testResult = $true
        } else {
            Write-Host "[FAILED] Integration test failed with exit code: $($testProcess.ExitCode)" -ForegroundColor Red
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
            Write-Host "[CLEANUP] Stopping server..."
            try {
                $serverProcess.Kill()
                $serverProcess.WaitForExit(5000)
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
Write-Host "=== DTC Integration Test Runner ===" -ForegroundColor Cyan
Write-Host "Build Directory: $BuildDir"
Write-Host "Server Port: $ServerPort"
Write-Host "Timeout: $TimeoutSeconds seconds"
Write-Host ""

$success = Run-IntegrationTest

if ($success) {
    Write-Host "=== INTEGRATION TEST PASSED ===" -ForegroundColor Green
    exit 0
} else {
    Write-Host "=== INTEGRATION TEST FAILED ===" -ForegroundColor Red
    exit 1
}