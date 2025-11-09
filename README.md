# Coinbase DTC Core

> **🚀 AUTHENTICATION READY - JWT working, DTC Integration Pending**
>
> **What's Working**: ✅ Coinbase API connectivity, JWT authentication (ES256/ECDSA), HTTP client, cross-platform builds, Docker deployment  
> **What's Missing**: ❌ DTC protocol integration, market data feeds, trading operations
>
> This project is building the infrastructure for Coinbase + DTC integration. The API layer and authentication are functional, but the core DTC features are not yet implemented.

Open-source C++17 library for integrating Coinbase Advanced Trade API with the Data Trading Client (DTC) protocol for market data feeds and trading operations.

## 🚀 Recent Major Updates (November 2025)

### ✅ **API Integration Working**

- **Live API Connectivity**: Successfully connecting to Coinbase Advanced Trade API
- **HTTP Client**: Native libcurl integration with cross-platform fallback
- **Endpoint Management**: Centralized URL management for sandbox/production environments
- **JSON Processing**: nlohmann_json integration for response parsing

### ✅ **Cross-Platform Development**

- **Windows**: Visual Studio Community 2022 support with vcpkg
- **Linux**: Docker containerized development and production
- **CMake Presets**: Platform-specific configurations
- **CI/CD**: GitHub Actions pipeline ready

### ✅ **Security & Credentials**

- **Multiple Auth Formats**: Legacy API keys + modern CDP credentials
- **Secrets Management**: Secure credential storage with .gitignore protection  
- **Environment Variables**: Production-ready configuration
- **JWT Working**: ES256/ECDSA authentication implemented for CDP

## 🎯 Current Status & Test Results

### ✅ **Infrastructure Working** (Foundation Layer)

| Component | Status | Description |
|-----------|--------|-------------|
| **HTTP Client** | ✅ **Production Ready** | libcurl + fallback system |
| **API Connectivity** | ✅ **Verified** | All public endpoints tested |
| **JWT Authentication** | ✅ **Working** | ES256/ECDSA for Advanced Trade API |
| **Cross-Platform Build** | ✅ **Working** | Windows MSVC + Linux GCC |
| **Docker Support** | ✅ **Tested** | Multi-stage builds operational |
| **Endpoint Management** | ✅ **Complete** | Sandbox/production switching |
| **JSON Parsing** | ✅ **Integrated** | nlohmann_json responses |
| **Security Framework** | ✅ **Implemented** | Secrets + env variables |

### ❌ **Missing Core Features** (Main Project Goals)

| Component | Status | Description |
|-----------|--------|-------------|
| **DTC Protocol** | ❌ **Not Implemented** | Core protocol message handling |
| **Market Data Feeds** | ❌ **Stubbed Only** | Real-time WebSocket feeds |
| **Trading Operations** | ❌ **API Ready Only** | Order management via DTC |
| **DTC Server** | ❌ **Basic Shell** | Prints startup message only |
| **Data Translation** | ❌ **Not Started** | Coinbase → DTC mapping |
| **Feed Subscription** | ❌ **Not Started** | WebSocket market data |

### Latest Test Results

```bash
🚀 Testing Coinbase Advanced Trade API...
✅ HTTP client: libcurl (native)  
✅ GET /time - Server time (200 OK)
✅ GET /market/products - Product listings (200 OK)
✅ GET /market/products/{id} - Product details (200 OK)  
✅ GET /market/product_book - Order book (200 OK)
✅ JWT Authentication - ES256/ECDSA token generation working
✅ Docker build - Multi-stage container working
⚠️ GET /accounts - Authentication verified (PowerShell tests pass)
```

## 📁 Project Architecture

```
coinbase-dtc-core/
├── CMakePresets.json                    # VS Community + Docker configs
├── vcpkg.json                          # Windows dependencies
├── WINDOWS_SETUP.md                    # Detailed Windows setup guide
├── 
├── include/coinbase_dtc_core/
│   ├── endpoints/endpoint.hpp          # 🆕 API URL management
│   ├── credentials/credentials_manager.hpp # 🆕 Auth management
│   ├── dtc/protocol.hpp               # DTC protocol stubs
│   ├── feed/coinbase/feed.hpp         # Market data feeds
│   ├── server/server.hpp              # Server components
│   └── util/log.hpp                   # Logging utilities
│
├── settings/                          # 🆕 Public configuration
│   ├── coinbase_settings.h           # API endpoints, rate limits
│   └── README.md                      # Settings documentation
│
├── secrets/                           # 🆕 Private credentials (gitignored)
│   ├── coinbase.h.template           # API key template
│   ├── cdp_api_key.json              # CDP credentials (if present)
│   └── README.md                      # Security guidelines
│
├── tests/
│   ├── test_basic.cpp                # Core functionality tests
│   └── test_advanced_trade_api.cpp   # 🆕 Live API connectivity tests
│
└── docker-compose.yml                # Container orchestration
```

