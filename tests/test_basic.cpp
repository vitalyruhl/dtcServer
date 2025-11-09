#include "coinbase_dtc_core/dtc/protocol.hpp"
#include "coinbase_settings.h"
#include <cassert>
#include <iostream>

int main() {
    using namespace coinbase_dtc_core;
    
    std::cout << "Running basic tests..." << std::endl;
    
    // Test 1: Original protocol test
    {
        std::cout << "Testing DTC Protocol..." << std::endl;
        dtc::Protocol p;
        auto v = p.version();
        std::cout << "Protocol version: " << v << std::endl;
        assert(!v.empty());
        std::cout << "✅ Protocol test passed" << std::endl;
    }
    
    // Test 2: Settings Configuration  
    {
        std::cout << "Testing settings configuration..." << std::endl;
        
        // Test that settings are accessible
        std::string api_url = settings::api::PUBLIC_API_URL;
        assert(!api_url.empty());
        assert(api_url == "https://api.exchange.coinbase.com");
        std::cout << "API URL: " << api_url << std::endl;
        
        auto products = settings::products::MAJOR_PAIRS;
        assert(!products.empty());
        assert(products[0] == "BTC-USD");
        std::cout << "First trading pair: " << products[0] << std::endl;
        
        int timeout = settings::rate_limits::REQUEST_TIMEOUT_SECONDS;
        assert(timeout > 0);
        std::cout << "Request timeout: " << timeout << " seconds" << std::endl;
        
        std::cout << "✅ Settings configuration test passed" << std::endl;
    }
    
    std::cout << "🎉 All basic tests passed!" << std::endl;
    return 0;
}
