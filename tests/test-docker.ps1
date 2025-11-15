# Docker Test Runner Script for Open DTC Server (PowerShell version)
# This script provides easy commands for building and testing with Docker

param(
    [Parameter(Mandatory=$false)]
    [string]$Command = "help"
)

function Write-ColorOutput($Message, $ForegroundColor) {
    Write-Host $Message -ForegroundColor $ForegroundColor
}

function Show-Usage {
    Write-Host "Usage: .\test-docker.ps1 [command]"
    Write-Host ""
    Write-Host "Commands:"
    Write-Host "  build        Build all Docker images"
    Write-Host "  test         Run tests in Docker container"
    Write-Host "  test-basic   Run only basic tests"
    Write-Host "  run          Start the DTC server container"
    Write-Host "  dev          Start development environment"
    Write-Host "  clean        Clean up Docker images and containers"
    Write-Host "  logs         Show logs from running DTC server"
    Write-Host "  stop         Stop running DTC server"
    Write-Host ""
}

function Build-Images {
    Write-ColorOutput "Building Docker images..." "Blue"
    docker build --target builder -t open-dtc-server:builder .
    docker build --target test -t open-dtc-server:test .
    docker build --target runtime -t open-dtc-server:runtime .
    Write-ColorOutput "✅ Build completed" "Green"
}

function Run-Tests {
    Write-ColorOutput "Running tests in Docker container..." "Blue"
    docker run --rm open-dtc-server:test
}

function Run-BasicTests {
    Write-ColorOutput "Running basic tests only..." "Blue"
    docker run --rm open-dtc-server:test ctest --build-config Release --test-dir build --tests-regex "BasicTest" --output-on-failure --verbose
}

function Start-Server {
    Write-ColorOutput "Starting DTC server..." "Blue"
    docker run --rm --name dtc-server -p 11000:11000 -d open-dtc-server:runtime
    Write-ColorOutput "✅ DTC server started on port 11000" "Green"
    Write-Host "Use '.\test-docker.ps1 logs' to view logs, '.\test-docker.ps1 stop' to stop"
}

function Start-Dev {
    Write-ColorOutput "Starting development environment..." "Blue"
    docker-compose up -d dev
    Write-ColorOutput "✅ Development environment started" "Green"
    Write-Host "Use 'docker-compose exec dev /bin/bash' to access the development shell"
}

function Stop-Server {
    Write-ColorOutput "Stopping DTC server..." "Blue"
    try {
        docker stop dtc-server 2>$null
    }
    catch {
        Write-Host "Server not running"
    }
    Write-ColorOutput "✅ Server stopped" "Green"
}

function Show-Logs {
    docker logs dtc-server
}

function Cleanup {
    Write-ColorOutput "Cleaning up Docker images and containers..." "Yellow"
    try { docker stop dtc-server 2>$null } catch {}
    try { docker-compose down 2>$null } catch {}
    try { 
        docker rmi open-dtc-server:builder, open-dtc-server:test, open-dtc-server:runtime 2>$null 
    } catch {}
    docker system prune -f
    Write-ColorOutput "✅ Cleanup completed" "Green"
}

# Main command handler
switch ($Command.ToLower()) {
    "build" { Build-Images }
    "test" { Run-Tests }
    "test-basic" { Run-BasicTests }
    "run" { Start-Server }
    "stop" { Stop-Server }
    "dev" { Start-Dev }
    "logs" { Show-Logs }
    "clean" { Cleanup }
    default { Show-Usage }
}