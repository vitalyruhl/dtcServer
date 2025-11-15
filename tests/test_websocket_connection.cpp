#include "include/coinbase_dtc_core/exchanges/coinbase/coinbase_feed.hpp"
#include "include/coinbase_dtc_core/exchanges/base/exchange_feed.hpp"
#include "include/coinbase_dtc_core/core/util/log.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace open_dtc_server;

int main()
{
    // Initialize logging
    util::log("Starting WebSocket connection test");

    // Create exchange config
    exchanges::base::ExchangeConfig config;

    // Create coinbase feed instance
    exchanges::coinbase::CoinbaseFeed feed(config);

    // Subscribe to BTC-USD (this should now work since we're connected)
    std::cout << "[INFO] Subscribing to BTC-USD trades and level2..." << std::endl;
    feed.subscribe_trades("BTC-USD");
    feed.subscribe_level2("BTC-USD");

    std::cout << "[INFO] Connecting to Coinbase WebSocket..." << std::endl;

    // Attempt to connect
    if (feed.connect())
    {
        std::cout << "[SUCCESS] Connected to Coinbase WebSocket!" << std::endl;

        // Subscribe after connection
        std::cout << "[INFO] Now subscribing to BTC-USD trades and level2..." << std::endl;
        feed.subscribe_trades("BTC-USD");
        feed.subscribe_level2("BTC-USD");

        // Wait for some data
        std::cout << "[INFO] Waiting for market data..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));

        // Disconnect
        feed.disconnect();
        std::cout << "[INFO] Disconnected from Coinbase WebSocket" << std::endl;
    }
    else
    {
        std::cout << "[ERROR] Failed to connect to Coinbase WebSocket" << std::endl;
        return 1;
    }

    return 0;
}