## 🏗️ Quick Start

### Windows Development (Recommended for Fast Iteration)

```powershell
# One-time setup: Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
C:\vcpkg\vcpkg integrate install
setx VCPKG_ROOT "C:\vcpkg"

# Clone and build
git clone https://github.com/your-repo/coinbase-dtc-core.git
cd coinbase-dtc-core
cmake --preset windows-vs2022
cmake --build out/build/windows-vs2022 --config Debug

# Test API connectivity
.\out\build\windows-vs2022\Debug\test_advanced_trade_api.exe
```

### Linux/Docker (Production Environment)

```bash
# Development
docker-compose --profile dev run --rm coinbase-dtc-core-dev bash -c \
  "cmake -B build && cmake --build build && build/test_advanced_trade_api"

# Production
docker-compose up coinbase-dtc-core
```

## 🔒 Security & Authentication

### Current Implementation

```cpp
// Environment variables (production)
COINBASE_API_KEY="your-api-key"
COINBASE_API_SECRET="your-api-secret" 
COINBASE_PASSPHRASE="your-passphrase"

// CDP format (new)
CDP_API_KEY_ID="your-cdp-key-id"
CDP_PRIVATE_KEY="your-private-key"

// Or via JSON file (development)
secrets/cdp_api_key.json
```

### Security Features

- **🔒 Git Protection**: `secrets/` directory excluded from version control
- **🌍 Environment Variables**: Production credential management
- **📁 Local Files**: Development convenience with security
- **🔄 Multiple Formats**: Legacy and CDP authentication support
- **⚡ JWT Working**: ES256/ECDSA authentication for Advanced Trade API

## 🎯 Roadmap & Next Steps

### Phase 1: Core Infrastructure ✅ **COMPLETE**

- [x] Cross-platform build system (CMake + vcpkg)
- [x] HTTP client integration (libcurl + fallback)
- [x] Basic API connectivity (all public endpoints working)
- [x] Development environment setup (Windows + Docker)
- [x] Endpoint management (sandbox/production)
- [x] Security framework (secrets + environment variables)

### Phase 2: Authentication ✅ **COMPLETE**

- [x] Credential management framework
- [x] Multiple authentication format support
- [x] **Complete**: JWT token generation for CDP (ES256/ECDSA)
- [x] **Complete**: API request signing with JWT
- [ ] **Future**: Token refresh logic (as needed)

### Phase 3: DTC Protocol ❌ **NOT STARTED** (Core Missing Feature!)

- [ ] **Critical**: DTC protocol message structures
- [ ] **Critical**: DTC server implementation (currently just prints startup)
- [ ] **Critical**: Message encoding/decoding
- [ ] **Critical**: DTC client connection handling
- [ ] **Critical**: Protocol version negotiation

### Phase 4: Market Data Integration ❌ **NOT STARTED** (Main Purpose!)

- [ ] **Critical**: WebSocket real-time feeds from Coinbase
- [ ] **Critical**: Coinbase message → DTC message mapping  
- [ ] **Critical**: Market data streaming to DTC clients
- [ ] **Critical**: Historical data retrieval
- [ ] **Critical**: Order book management

### Phase 5: Trading Operations ❌ **NOT STARTED**

- [ ] Order placement/cancellation via DTC protocol
- [ ] Account management with DTC integration
- [ ] Position tracking through DTC
- [ ] Risk management
- [ ] Portfolio operations

### Phase 6: Production Hardening **PLANNED**

- [ ] Error handling & retry logic
- [ ] Connection pooling
- [ ] Rate limiting compliance
- [ ] Monitoring & alerting
- [ ] Performance optimization

---

**⚠️ Reality Check**: While we have solid API connectivity, the core DTC features that make this project useful are not yet implemented. The project is currently at the "foundation" stage.

## License

Licensed under the Apache License, Version 2.0. See `LICENSE` for details.


<br>
<br>


## Donate

<table align="center" width="100%" border="0" bgcolor:=#3f3f3f>
<tr align="center">
<td align="center">  
if you prefer a one-time donation

[![donate-Paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://paypal.me/FamilieRuhl)

</td>

<td align="center">  
Become a patron, by simply clicking on this button (**very appreciated!**):

[![Become a patron](https://c5.patreon.com/external/logo/become_a_patron_button.png)](https://www.patreon.com/join/6555448/checkout?ru=undefined)

</td>
</tr>
</table>

<br>
<br>

---
