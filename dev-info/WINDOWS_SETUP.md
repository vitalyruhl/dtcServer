# Windows Development Setup (Visual Studio Community)

> **✅ Status**: Fully tested and working! API connectivity verified.

This guide helps you set up a complete Windows development environment for the Coinbase DTC Core project using Visual Studio Community 2022.

## Prerequisites

### 1. Visual Studio Community 2022 (Free)

Download from: https://visualstudio.microsoft.com/vs/community/

**Required workloads**:
- ✅ **Desktop development with C++**
- ✅ **CMake tools for C++** (included in above)
- ✅ **vcpkg package manager** (will be added manually)

### 2. vcpkg Package Manager

```powershell
# Install vcpkg globally
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
C:\vcpkg\vcpkg integrate install

# Set environment variable (restart terminal after this)
setx VCPKG_ROOT "C:\vcpkg"
```

**Verification**: Restart PowerShell and check:
```powershell
echo $env:VCPKG_ROOT  # Should show: C:\vcpkg
```

## 🚀 Quick Start (3 Minutes)

### 1. Clone and Setup

```powershell
git clone https://github.com/your-repo/coinbase-dtc-core.git
cd coinbase-dtc-core
```

### 2. Open in Visual Studio

```
File → Open → Folder → Select coinbase-dtc-core directory
```

### 3. Configure and Build

1. **Select CMake preset**: `windows-vs2022` (VS will detect automatically)
2. **Install dependencies**: VS automatically installs from `vcpkg.json`:
   - `curl[ssl]` - HTTP client with SSL support  
   - `nlohmann-json` - JSON parsing library
   - `gtest` - Testing framework
3. **Build**: Press `Ctrl+Shift+B` or Build → Build All

**Expected output**:
```
Build succeeded.
    coinbase_dtc_core -> C:\...\out\build\windows-vs2022\Debug\coinbase_dtc_core.lib
    coinbase_dtc_server -> C:\...\out\build\windows-vs2022\Debug\coinbase_dtc_server.exe
    test_advanced_trade_api -> C:\...\out\build\windows-vs2022\Debug\test_advanced_trade_api.exe
```

### 4. Test API Connectivity

```powershell
# Run the API test (no credentials required)
.\out\build\windows-vs2022\Debug\test_advanced_trade_api.exe
```

**Expected results** ✅:
```
🚀 Testing Coinbase Advanced Trade API...
✅ HTTP client: libcurl (native)  
✅ GET /time - Server time (200 OK)
✅ GET /market/products - Product listings (200 OK)
✅ GET /market/products/BTC-USD - Product details (200 OK)  
✅ GET /market/product_book - Order book (200 OK)
⚠️ GET /accounts - Authentication required (expected)

🎉 All public endpoints working! Ready for development.
```

## 🔧 Development Workflow

### Fast Edit-Build-Test Cycle

```powershell
# Make code changes in VS → Save → Build (Ctrl+Shift+B) → Test
.\out\build\windows-vs2022\Debug\test_advanced_trade_api.exe

# Full rebuild if needed
cmake --build out/build/windows-vs2022 --config Debug --target clean
cmake --build out/build/windows-vs2022 --config Debug
```

### Project Configuration

The project uses **CMakePresets.json** for VS Community integration:

```json
{
  "configurePresets": [
    {
      "name": "windows-vs2022",
      "generator": "Visual Studio 17 2022",
      "toolchainFile": "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
    }
  ]
}
```

### Dependencies (vcpkg.json)

Automatically managed dependencies:
```json
{
  "dependencies": [
    {
      "name": "curl",
      "features": ["ssl"]
    },
    "nlohmann-json",
    "gtest"
  ]
}
```
- ✅ Market trades
- ⚠️  Account endpoints (requires authentication)

## Project Structure

```
coinbase-dtc-core/
├── CMakePresets.json          # VS Community configuration
├── vcpkg.json                 # Windows dependencies 
├── include/coinbase_dtc_core/
│   └── endpoints/endpoint.hpp # API endpoint management
├── tests/
│   └── test_advanced_trade_api.cpp  # Sandbox API test
└── out/                       # VS build output
```

## Development Workflow

1. **Windows (fast development)**: Use VS Community for coding/testing
2. **Linux (production)**: Use Docker for final builds and deployment
3. **CI/CD**: GitHub Actions builds both platforms

## Next Steps

- ✅ Test sandbox endpoints (public)
- 🔄 Add CDP authentication for private endpoints  
- 🔄 Implement JWT token signing
- 🔄 Add real-time WebSocket feeds
- 🔄 Integration with DTC protocol