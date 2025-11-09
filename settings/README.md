# Settings Configuration

## Overview

This folder contains **non-sensitive configuration settings** for the coinbase-dtc-core project.

**🔍 These files are safe to commit to version control** - they contain no secrets or credentials.

## File Structure

```
settings/
├── README.md              # This file
└── coinbase_settings.h    # Coinbase API configuration settings
```

## Settings Categories

### 🌐 **API Configuration** (`api` namespace)
- **Endpoints**: Public API, Advanced Trade API, WebSocket URLs
- **No credentials** - just the base URLs and endpoints

### ⚡ **Rate Limiting** (`rate_limits` namespace) 
- **Request limits**: Per second, per hour, burst limits
- **Timeouts**: Connection and request timeouts
- **Retry logic**: Max attempts, delay configurations

### 📈 **Trading Pairs** (`products` namespace)
- **Product lists**: Major pairs, alt coins, stablecoins
- **Categorized**: Easy to enable/disable different asset classes
- **Default product**: For testing and examples

### 📊 **Historical Data** (`historical` namespace)
- **Granularity**: Time intervals (1m, 5m, 1h, 1d, etc.)
- **Request limits**: Max candles per request, pagination settings
- **Data quality**: Volume thresholds, price change validation

### 🔄 **Real-time Data** (`realtime` namespace)
- **WebSocket channels**: Ticker, level2, matches, etc.
- **Connection settings**: Heartbeat, reconnection logic
- **Buffer settings**: Message sizes and queue limits

### 🌉 **DTC Protocol** (`dtc` namespace)
- **Server settings**: Port, host, max connections
- **Protocol settings**: Version, heartbeat intervals
- **Data formats**: Symbol lengths, exchange identifiers

### 💾 **Storage** (`storage` namespace)
- **Database settings**: SQLite path, connection pooling
- **Data retention**: How long to keep cached data
- **Compression**: Settings for data compression

### 📝 **Logging** (`logging` namespace)
- **Log levels**: TRACE, DEBUG, INFO, WARN, ERROR, FATAL
- **File settings**: Log rotation, file sizes
- **Format settings**: Log message formatting

## Usage in Code

### Include Settings
```cpp
#include "settings/coinbase_settings.h"

using namespace coinbase_dtc_core::settings;
```

### Access Configuration
```cpp
// API endpoints
std::string public_api = api::PUBLIC_API_URL;
std::string websocket_url = api::WEBSOCKET_URL;

// Rate limits
int max_requests = rate_limits::PUBLIC_REQUESTS_PER_SECOND;
int timeout = rate_limits::REQUEST_TIMEOUT_SECONDS;

// Product lists
auto major_pairs = products::MAJOR_PAIRS;
std::string default_product = products::DEFAULT_PRODUCT;

// Historical data settings
int granularity = historical::granularity::ONE_HOUR;
int max_candles = historical::MAX_CANDLES_PER_REQUEST;

// DTC server settings
int dtc_port = dtc::DEFAULT_PORT;
```

### Environment-specific Overrides
```cpp
// Override settings at runtime based on environment
std::string api_url = api::PUBLIC_API_URL;

// Check for environment-specific override
if (std::getenv("COINBASE_API_URL")) {
    api_url = std::getenv("COINBASE_API_URL");
}

// Check for custom settings from secrets file
#include "../secrets/coinbase.h"
if (!secrets::env_overrides::CUSTOM_API_URL.empty()) {
    api_url = secrets::env_overrides::CUSTOM_API_URL;
}
```

## Configuration Philosophy

### ✅ **Settings (This Folder)**
- **Public configuration** that can be shared
- **Default values** that work for most users
- **Non-sensitive** parameters like timeouts, URLs, limits
- **Can be committed** to version control

### 🔒 **Secrets (../secrets/ folder)**
- **Private credentials** that must be protected
- **API keys, passwords, connection strings**
- **Environment-specific** sensitive overrides
- **Must NOT be committed** to version control

## Customizing Settings

### Method 1: Direct Modification
Edit `coinbase_settings.h` directly for permanent changes:

```cpp
// Change default product
constexpr const char* DEFAULT_PRODUCT = "ETH-USD";  // Changed from BTC-USD

// Adjust rate limits
constexpr int PUBLIC_REQUESTS_PER_SECOND = 5;       // More conservative
```

### Method 2: Runtime Configuration
Override settings at runtime without modifying files:

```cpp
// Create configuration wrapper
struct RuntimeConfig {
    std::string default_product = products::DEFAULT_PRODUCT;
    int requests_per_second = rate_limits::PUBLIC_REQUESTS_PER_SECOND;
    
    // Load from environment, config file, command line, etc.
    void loadFromEnvironment() {
        if (auto env_product = std::getenv("COINBASE_DEFAULT_PRODUCT")) {
            default_product = env_product;
        }
        if (auto env_rate = std::getenv("COINBASE_RATE_LIMIT")) {
            requests_per_second = std::stoi(env_rate);
        }
    }
};
```

### Method 3: Configuration File
Create a JSON/YAML configuration system:

```json
{
  "api": {
    "base_url": "https://api.exchange.coinbase.com",
    "timeout_seconds": 30
  },
  "products": {
    "default": "BTC-USD",
    "enabled": ["BTC-USD", "ETH-USD", "LTC-USD"]
  },
  "rate_limits": {
    "requests_per_second": 10
  }
}
```

## Development vs Production

### Development Settings
- **Lower rate limits** to be respectful during testing
- **Shorter timeouts** for faster feedback
- **Debug logging** enabled
- **Smaller data ranges** for testing

### Production Settings  
- **Higher rate limits** (if you have paid plan)
- **Longer timeouts** for stability
- **Info/Warn logging** only
- **Full historical data** ranges

## Integration with Other Components

### With Secrets
```cpp
#include "settings/coinbase_settings.h"
#include "../secrets/coinbase.h"

// Combine settings with credentials
class CoinbaseClient {
    std::string api_url_ = settings::api::PUBLIC_API_URL;
    std::string api_key_ = secrets::COINBASE_API_KEY;
    int timeout_ = settings::rate_limits::REQUEST_TIMEOUT_SECONDS;
};
```

### With CMake
Add to your CMakeLists.txt:

```cmake
# Include settings directory
target_include_directories(your_target PRIVATE
    ${CMAKE_SOURCE_DIR}/settings
    ${CMAKE_SOURCE_DIR}/secrets  # If using secrets
)
```

## Future Enhancements

1. **Configuration validation** at compile time
2. **Environment-based config loading** 
3. **JSON/YAML configuration files**
4. **Hot reloading** of configuration
5. **Per-environment** setting profiles
6. **Settings documentation** generation

For API credentials, see `secrets/README.md`