#include "coinbase_dtc_core/core/server/server.hpp"
#include "coinbase_dtc_core/core/util/log.hpp"
#include "coinbase_dtc_core/exchanges/factory/exchange_factory.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace open_dtc_server;

int main()
{
    util::log("[TEST] Starting Server tests...");

    try
    {
        // Test 1: Server creation
        {
            coinbase_dtc_core::core::server::DTCServer dtc_server(11000);
            util::log("[TEST] ✅ Server created successfully");
        }

        util::log("[TEST] All Server tests completed successfully! ✅");
        return 0;
    }
    catch (const std::exception &e)
    {
        util::log("[ERROR] Server test failed: " + std::string(e.what()));
        return 1;
    }
}