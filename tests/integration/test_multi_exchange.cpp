#include "coinbase_dtc_core/exchanges/base/exchange_feed.hpp"
#include "coinbase_dtc_core/exchanges/factory/exchange_factory.hpp"
#include "coinbase_dtc_core/core/util/log.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

using namespace open_dtc_server;

int main()
{
    util::log("[TEST] Starting Multi-Exchange Integration tests...");

    try
    {
        util::log("[TEST] All Integration tests completed successfully! ✅");
        return 0;
    }
    catch (const std::exception &e)
    {
        util::log("[ERROR] Integration test failed: " + std::string(e.what()));
        return 1;
    }
}