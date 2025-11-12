#include "coinbase_dtc_core/server/server.hpp"
#include "coinbase_dtc_core/util/log.hpp"
#include "coinbase_dtc_core/dtc/protocol.hpp"
#include "coinbase_dtc_core/feed/coinbase/feed.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>

// Global server instance for signal handling
coinbase_dtc_core::server::Server* g_server = nullptr;

void signal_handler(int signal) {
    if (g_server) {
        coinbase_dtc_core::util::log("Received shutdown signal, stopping server...");
        g_server->stop();
    }
}

int main() {
    using namespace coinbase_dtc_core;
    
    util::log("[START] CoinbaseDTC Server Starting...");
    
    // Setup signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        // Create server configuration
        server::ServerConfig config;
        config.port = 11099;                    // DTC default port
        config.max_clients = 50;                // Allow 50 concurrent clients
        config.enable_logging = true;
        config.heartbeat_interval = 30;
        
        // Create and configure server
        server::Server srv(config);
        g_server = &srv;
        
        // Set up connection handlers
        srv.set_connection_handler([](std::shared_ptr<server::ClientSession> client) {
            util::log("[CONN] New client connected: " + client->get_remote_address());
        });
        
        srv.set_disconnection_handler([](std::shared_ptr<server::ClientSession> client) {
            util::log("[DISC] Client disconnected: " + client->get_username() + " (" + client->get_remote_address() + ")");
        });
        
        // Start the DTC server
        if (!srv.start()) {
            util::log("[ERROR] Failed to start DTC server");
            return 1;
        }
        
        util::log("[OK] DTC Server started successfully!");
        std::cout << srv.status() << std::endl;
        
        // Initialize market data simulation
        util::log("[MARKET] Starting market data simulation...");
        
        // Create Coinbase feed (for future use)
        feed::coinbase::Feed feed;
        if (feed.connect()) {
            util::log("[OK] Connected to Coinbase feed");
        }
        
        // Main server loop with market data simulation
        uint64_t last_trade_time = 0;
        uint64_t last_bid_ask_time = 0;
        double base_price = 65000.0; // BTC base price
        
        while (srv.is_running()) {
            uint64_t current_time = server::Server::get_current_timestamp();
            
            // Simulate trade updates every 5 seconds
            if (current_time - last_trade_time > 5000000) { // 5 seconds in microseconds
                double price_variance = (rand() % 1000 - 500) / 10.0; // ±50 price variance
                double trade_price = base_price + price_variance;
                double volume = (rand() % 100) / 1000.0 + 0.01; // 0.01 to 0.11 BTC
                
                srv.broadcast_trade_update(1, trade_price, volume, current_time);
                util::log("[TRADE] Trade broadcast: BTC-USD $" + std::to_string(trade_price) + 
                         " vol:" + std::to_string(volume));
                
                last_trade_time = current_time;
            }
            
            // Simulate bid/ask updates every 2 seconds
            if (current_time - last_bid_ask_time > 2000000) { // 2 seconds in microseconds
                double spread = (rand() % 20 + 5) / 10.0; // $0.5 to $2.5 spread
                double bid_price = base_price - spread / 2;
                double ask_price = base_price + spread / 2;
                double bid_qty = (rand() % 50 + 10) / 10.0;   // 1.0 to 6.0 BTC
                double ask_qty = (rand() % 50 + 10) / 10.0;
                
                srv.broadcast_bid_ask_update(1, bid_price, bid_qty, ask_price, ask_qty, current_time);
                util::log("[BOOK] OrderBook broadcast: Bid $" + std::to_string(bid_price) + 
                         " x " + std::to_string(bid_qty) +
                         " | Ask $" + std::to_string(ask_price) + 
                         " x " + std::to_string(ask_qty));
                
                last_bid_ask_time = current_time;
                
                // Adjust base price slightly for next iteration
                base_price += (rand() % 200 - 100) / 100.0; // ±$1 drift
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        util::log("[STOP] Server main loop ended");
        
    } catch (const std::exception& e) {
        util::log("[ERROR] Server error: " + std::string(e.what()));
        return 1;
    }
    
    util::log("[OK] CoinbaseDTC Server shutdown complete");
    return 0;
}
