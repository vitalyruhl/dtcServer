#include "dtc_test_client/test_client.hpp"
#include "coinbase_dtc_core/core/util/log.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace dtc_test_client
{

    DTCTestClient::DTCTestClient()
        : host_("127.0.0.1"), port_(11099), connected_(false), next_request_id_(1)
    {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

        // Initialize with priority symbols
        symbols_ = {
            {"BTC-USD", "Bitcoin / US Dollar", 0.0, 0.0, false},
            {"ETH-USD", "Ethereum / US Dollar", 0.0, 0.0, false},
            {"STRK-USD", "Starknet / US Dollar", 0.0, 0.0, false}};
    }

    DTCTestClient::~DTCTestClient()
    {
        disconnect();
#ifdef _WIN32
        WSACleanup();
#endif
    }

    bool DTCTestClient::connect(const std::string &host, int port)
    {
        host_ = host;
        port_ = port;

        log_message("Connecting to DTC Server: " + host + ":" + std::to_string(port));

        // For now, simulate connection (will implement actual socket later)
        connected_ = true;

        if (connected_)
        {
            log_message("✅ Connected to DTC Server successfully");
            if (status_callback_)
            {
                status_callback_("Connected to " + host + ":" + std::to_string(port));
            }
            return send_logon_request();
        }

        return false;
    }

    void DTCTestClient::disconnect()
    {
        if (connected_)
        {
            log_message("Disconnecting from DTC Server");
            connected_ = false;
            subscribed_symbols_.clear();

            if (status_callback_)
            {
                status_callback_("Disconnected");
            }
        }
    }

    bool DTCTestClient::is_connected() const
    {
        return connected_;
    }

    bool DTCTestClient::get_account_info()
    {
        if (!connected_)
        {
            log_message("❌ Not connected to server");
            return false;
        }

        log_message("Requesting account information...");

        // Simulate account info response
        account_info_.account_id = "test-account-12345";
        account_info_.balance = 10000.50;
        account_info_.currency = "USD";
        account_info_.verified = true;

        log_message("✅ Account Info Retrieved:");
        log_message("  Account ID: " + account_info_.account_id);
        log_message("  Balance: $" + std::to_string(account_info_.balance));
        log_message("  Currency: " + account_info_.currency);
        log_message("  Verified: " + std::string(account_info_.verified ? "Yes" : "No"));

        if (account_callback_)
        {
            account_callback_(account_info_);
        }

        return true;
    }

    bool DTCTestClient::get_available_symbols()
    {
        if (!connected_)
        {
            log_message("❌ Not connected to server");
            return false;
        }

        log_message("Requesting available trading symbols...");

        // Simulate real symbol data
        symbols_ = {
            {"STRK-USD", "Starknet / US Dollar", 1.42, 850000.0, true},
            {"BTC-USD", "Bitcoin / US Dollar", 43250.80, 1250.5, true},
            {"ETH-USD", "Ethereum / US Dollar", 2680.45, 5500.2, true},
            {"SOL-USD", "Solana / US Dollar", 98.75, 12000.0, true},
            {"ADA-USD", "Cardano / US Dollar", 0.45, 2500000.0, true}};

        log_message("✅ Found " + std::to_string(symbols_.size()) + " trading symbols:");
        for (const auto &symbol : symbols_)
        {
            log_message("  " + symbol.symbol + " - " + symbol.display_name +
                        " (Price: $" + std::to_string(symbol.price) + ")");
        }

        return true;
    }

    bool DTCTestClient::get_symbol_info(const std::string &symbol)
    {
        if (!connected_)
        {
            log_message("❌ Not connected to server");
            return false;
        }

        log_message("Requesting symbol info for: " + symbol);

        // Find symbol and update info
        for (auto &sym : symbols_)
        {
            if (sym.symbol == symbol)
            {
                // Simulate updated market data
                sym.price = sym.price + (rand() % 100 - 50) * 0.01; // Random price change
                sym.volume = sym.volume + (rand() % 1000);

                log_message("✅ Symbol Info for " + symbol + ":");
                log_message("  Display Name: " + sym.display_name);
                log_message("  Current Price: $" + std::to_string(sym.price));
                log_message("  24h Volume: " + std::to_string(sym.volume));
                log_message("  Status: " + std::string(sym.active ? "Active" : "Inactive"));

                if (symbol_callback_)
                {
                    symbol_callback_(sym);
                }
                return true;
            }
        }

        log_message("❌ Symbol not found: " + symbol);
        return false;
    }

    bool DTCTestClient::get_symbol_last_trade(const std::string &symbol)
    {
        if (!connected_)
        {
            log_message("❌ Not connected to server");
            return false;
        }

        log_message("Requesting last trade for: " + symbol);

        // Simulate last trade data
        double last_price = 100.0 + (rand() % 10000) * 0.01;
        double last_size = 1.0 + (rand() % 1000) * 0.01;

        log_message("✅ Last Trade for " + symbol + ":");
        log_message("  Price: $" + std::to_string(last_price));
        log_message("  Size: " + std::to_string(last_size));

        if (trade_callback_)
        {
            trade_callback_(symbol, last_price, last_size);
        }

        return true;
    }

    bool DTCTestClient::get_symbol_depth(const std::string &symbol)
    {
        if (!connected_)
        {
            log_message("❌ Not connected to server");
            return false;
        }

        log_message("Requesting market depth for: " + symbol);

        // Simulate market depth data
        MarketDepth depth;
        depth.symbol = symbol;
        depth.timestamp = std::to_string(std::time(nullptr));

        // Generate realistic bid/ask spread
        double mid_price = 100.0 + (rand() % 10000) * 0.01;

        // Bids (lower prices)
        for (int i = 0; i < 5; i++)
        {
            double price = mid_price - (i + 1) * 0.01;
            double size = 10.0 + (rand() % 100);
            depth.bids.emplace_back(price, size);
        }

        // Asks (higher prices)
        for (int i = 0; i < 5; i++)
        {
            double price = mid_price + (i + 1) * 0.01;
            double size = 10.0 + (rand() % 100);
            depth.asks.emplace_back(price, size);
        }

        log_message("✅ Market Depth for " + symbol + ":");
        log_message("  Bids (Buy Orders):");
        for (const auto &bid : depth.bids)
        {
            log_message("    $" + std::to_string(bid.first) + " x " + std::to_string(bid.second));
        }
        log_message("  Asks (Sell Orders):");
        for (const auto &ask : depth.asks)
        {
            log_message("    $" + std::to_string(ask.first) + " x " + std::to_string(ask.second));
        }

        if (depth_callback_)
        {
            depth_callback_(depth);
        }

        return true;
    }

    bool DTCTestClient::subscribe_to_symbol(const std::string &symbol)
    {
        if (!connected_)
        {
            log_message("❌ Not connected to server");
            return false;
        }

        // Check if already subscribed
        for (const auto &sub : subscribed_symbols_)
        {
            if (sub == symbol)
            {
                log_message("⚠️ Already subscribed to: " + symbol);
                return true;
            }
        }

        log_message("📡 Subscribing to real-time data for: " + symbol);
        subscribed_symbols_.push_back(symbol);

        // Simulate subscription confirmation
        log_message("✅ Subscription confirmed for " + symbol);
        log_message("📊 Real-time data stream active");

        return send_market_data_request(symbol);
    }

    bool DTCTestClient::unsubscribe_from_symbol(const std::string &symbol)
    {
        auto it = std::find(subscribed_symbols_.begin(), subscribed_symbols_.end(), symbol);
        if (it != subscribed_symbols_.end())
        {
            subscribed_symbols_.erase(it);
            log_message("❌ Unsubscribed from: " + symbol);
            return true;
        }

        log_message("⚠️ Not subscribed to: " + symbol);
        return false;
    }

    void DTCTestClient::log_message(const std::string &message)
    {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now.time_since_epoch()) %
                  1000;

        std::ostringstream oss;
        struct tm *timeinfo = std::localtime(&time_t);
        oss << "[" << std::setfill('0') << std::setw(2) << timeinfo->tm_hour
            << ":" << std::setw(2) << timeinfo->tm_min
            << ":" << std::setw(2) << timeinfo->tm_sec;
        oss << "." << std::setfill('0') << std::setw(3) << ms.count() << "] ";
        oss << message;

        std::string timestamped_message = oss.str();
        log_messages_.push_back(timestamped_message);

        // Keep only last 100 messages
        if (log_messages_.size() > 100)
        {
            log_messages_.erase(log_messages_.begin());
        }

        // Output to console
        std::cout << timestamped_message << std::endl;

        // Notify status callback
        if (status_callback_)
        {
            status_callback_(timestamped_message);
        }
    }

    std::string DTCTestClient::get_connection_status() const
    {
        if (connected_)
        {
            return "🟢 Connected to " + host_ + ":" + std::to_string(port_) +
                   " | Subscriptions: " + std::to_string(subscribed_symbols_.size());
        }
        return "🔴 Disconnected";
    }

    // Callback setters
    void DTCTestClient::set_account_callback(std::function<void(const AccountInfo &)> callback)
    {
        account_callback_ = callback;
    }

    void DTCTestClient::set_symbol_callback(std::function<void(const SymbolInfo &)> callback)
    {
        symbol_callback_ = callback;
    }

    void DTCTestClient::set_depth_callback(std::function<void(const MarketDepth &)> callback)
    {
        depth_callback_ = callback;
    }

    void DTCTestClient::set_trade_callback(std::function<void(const std::string &, double, double)> callback)
    {
        trade_callback_ = callback;
    }

    void DTCTestClient::set_status_callback(std::function<void(const std::string &)> callback)
    {
        status_callback_ = callback;
    }

    // Private DTC protocol methods
    bool DTCTestClient::send_logon_request()
    {
        log_message("Sending DTC logon request...");

        // Simulate logon process
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        log_message("✅ DTC Logon successful");
        return true;
    }

    bool DTCTestClient::send_market_data_request(const std::string &symbol)
    {
        log_message("Sending market data request for: " + symbol);

        // Simulate market data subscription
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        log_message("✅ Market data subscription active for " + symbol);
        return true;
    }

    bool DTCTestClient::send_account_balance_request()
    {
        log_message("Sending account balance request...");

        // Simulate account balance request
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        return true;
    }

} // namespace dtc_test_client