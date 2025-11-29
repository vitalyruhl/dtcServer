#include "coinbase_dtc_core/core/server/server.hpp"
#include "coinbase_dtc_core/core/util/advanced_log.hpp"
#include "coinbase_dtc_core/core/auth/jwt_auth.hpp"
#include "coinbase_dtc_core/exchanges/base/exchange_feed.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>

// Global server instance for signal handling
coinbase_dtc_core::core::server::DTCServer *g_server = nullptr;

static std::string extract_api_key_short(const std::string &key)
{
    const auto pos = key.find_last_of('/');
    if (pos != std::string::npos && pos + 1 < key.size())
    {
        return key.substr(pos + 1);
    }
    return key;
}

static std::string redact_key_id(const std::string &key)
{
    if (key.size() <= 8)
        return key;
    return key.substr(0, 4) + "..." + key.substr(key.size() - 4);
}

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
    std::string credentials_path = "config/cdp_api_key_ECDSA.json"; // Default path
    std::string log_level = "advanced";                             // Default log level
    std::string log_config = "config/logging.ini";                  // Default config path

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
        logger.setLogProfile(open_dtc_server::util::LogProfile::PROFILE_STD);
    }
    else if (log_level == "advanced")
    {
        logger.setLogProfile(open_dtc_server::util::LogProfile::PROFILE_ADVANCED);
    }
    else if (log_level == "verbose")
    {
        logger.setLogProfile(open_dtc_server::util::LogProfile::PROFILE_VERBOSE);
    }
    else
    {
        LOG_WARN("Unknown log level '" + log_level + "', using 'advanced'");
        logger.setLogProfile(open_dtc_server::util::LogProfile::PROFILE_ADVANCED);
    }

    LOG_INFO("=== DTC SERVER STARTUP ===");
    LOG_INFO("Credentials file: " + credentials_path);
    LOG_INFO("Log level: " + log_level);
    LOG_INFO("Log config: " + log_config);
    LOG_TRACE("[DEBUG] Logger initialization completed");

    if (credentials_path != "cdp_api_key.json")
    {
        LOG_INFO("[CONFIG] Using credentials file: " + credentials_path);
    }

    LOG_TRACE("[DEBUG] Setting up signal handling...");
    // Setup signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    LOG_TRACE("[DEBUG] Signal handlers configured");

    try
    {
        LOG_TRACE("[DEBUG] Creating server configuration...");
        // Create server configuration
        ServerConfig config;
        config.bind_address = "0.0.0.0"; // Allow connections from any interface
        config.port = 11099;
        config.server_name = "CoinbaseDTCServer";
        config.password = "";
        config.require_authentication = false;
        config.credentials_file_path = credentials_path; // Set the credentials path
        LOG_TRACE("[DEBUG] Server config created");

        LOG_TRACE("[DEBUG] Creating DTCServer instance...");
        // Create server instance
        DTCServer srv(config);
        g_server = &srv;
        LOG_TRACE("[DEBUG] DTCServer instance created successfully");

        LOG_TRACE("[DEBUG] Configuring Coinbase exchange...");
        // Load CDP credentials from JSON file
        LOG_INFO("[CONFIG] Loading CDP credentials from: " + credentials_path);
        open_dtc_server::auth::CDPCredentials credentials;
        try
        {
            credentials = open_dtc_server::auth::CDPCredentials::from_json_file(credentials_path);
            LOG_INFO("[CONFIG] Parsed CDP key id: " + redact_key_id(credentials.key_id));
            LOG_INFO("[CONFIG] Private key length: " + std::to_string(credentials.private_key.size()));
        }
        catch (const std::exception &ex)
        {
            LOG_WARN(std::string("[WARNING] Failed to parse CDP credentials: ") + ex.what());
        }

        if (!credentials.is_valid())
        {
            LOG_TRACE("[DEBUG] Attempting to load CDP credentials from environment variables");
            credentials = open_dtc_server::auth::CDPCredentials::from_environment();
            if (credentials.is_valid())
            {
                LOG_INFO("[CONFIG] Loaded CDP credentials from environment, key id: " + redact_key_id(credentials.key_id));
            }
        }

        bool has_valid_credentials = credentials.is_valid();
        if (has_valid_credentials)
        {
            LOG_INFO("[SUCCESS] CDP credentials loaded successfully");
        }
        else
        {
            LOG_WARN("[WARNING] No valid CDP credentials found - using public data only");
        }

        // Add Coinbase exchange for real market data
        open_dtc_server::exchanges::base::ExchangeConfig coinbase_config;
        coinbase_config.name = "coinbase"; // Must match factory name
        coinbase_config.websocket_url = "wss://ws-feed.exchange.coinbase.com";
        coinbase_config.api_url = "https://api.exchange.coinbase.com";
        coinbase_config.port = 443;
        coinbase_config.requires_auth = has_valid_credentials; // Enable auth if we have credentials

        // Set credentials in config if available
        if (has_valid_credentials)
        {
            coinbase_config.api_key = extract_api_key_short(credentials.key_id);
            coinbase_config.secret_key = credentials.private_key;
            coinbase_config.passphrase = credentials.passphrase;
            LOG_INFO("[CONFIG] Coinbase exchange configured with authentication (key id " + redact_key_id(credentials.key_id) + ")");
        }
        else
        {
            LOG_INFO("[CONFIG] Coinbase exchange configured for public data only");
        }
        LOG_TRACE("[DEBUG] Coinbase config prepared");

        LOG_TRACE("[DEBUG] Adding Coinbase exchange to server...");
        if (!srv.add_exchange(coinbase_config))
        {
            LOG_WARN("Warning: Failed to add Coinbase exchange - continuing with mock data");
        }
        else
        {
            LOG_INFO("[SUCCESS] Added Coinbase exchange for real market data");

            LOG_TRACE("[DEBUG] Subscribing to BTC-USD...");
            // Subscribe to specific symbols for testing
            srv.subscribe_symbol("BTC-USD", "coinbase");
            LOG_TRACE("[DEBUG] Symbol subscription completed");
            // Removed ETH-USD and SOL-USD to keep logs clean for testing
        }
        LOG_INFO("Server configured, starting...");

        if (!srv.start())
        {
            LOG_ERROR("Failed to start server");
            return 1;
        }

        LOG_INFO("DTC Server started successfully");
        LOG_TRACE("Reading server status...");
        LOG_INFO("Server status: " + srv.get_status());

        LOG_TRACE("Entering main server loop...");
        // Keep server running
        while (srv.is_running())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        LOG_INFO("CoinbaseDTC Server shutdown complete");
        LOG_TRACE("Exiting main function normally");
        return 0;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Server error: " + std::string(e.what()));
        LOG_TRACE("Exception caught in main function");
        return 1;
    }
}
