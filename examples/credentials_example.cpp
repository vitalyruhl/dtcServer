#include "coinbase_dtc_core/credentials/credentials_manager.hpp"
#include "settings/coinbase_settings.h"
#include "coinbase_dtc_core/test/test_config.hpp"
#include <iostream>

int main() {
    using namespace coinbase_dtc_core;
    
    // Get credentials (works with or without secrets file)
    std::string api_key = credentials::CredentialsManager::getApiKey();
    std::string api_secret = credentials::CredentialsManager::getApiSecret();
    
    // Determine which mode we're in
    bool has_credentials = credentials::CredentialsManager::hasCredentials();
    
    if (has_credentials) {
        std::cout << "🔐 Using authenticated API mode" << std::endl;
        std::cout << "API Key: " << (api_key.empty() ? "Not set" : "***hidden***") << std::endl;
    } else {
        std::cout << "🌐 Using public API mode (no authentication)" << std::endl;
    }
    
    // Get API URL (with potential override)
    std::string api_url = settings::api::PUBLIC_API_URL;
    std::string custom_url = credentials::CredentialsManager::getCustomApiUrl();
    if (!custom_url.empty()) {
        api_url = custom_url;
        std::cout << "🔧 Using custom API URL: " << api_url << std::endl;
    } else {
        std::cout << "🌐 Using default API URL: " << api_url << std::endl;
    }
    
    // Show configuration
    std::cout << "📊 Default product: " << settings::products::DEFAULT_PRODUCT << std::endl;
    std::cout << "⚡ Rate limit: " << settings::rate_limits::PUBLIC_REQUESTS_PER_SECOND << " req/sec" << std::endl;
    std::cout << "⏱️ Timeout: " << settings::rate_limits::REQUEST_TIMEOUT_SECONDS << " seconds" << std::endl;
    
    return 0;
}