#include "coinbase_dtc_core/exchanges/coinbase/websocket_client.hpp"
#include "coinbase_dtc_core/core/util/log.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main()
{
    std::cout << "=== SSL WebSocket Connection Test ===" << std::endl;

    auto trade_callback = [](const open_dtc_server::exchanges::base::MarketTrade &trade)
    {
        std::cout << "[TRADE] Symbol: " << trade.symbol
                  << ", Price: " << trade.price
                  << ", Volume: " << trade.volume
                  << ", Side: " << trade.side << std::endl;
    };

    auto level2_callback = [](const open_dtc_server::exchanges::base::MarketLevel2 &level2)
    {
        std::cout << "[L2] Symbol: " << level2.symbol
                  << ", Bid: " << level2.bid_price << "@" << level2.bid_size
                  << ", Ask: " << level2.ask_price << "@" << level2.ask_size << std::endl;
    };

    // Create WebSocket client
    open_dtc_server::feed::coinbase::WebSocketClient client;
    client.set_trade_callback(trade_callback);
    client.set_level2_callback(level2_callback);

    std::cout << "\nAttempting to connect to Coinbase WebSocket (SSL)..." << std::endl;

    // Connect with SSL
    if (client.connect())
    {
        std::cout << "✓ SSL Connection established successfully!" << std::endl;
        std::cout << "Status: " << client.get_status() << std::endl;

        // Subscribe to BTC-USD
        std::cout << "\nSubscribing to BTC-USD..." << std::endl;

        if (client.subscribe_trades("BTC-USD"))
        {
            std::cout << "✓ Subscribed to BTC-USD trades" << std::endl;
        }

        if (client.subscribe_level2("BTC-USD"))
        {
            std::cout << "✓ Subscribed to BTC-USD level2" << std::endl;
        } // Wait for some data
        std::cout << "\nWaiting for market data (10 seconds)..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));

        std::cout << "\nFinal status: " << client.get_status() << std::endl;

        // Disconnect
        std::cout << "\nDisconnecting..." << std::endl;
        client.disconnect();
        std::cout << "✓ Disconnected" << std::endl;
    }
    else
    {
        std::cout << "✗ Failed to establish SSL connection" << std::endl;
        std::cout << "Status: " << client.get_status() << std::endl;
        return 1;
    }

    std::cout << "\n=== SSL WebSocket Test Complete ===" << std::endl;
    return 0;
}