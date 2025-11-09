#pragma once

#ifndef COINBASE_CREDENTIALS_H
#define COINBASE_CREDENTIALS_H

#include <string>
#include <cstdlib>

namespace coinbase_dtc_core {
namespace credentials {

/**
 * Coinbase API Credentials Manager
 * 
 * This class handles loading credentials from multiple sources:
 * 1. Environment variables (for CI/CD and production)
 * 2. CDP JSON file (new format: cdp_api_key.json)
 * 3. Local secrets file (legacy format)
 * 4. Default empty values (for public API testing)
 */

class CredentialsManager {
public:
    // Legacy API Key methods (for backward compatibility)
    static std::string getApiKey() {
        // Try environment variable first
        if (const char* env_key = std::getenv("COINBASE_API_KEY")) {
            return std::string(env_key);
        }
        
        // Try loading from secrets file (if it exists)
        #ifdef INCLUDE_SECRETS_FILE
            #include "../secrets/coinbase.h"
            return secrets::COINBASE_API_KEY;
        #endif
        
        // Default: empty string (public API mode)
        return "";
    }
    
    static std::string getApiSecret() {
        if (const char* env_secret = std::getenv("COINBASE_API_SECRET")) {
            return std::string(env_secret);
        }
        
        #ifdef INCLUDE_SECRETS_FILE
            #include "../secrets/coinbase.h"
            return secrets::COINBASE_API_SECRET;
        #endif
        
        return "";
    }
    
    static std::string getPassphrase() {
        if (const char* env_passphrase = std::getenv("COINBASE_PASSPHRASE")) {
            return std::string(env_passphrase);
        }
        
        #ifdef INCLUDE_SECRETS_FILE
            #include "../secrets/coinbase.h"
            return secrets::COINBASE_PASSPHRASE;
        #endif
        
        return "";
    }
    
    // CDP (New format) methods - will be implemented when CDP integration is ready
    static std::string getCDPApiKeyId() {
        // Try environment variable first
        if (const char* env_id = std::getenv("CDP_API_KEY_ID")) {
            return std::string(env_id);
        }
        
        // TODO: Implement CDP JSON file loading
        // For now, return empty
        return "";
    }
    
    static std::string getCDPPrivateKey() {
        // Try environment variable first
        if (const char* env_key = std::getenv("CDP_PRIVATE_KEY")) {
            return std::string(env_key);
        }
        
        // TODO: Implement CDP JSON file loading
        // For now, return empty
        return "";
    }
    
    // Check if we have valid credentials (any format)
    static bool hasCredentials() {
        return hasLegacyCredentials() || hasCDPCredentials();
    }
    
    static bool hasLegacyCredentials() {
        return !getApiKey().empty() && !getApiSecret().empty();
    }
    
    static bool hasCDPCredentials() {
        return !getCDPApiKeyId().empty() && !getCDPPrivateKey().empty();
    }
    
    // Get preferred authentication type
    static std::string getAuthType() {
        if (hasCDPCredentials()) {
            return "CDP";  // Newer format
        } else if (hasLegacyCredentials()) {
            return "Legacy";
        } else {
            return "Public";
        }
    }
    
    // Get custom API URL override
    static std::string getCustomApiUrl() {
        if (const char* env_url = std::getenv("COINBASE_API_URL")) {
            return std::string(env_url);
        }
        
        #ifdef INCLUDE_SECRETS_FILE
            #include "../secrets/coinbase.h"
            return secrets::env_overrides::CUSTOM_API_URL;
        #endif
        
        return "";
    }
};

} // namespace credentials
} // namespace coinbase_dtc_core

#endif // COINBASE_CREDENTIALS_H