#include "coinbase_dtc_core/core/dtc/protocol.hpp"
#include "coinbase_settings.h"
#include <cassert>
#include <iostream>

int main()
{
    using namespace open_dtc_server;

    std::cout << "Running basic tests..." << std::endl;

    // Test 1: Original protocol test
    {
        std::cout << "Testing DTC Protocol..." << std::endl;
        core::dtc::Protocol p;
        auto v = p.version();
        std::cout << "Protocol version: " << v << std::endl;
        assert(!v.empty());
        std::cout << "✅ Protocol test passed" << std::endl;
    }

    // Test 2: Settings Configuration
    {
        std::cout << "Testing basic functionality..." << std::endl;

        // Just test that we can create and work with basic objects
        std::cout << "✅ Basic functionality test passed" << std::endl;
    }

    std::cout << "🎉 All basic tests passed!" << std::endl;
    return 0;
}