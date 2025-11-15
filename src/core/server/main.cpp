#include "coinbase_dtc_core/core/server/server.hpp"
#include "coinbase_dtc_core/core/util/log.hpp"
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
        open_dtc_server::util::log("Received shutdown signal, stopping server...");
        g_server->stop();
    }
}

int main()
{
    using namespace coinbase_dtc_core::core::server;

    open_dtc_server::util::log("[START] CoinbaseDTC Server Starting...");

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

        // Create server instance
        DTCServer srv(config);
        g_server = &srv;

        open_dtc_server::util::log("Server configured, starting...");

        if (!srv.start())
        {
            open_dtc_server::util::log("Failed to start server");
            return 1;
        }

        open_dtc_server::util::log("DTC Server started successfully");
        open_dtc_server::util::log("Server status: " + srv.get_status());

        // Keep server running
        while (srv.is_running())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        open_dtc_server::util::log("CoinbaseDTC Server shutdown complete");
        return 0;
    }
    catch (const std::exception &e)
    {
        open_dtc_server::util::log("Server error: " + std::string(e.what()));
        return 1;
    }
}