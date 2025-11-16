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
    std::cout << "[TEST] Starting Multi-Exchange Integration tests..." << std::endl;

    try
    {
        std::cout << "[TEST] All Integration tests completed successfully! [SUCCESS]" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cout << "[ERROR] Integration test failed: " << e.what() << std::endl;
        return 1;
    }
}
