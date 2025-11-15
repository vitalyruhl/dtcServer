#pragma once
#include "coinbase_dtc_core/core/dtc/protocol.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace dtc_test_client
{

    struct SymbolInfo
    {
        std::string symbol;
        std::string display_name;
        double price = 0.0;
        double volume = 0.0;
        bool active = false;
    };

    struct AccountInfo
    {
        std::string account_id;
        double balance = 0.0;
        std::string currency;
        bool verified = false;
    };

    struct MarketDepth
    {
        std::vector<std::pair<double, double>> bids; // price, size
        std::vector<std::pair<double, double>> asks; // price, size
        std::string symbol;
        std::string timestamp;
    };

    class DTCTestClient
    {
    public:
        DTCTestClient();
        ~DTCTestClient();

        // Connection management
        bool connect(const std::string &host, int port);
        void disconnect();
        bool is_connected() const;

        // Account operations
        bool get_account_info();
        const AccountInfo &get_current_account() const { return account_info_; }

        // Symbol operations
        bool get_available_symbols();
        const std::vector<SymbolInfo> &get_symbols() const { return symbols_; }
        bool get_symbol_info(const std::string &symbol);
        bool get_symbol_last_trade(const std::string &symbol);
        bool get_symbol_depth(const std::string &symbol);

        // Subscription management
        bool subscribe_to_symbol(const std::string &symbol);
        bool unsubscribe_from_symbol(const std::string &symbol);

        // Data callbacks
        void set_account_callback(std::function<void(const AccountInfo &)> callback);
        void set_symbol_callback(std::function<void(const SymbolInfo &)> callback);
        void set_depth_callback(std::function<void(const MarketDepth &)> callback);
        void set_trade_callback(std::function<void(const std::string &, double, double)> callback);
        void set_status_callback(std::function<void(const std::string &)> callback);

        // UI helpers
        void log_message(const std::string &message);
        std::string get_connection_status() const;

    private:
        // DTC protocol handling
        bool send_logon_request();
        bool send_market_data_request(const std::string &symbol);
        bool send_account_balance_request();

        void process_incoming_messages();
        void handle_logon_response(const open_dtc_server::core::dtc::LogonResponse &response);
        void handle_market_data_update(const open_dtc_server::core::dtc::MarketDataUpdateTrade &update);

        // Network
        std::string host_;
        int port_;
        bool connected_;

        // Data storage
        AccountInfo account_info_;
        std::vector<SymbolInfo> symbols_;
        std::vector<std::string> subscribed_symbols_;

        // Callbacks
        std::function<void(const AccountInfo &)> account_callback_;
        std::function<void(const SymbolInfo &)> symbol_callback_;
        std::function<void(const MarketDepth &)> depth_callback_;
        std::function<void(const std::string &, double, double)> trade_callback_;
        std::function<void(const std::string &)> status_callback_;

        // Internal state
        std::vector<std::string> log_messages_;
        uint32_t next_request_id_;
    };

} // namespace dtc_test_client