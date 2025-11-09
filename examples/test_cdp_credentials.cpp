#include "coinbase_dtc_core/credentials/cdp_credentials.hpp"
#include "coinbase_dtc_core/credentials/credentials_manager.hpp"
#include "settings/coinbase_settings.h"
#include <iostream>

int main() {
    using namespace coinbase_dtc_core;
    
    std::cout << "=== CDP Credentials Test ===" << std::endl;
    
    // Test CDP credentials loading
    auto cdp_creds = credentials::CDPCredentialsManager::getCredentials();
    
    if (cdp_creds.isValid()) {
        std::cout << "✅ CDP Credentials found!" << std::endl;
        std::cout << "   API Key ID: " << cdp_creds.api_key_id.substr(0, 8) << "..." << std::endl;
        std::cout << "   Private Key: " << cdp_creds.private_key.substr(0, 10) << "..." << std::endl;
        
        // Test via credentials manager
        std::string key_id = credentials::CredentialsManager::getCDPApiKeyId();
        std::string private_key = credentials::CredentialsManager::getCDPPrivateKey();
        
        std::cout << "   Via Manager - Key ID: " << key_id.substr(0, 8) << "..." << std::endl;
        std::cout << "   Via Manager - Private Key: " << private_key.substr(0, 10) << "..." << std::endl;
        
        std::cout << "🔐 Authentication Type: " << credentials::CredentialsManager::getAuthType() << std::endl;
        
    } else {
        std::cout << "❌ No CDP credentials found" << std::endl;
        std::cout << "   Looking for: secrets/cdp_api_key.json" << std::endl;
        std::cout << "   Or environment variables: CDP_API_KEY_ID, CDP_PRIVATE_KEY" << std::endl;
    }
    
    // Show settings
    std::cout << "\n=== API Configuration ===" << std::endl;
    std::cout << "Public API URL: " << settings::api::PUBLIC_API_URL << std::endl;
    std::cout << "Advanced API URL: " << settings::api::ADVANCED_API_URL << std::endl;
    std::cout << "Rate Limit: " << settings::rate_limits::PUBLIC_REQUESTS_PER_SECOND << " req/sec" << std::endl;
    
    // Test credentials manager
    std::cout << "\n=== Credentials Summary ===" << std::endl;
    std::cout << "Has any credentials: " << (credentials::CredentialsManager::hasCredentials() ? "Yes" : "No") << std::endl;
    std::cout << "Has legacy credentials: " << (credentials::CredentialsManager::hasLegacyCredentials() ? "Yes" : "No") << std::endl;
    std::cout << "Has CDP credentials: " << (credentials::CredentialsManager::hasCDPCredentials() ? "Yes" : "No") << std::endl;
    std::cout << "Auth type: " << credentials::CredentialsManager::getAuthType() << std::endl;
    
    return 0;
}