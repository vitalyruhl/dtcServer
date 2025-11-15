#include "coinbase_dtc_core/exchanges/coinbase/ssl_websocket_client.hpp"
#include "coinbase_dtc_core/core/util/log.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace open_dtc_server::feed::coinbase;

int main()
{
    std::cout << "=== SSL WebSocket Client Test ===" << std::endl;

    // Create SSL WebSocket client
    SSLWebSocketClient client;

    // Set up message callback
    client.set_message_callback([](const std::string &message)
                                { std::cout << "[RECEIVED] " << message.substr(0, 100) << "..." << std::endl; });

    // Set up connection callback
    client.set_connection_callback([](bool connected)
                                   {
        if (connected) {
            std::cout << "[STATUS] Connected to Coinbase Advanced Trade" << std::endl;
        } else {
            std::cout << "[STATUS] ❌ Disconnected from Coinbase" << std::endl;
        } });

    // Try to connect
    std::cout << "[TEST] Attempting SSL WebSocket connection..." << std::endl;
    bool connected = client.connect("ws-feed.exchange.coinbase.com", 443);

    if (connected)
    {
        std::cout << "[SUCCESS] SSL WebSocket connection established!" << std::endl;

        // Try JWT authentication
        std::cout << "[TEST] Attempting JWT authentication..." << std::endl;
        if (client.authenticate_with_jwt())
        {
            std::cout << "[SUCCESS] JWT authentication successful!" << std::endl;

            // Subscribe to Bitcoin ticker
            std::cout << "[TEST] Subscribing to BTC-USD ticker..." << std::endl;
            client.subscribe_to_ticker({"BTC-USD"});

            // Keep alive for 10 seconds
            std::cout << "[WAIT] Listening for 10 seconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
        else
        {
            std::cout << "[ERROR] JWT authentication failed" << std::endl;
        }

        client.disconnect();
    }
    else
    {
        std::cout << "[ERROR] SSL WebSocket connection failed" << std::endl;
    }

    std::cout << "[TEST] SSL WebSocket test completed" << std::endl;
    return 0;
}