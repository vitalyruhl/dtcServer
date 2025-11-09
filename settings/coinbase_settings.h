#pragma once

#ifndef COINBASE_SETTINGS_H
#define COINBASE_SETTINGS_H

#include <string>
#include <vector>

namespace coinbase_dtc_core {
namespace settings {

/**
 * Coinbase API Configuration Settings
 * 
 * This file contains non-sensitive configuration settings for Coinbase integration.
 * These settings can be safely committed to version control.
 * 
 * For API credentials, see secrets/coinbase.h
 */

// API Endpoints
namespace api {
    // Coinbase Public API (No authentication required)
    // Used for public market data: prices, orderbook, trades, candles
    constexpr const char* PUBLIC_API_URL = "https://api.exchange.coinbase.com";
    
    // Coinbase Advanced Trade API (Authentication required) 
    // Only needed for private account data or trading functionality
    constexpr const char* ADVANCED_API_URL = "https://api.coinbase.com/api/v3";
    
    // WebSocket Feed for real-time data
    constexpr const char* WEBSOCKET_URL = "wss://ws-feed.exchange.coinbase.com";
}

// Rate Limiting Configuration
namespace rate_limits {
    // Public API rate limits
    constexpr int PUBLIC_REQUESTS_PER_SECOND = 10;
    constexpr int PUBLIC_REQUESTS_PER_HOUR = 10000;
    
    // Advanced Trade API rate limits (with authentication)
    constexpr int AUTHENTICATED_REQUESTS_PER_SECOND = 10;
    constexpr int AUTHENTICATED_BURST_LIMIT = 20;
    
    // Request timeouts
    constexpr int REQUEST_TIMEOUT_SECONDS = 30;
    constexpr int CONNECT_TIMEOUT_SECONDS = 10;
    
    // Retry configuration
    constexpr int MAX_RETRY_ATTEMPTS = 3;
    constexpr int RETRY_BASE_DELAY_MS = 1000;  // 1 second base delay
}

// Trading Pairs Configuration
namespace products {
    // Major cryptocurrencies
    const std::vector<std::string> MAJOR_PAIRS = {
        "BTC-USD",
        "ETH-USD",
        "LTC-USD", 
        "BCH-USD"
    };
    
    // Alternative cryptocurrencies
    const std::vector<std::string> ALT_PAIRS = {
        "ADA-USD",
        "DOT-USD",
        "LINK-USD",
        "XLM-USD",
        "UNI-USD",
        "AAVE-USD"
    };
    
    // Stablecoins
    const std::vector<std::string> STABLE_PAIRS = {
        "USDC-USD",
        "USDT-USD",
        "DAI-USD"
    };
    
    // All supported products (combined)
    const std::vector<std::string> ALL_SUPPORTED = {
        // Major
        "BTC-USD", "ETH-USD", "LTC-USD", "BCH-USD",
        // Alt coins
        "ADA-USD", "DOT-USD", "LINK-USD", "XLM-USD", "UNI-USD", "AAVE-USD",
        // Stablecoins  
        "USDC-USD", "USDT-USD", "DAI-USD"
    };
    
    // Default product for testing/examples
    constexpr const char* DEFAULT_PRODUCT = "BTC-USD";
}

// Historical Data Configuration
namespace historical {
    // Granularity options (in seconds)
    namespace granularity {
        constexpr int ONE_MINUTE = 60;
        constexpr int FIVE_MINUTES = 300;
        constexpr int FIFTEEN_MINUTES = 900;
        constexpr int ONE_HOUR = 3600;
        constexpr int SIX_HOURS = 21600;
        constexpr int ONE_DAY = 86400;
        
        // Default granularity for data collection
        constexpr int DEFAULT = ONE_HOUR;
    }
    
    // Request limits
    constexpr int MAX_CANDLES_PER_REQUEST = 300;    // Coinbase limit
    constexpr int DEFAULT_DAYS_HISTORY = 30;        // 30 days of history
    constexpr int MAX_DAYS_PER_REQUEST = 5;         // To respect rate limits
    
    // Data quality settings
    constexpr double MIN_VOLUME_THRESHOLD = 0.01;   // Minimum volume to consider valid
    constexpr int MAX_PRICE_CHANGE_PERCENT = 50;    // Flag suspicious price changes
}

// Real-time Data Configuration
namespace realtime {
    // WebSocket channels
    namespace channels {
        constexpr const char* TICKER = "ticker";
        constexpr const char* LEVEL2 = "level2";
        constexpr const char* MATCHES = "matches";
        constexpr const char* HEARTBEAT = "heartbeat";
        constexpr const char* STATUS = "status";
    }
    
    // Connection settings
    constexpr int HEARTBEAT_INTERVAL_SECONDS = 30;
    constexpr int RECONNECT_DELAY_SECONDS = 5;
    constexpr int MAX_RECONNECT_ATTEMPTS = 10;
    
    // Buffer settings
    constexpr int MESSAGE_BUFFER_SIZE = 1000;
    constexpr int MAX_MESSAGE_SIZE_BYTES = 65536;   // 64KB
}

// DTC Protocol Configuration
namespace dtc {
    // Server settings
    constexpr int DEFAULT_PORT = 11099;              // Standard DTC port
    constexpr const char* DEFAULT_HOST = "0.0.0.0";
    constexpr int MAX_CONNECTIONS = 100;
    
    // Protocol settings
    constexpr const char* PROTOCOL_VERSION = "8";    // DTC Protocol version
    constexpr int HEARTBEAT_INTERVAL_SECONDS = 10;
    constexpr int CLIENT_TIMEOUT_SECONDS = 60;
    
    // Data settings
    constexpr int SYMBOL_LENGTH_MAX = 64;
    constexpr int EXCHANGE_LENGTH_MAX = 16;
}

// Database/Cache Configuration
namespace storage {
    // SQLite settings (for local cache)
    constexpr const char* DEFAULT_DB_PATH = "./data/coinbase_cache.db";
    constexpr int CONNECTION_POOL_SIZE = 10;
    constexpr int QUERY_TIMEOUT_SECONDS = 30;
    
    // Data retention
    constexpr int DEFAULT_RETENTION_DAYS = 365;     // 1 year
    constexpr int CLEANUP_INTERVAL_HOURS = 24;      // Daily cleanup
    
    // Compression settings
    constexpr bool ENABLE_COMPRESSION = true;
    constexpr int COMPRESSION_LEVEL = 6;             // 1-9, 6 is good balance
}

// Logging Configuration
namespace logging {
    // Log levels
    enum class Level {
        TRACE = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5
    };
    
    constexpr Level DEFAULT_LEVEL = Level::INFO;
    constexpr const char* DEFAULT_FORMAT = "[%Y-%m-%d %H:%M:%S] [%l] %v";
    constexpr bool ENABLE_FILE_LOGGING = true;
    constexpr const char* DEFAULT_LOG_FILE = "./logs/coinbase_dtc.log";
    constexpr int MAX_LOG_FILE_SIZE_MB = 100;
    constexpr int MAX_LOG_FILES = 5;
}

} // namespace settings
} // namespace coinbase_dtc_core

#endif // COINBASE_SETTINGS_H