#pragma once

#ifndef COINBASE_API_SECRETS_H
#define COINBASE_API_SECRETS_H

#include <string>

namespace coinbase_dtc_core {
namespace secrets {

/**
 * Coinbase API Credentials
 * 
 * SECURITY WARNING: This file contains sensitive API credentials.
 * - Never commit this file to version control
 * - Keep these credentials secure and rotate them regularly  
 * - Use minimal permissions (View only for market data)
 * - Consider IP whitelisting for production use
 * 
 * For non-sensitive configuration, see settings/coinbase_settings.h
 */

// API Credentials
// Get these from: https://cloud.coinbase.com/ or Coinbase Pro settings
// Leave empty strings "" to use public API only (no authentication)

// Legacy API credentials (older format)
const std::string COINBASE_API_KEY = "";                    // Your Coinbase API key
const std::string COINBASE_API_SECRET = "";                 // Your Coinbase API secret  
const std::string COINBASE_PASSPHRASE = "";                 // Your passphrase (legacy API only)

// Coinbase Developer Platform (CDP) credentials (newer format)
// These are loaded from cdp_api_key_ECDSA.json file in this folder
const std::string CDP_JSON_FILE_PATH = "secrets/cdp_api_key_ECDSA.json";

// Manual CDP credentials (if not using JSON file)
const std::string CDP_API_KEY_ID = "";                      // CDP API Key ID
const std::string CDP_PRIVATE_KEY = "";                     // CDP Private Key (Base64 encoded)

// Optional: Environment-specific overrides
// These can be set via environment variables at runtime
namespace env_overrides {
    // Database connection string (if different from default)
    const std::string DATABASE_URL = "";                    // e.g., "postgresql://user:pass@host:port/db"
    
    // Custom API endpoints (for testing/staging)
    const std::string CUSTOM_API_URL = "";                  // Override default API URL
    
    // Production-specific settings
    const std::string PROD_LOG_ENDPOINT = "";               // External logging service
    const std::string MONITORING_API_KEY = "";              // For metrics/alerts
}

} // namespace secrets
} // namespace coinbase_dtc_core

#endif // COINBASE_API_SECRETS_H