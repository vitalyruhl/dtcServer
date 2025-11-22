#include "coinbase_dtc_core/core/server/server.hpp"
#include "coinbase_dtc_core/core/util/log.hpp"
#include "coinbase_dtc_core/core/util/advanced_log.hpp"
#include "coinbase_dtc_core/exchanges/base/exchange_feed.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>

// Global server instance for signal handling
coinbase_dtc_core::core::server::DTCServer *g_server = nullptr;

void signal_handler(int signal)
{
    if (g_server)
    {
        LOG_INFO("Received shutdown signal, stopping server...");
        g_server->stop();
    }
}

int main(int argc, char *argv[])
{
    using namespace coinbase_dtc_core::core::server;

    // Parse command line arguments
    std::string credentials_path = "cdp_api_key.json"; // Default path
    std::string log_level = "advanced";                // Default log level
    std::string log_config = "config/logging.ini";     // Default config path

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "--credentials" && i + 1 < argc)
        {
            credentials_path = argv[i + 1];
            i++; // Skip next argument as it's the path
        }
        else if (arg == "--loglevel" && i + 1 < argc)
        {
            log_level = argv[i + 1];
            i++; // Skip next argument as it's the level
        }
        else if (arg == "--logconfig" && i + 1 < argc)
        {
            log_config = argv[i + 1];
            i++; // Skip next argument as it's the config path
        }
        else if (arg == "--help" || arg == "-h")
        {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --credentials <path>     Path to CDP API credentials file\n";
            std::cout << "  --loglevel <level>       Log level: std, advanced, verbose (default: advanced)\n";
            std::cout << "  --logconfig <path>       Path to logging configuration file (default: config/logging.ini)\n";
            std::cout << "  --help, -h              Show this help message\n";
            std::cout << "\nLog Levels:\n";
            std::cout << "  std        - Only errors and critical messages\n";
            std::cout << "  advanced   - Info, warnings, errors (default)\n";
            std::cout << "  verbose    - Everything including debug and trace\n";
            return 0;
        }
    }

    // Initialize advanced logging system
    auto &logger = open_dtc_server::util::Logger::getInstance();
    if (!logger.initialize(log_config))
    {
        std::cerr << "Warning: Could not load logging config, using defaults" << std::endl;
    }

    // Set log level from command line
    if (log_level == "std")
    {
        logger.setLogProfile(open_dtc_server::util::LogProfile::STD);
    }
    else if (log_level == "advanced")
    {
        logger.setLogProfile(open_dtc_server::util::LogProfile::ADVANCED);
    }
    else if (log_level == "verbose")
    {
        logger.setLogProfile(open_dtc_server::util::LogProfile::VERBOSE);
    }
    else
    {
        LOG_WARNING("Unknown log level '" + log_level + "', using 'advanced'");
        logger.setLogProfile(open_dtc_server::util::LogProfile::ADVANCED);
    }

    LOG_INFO("=== DTC SERVER STARTUP ===");
    LOG_INFO("Credentials file: " + credentials_path);
    LOG_INFO("Log level: " + log_level);
    LOG_INFO("Log config: " + log_config);

    if (credentials_path != "cdp_api_key.json")
    {
        open_dtc_server::util::simple_log("[CONFIG] Using credentials file: " + credentials_path);
    }

    // Setup signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try
    {
        // Create server configuration
        ServerConfig config;
        config.bind_address = "0.0.0.0"; // Allow connections from any interface
        config.port = 11099;
        config.server_name = "CoinbaseDTCServer";
        config.password = "";
        config.require_authentication = false;
        config.credentials_file_path = credentials_path; // Set the credentials path

        // Create server instance
        DTCServer srv(config);
        g_server = &srv;

        // Add Coinbase exchange for real market data
        open_dtc_server::exchanges::base::ExchangeConfig coinbase_config;
        coinbase_config.name = "coinbase"; // Must match factory name
        coinbase_config.websocket_url = "wss://ws-feed.exchange.coinbase.com";
        coinbase_config.api_url = "https://api.exchange.coinbase.com";
        coinbase_config.port = 443;
        coinbase_config.requires_auth = false; // For public market data
        // Note: symbols will be configured separately through subscribe calls

        if (!srv.add_exchange(coinbase_config))
        {
            open_dtc_server::util::write_log("Warning: Failed to add Coinbase exchange - continuing with mock data");
        }
        else
        {
            open_dtc_server::util::write_log("[SUCCESS] Added Coinbase exchange for real market data");

            // Subscribe to specific symbols for testing
            srv.subscribe_symbol("BTC-USD", "coinbase");
            // Removed ETH-USD and SOL-USD to keep logs clean for testing
        }
        open_dtc_server::util::write_log("Server configured, starting...");

        if (!srv.start())
        {
            open_dtc_server::util::write_log("Failed to start server");
            return 1;
        }

        open_dtc_server::util::write_log("DTC Server started successfully");
        open_dtc_server::util::write_log("Server status: " + srv.get_status());

        // Keep server running
        while (srv.is_running())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        open_dtc_server::util::write_log("CoinbaseDTC Server shutdown complete");
        return 0;
    }
    catch (const std::exception &e)
    {
        open_dtc_server::util::write_log("Server error: " + std::string(e.what()));
        return 1;
    }
}
