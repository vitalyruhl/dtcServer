#pragma once

#include "coinbase_dtc_core/exchanges/base/exchange_feed.hpp"
// Note: WebSocket dependencies temporarily commented out for compilation
// #include <websocketpp/config/asio_client.hpp>
// #include <websocketpp/client.hpp>
#include <thread>
#include <atomic>
#include <chrono>
#include <unordered_set>

namespace coinbase_dtc_core
{
    namespace exchanges
    {
        namespace binance
        {

            /**
             * Binance exchange feed implementation.
             *
             * This demonstrates how to implement a different exchange using the same
             * ExchangeFeedBase interface. Binance has different:
             * - Symbol format (BTCUSDT vs BTC-USD)
             * - WebSocket protocol
             * - Message structures
             * - API endpoints
             */
            class BinanceFeed : public base::ExchangeFeedBase
            {
            public:
                explicit BinanceFeed(const base::ExchangeConfig &config);
                ~BinanceFeed() override;

                // ========================================================================
                // REQUIRED VIRTUAL METHOD IMPLEMENTATIONS
                // ========================================================================

                bool connect() override;
                void disconnect() override;
                bool is_connected() const override;

                bool subscribe_trades(const std::string &symbol) override;
                bool subscribe_level2(const std::string &symbol) override;
                bool unsubscribe(const std::string &symbol) override;
                bool subscribe_multiple_symbols(const std::vector<std::string> &symbols) override;

                // ========================================================================
                // BINANCE SYMBOL MAPPING
                // ========================================================================

                std::string normalize_symbol(const std::string &exchange_symbol) override;
                std::string exchange_symbol(const std::string &normalized_symbol) override;
                std::vector<std::string> get_available_symbols() override;

                // ========================================================================
                // STATUS AND MONITORING
                // ========================================================================

                std::string get_status() const override;
                std::vector<std::string> get_subscribed_symbols() const override;

            private:
                // ========================================================================
                // BINANCE-SPECIFIC TYPES AND CONSTANTS
                // ========================================================================

                // Note: WebSocket types temporarily commented out for compilation
                // using WebSocketClient = websocketpp::client<websocketpp::config::asio_tls_client>;
                // using ConnectionPtr = WebSocketClient::connection_ptr;
                // using MessagePtr = WebSocketClient::message_ptr;

                static constexpr const char *BINANCE_WS_URL = "wss://stream.binance.com:9443/ws/";
                static constexpr const char *BINANCE_API_URL = "https://api.binance.com";
                static constexpr int PING_INTERVAL_SECONDS = 30;
                static constexpr int RECONNECT_DELAY_SECONDS = 5;
                static constexpr int MAX_RECONNECT_ATTEMPTS = 10;

                // Subscription structure for Binance
                struct BinanceSubscription
                {
                    std::string stream_name; // e.g., "btcusdt@trade"
                    std::string symbol;      // normalized symbol
                    std::string type;        // "trade", "depth"
                    bool active;

                    BinanceSubscription() : active(false) {}
                };

                // ========================================================================
                // CONNECTION MANAGEMENT
                // ========================================================================

                void websocket_worker_loop();
                void ping_loop();
                void reconnect_loop();

                bool establish_websocket_connection();
                void cleanup_websocket_connection();

                // WebSocket event handlers
                // void on_websocket_open(websocketpp::connection_hdl hdl);
                // void on_websocket_close(websocketpp::connection_hdl hdl);
                // void on_websocket_fail(websocketpp::connection_hdl hdl);
                // void on_websocket_message(websocketpp::connection_hdl hdl, MessagePtr msg);

                // ========================================================================
                // MESSAGE PROCESSING (BINANCE FORMAT)
                // ========================================================================

                void process_message(const std::string &message);
                void process_trade_message(const std::string &data);
                void process_depth_message(const std::string &data);
                void process_error_message(const std::string &data);

                // ========================================================================
                // SUBSCRIPTION MANAGEMENT
                // ========================================================================

                bool send_subscribe_message(const std::string &stream_name);
                bool send_unsubscribe_message(const std::string &stream_name);
                std::string create_stream_name(const std::string &symbol, const std::string &type);

                void update_subscriptions();
                void resubscribe_all();

                // ========================================================================
                // BINANCE API HELPERS
                // ========================================================================

                std::vector<std::string> fetch_exchange_info();
                bool is_valid_binance_symbol(const std::string &symbol);

                // Symbol conversion utilities
                std::string to_binance_format(const std::string &normalized_symbol);
                std::string from_binance_format(const std::string &binance_symbol);

                // ========================================================================
                // MEMBER VARIABLES
                // ========================================================================

                // WebSocket connection (temporarily commented out)
                // WebSocketClient ws_client_;
                // websocketpp::connection_hdl ws_connection_hdl_;
                std::atomic<bool> ws_connected_{false};

                // Threading
                std::thread ws_thread_;
                std::thread ping_thread_;
                std::thread reconnect_thread_;
                std::atomic<bool> should_shutdown_{false};

                // Subscription management
                std::unordered_map<std::string, BinanceSubscription> subscriptions_;
                std::mutex subscriptions_mutex_;
                std::unordered_set<std::string> available_symbols_;
                std::mutex symbols_mutex_;

                // Connection monitoring
                std::atomic<std::chrono::steady_clock::time_point> last_message_time_;
                std::atomic<int> reconnect_attempts_{0};

                // Statistics
                std::atomic<uint64_t> total_trades_received_{0};
                std::atomic<uint64_t> total_depth_updates_received_{0};
                std::atomic<uint64_t> total_bytes_received_{0};
                std::atomic<std::chrono::steady_clock::time_point> connection_start_time_;
            };

        } // namespace binance
    } // namespace exchanges
} // namespace open_dtc_server