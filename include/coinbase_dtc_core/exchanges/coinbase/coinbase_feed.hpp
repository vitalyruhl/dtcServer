#pragma once

#include "../base/exchange_feed.hpp"
#include "../../core/http/http_client.hpp"
#include "../../core/auth/jwt_auth.hpp"
#include "../../core/util/log.hpp"
#include "endpoint.hpp"
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <condition_variable>
#include <chrono>
#include <memory>

// Forward declaration to avoid circular includes
namespace open_dtc_server
{
    namespace feed
    {
        namespace coinbase
        {
            class WebSocketClient;
        }
    }
}

namespace open_dtc_server
{
    namespace exchanges
    {
        namespace coinbase
        {

            /**
             * Coinbase Pro/Advanced Trade exchange implementation.
             *
             * This serves as a TEMPLATE for implementing new exchanges.
             * To add a new exchange (e.g., Binance):
             *
             * 1. Copy this entire coinbase folder to exchanges/binance/
             * 2. Rename CoinbaseFeed to BinanceFeed
             * 3. Update namespace from coinbase to binance
             * 4. Implement exchange-specific symbol mapping
             * 5. Implement exchange-specific WebSocket protocol
             * 6. Update ExchangeFactory to include the new exchange
             */
            class CoinbaseFeed : public base::ExchangeFeedBase
            {
            public:
                explicit CoinbaseFeed(const base::ExchangeConfig &config);
                ~CoinbaseFeed() override;

                // ========================================================================
                // IMPLEMENTATION OF PURE VIRTUAL METHODS
                // ========================================================================

                bool connect() override;
                void disconnect() override;
                bool is_connected() const override;

                bool subscribe_trades(const std::string &symbol) override;
                bool subscribe_level2(const std::string &symbol) override;
                bool unsubscribe(const std::string &symbol) override;
                bool subscribe_multiple_symbols(const std::vector<std::string> &symbols) override;

                // ========================================================================
                // COINBASE-SPECIFIC SYMBOL MAPPING
                // ========================================================================

                std::string normalize_symbol(const std::string &coinbase_symbol) override;
                std::string exchange_symbol(const std::string &normalized_symbol) override;
                std::vector<std::string> get_available_symbols() override;

                // ========================================================================
                // STATUS AND MONITORING
                // ========================================================================

                std::string get_status() const override;
                std::vector<std::string> get_subscribed_symbols() const override;

                // ========================================================================
                // COINBASE-SPECIFIC PUBLIC METHODS
                // ========================================================================

                /** Get current server time from Coinbase API */
                bool get_server_time(uint64_t &timestamp);

                /** Get product information for a specific symbol */
                bool get_product_info(const std::string &product_id, std::string &info);

                /** Get current ticker data for a symbol */
                bool get_ticker(const std::string &product_id, base::MarketLevel2 &ticker);

                /** Validate API credentials */
                bool test_credentials();

            private:
                // ========================================================================
                // COINBASE-SPECIFIC IMPLEMENTATION DETAILS
                // ========================================================================

                // WebSocket worker threads
                void websocket_worker_loop();
                void ping_loop();
                void reconnect_loop();

                // WebSocket protocol (Coinbase-specific)
                bool establish_websocket_connection();
                void cleanup_websocket();
                std::string create_subscribe_message(const std::string &channel,
                                                     const std::vector<std::string> &product_ids) const;
                std::string create_unsubscribe_message(const std::string &channel,
                                                       const std::vector<std::string> &product_ids) const;

                // Message processing
                void process_websocket_message(const std::string &message);
                void handle_trade_message(const std::string &message);
                void handle_level2_message(const std::string &message);
                void handle_ticker_message(const std::string &message);
                void handle_heartbeat_message(const std::string &message);
                void handle_error_message(const std::string &message);

                // WebSocket callbacks
                void on_trade_received(const exchanges::base::MarketTrade &trade);
                void on_level2_received(const exchanges::base::MarketLevel2 &level2);

                // Symbol mapping initialization
                void initialize_symbol_mappings();
                bool fetch_product_list();

                // HTTP API helpers
                std::string make_authenticated_request(const std::string &method,
                                                       const std::string &path,
                                                       const std::string &body = "");

                // Connection management
                bool should_reconnect() const;
                void schedule_reconnect();

                // Utility methods
                void cleanup_socket();
                uint64_t get_current_timestamp() const;
                std::string format_timestamp(uint64_t timestamp) const;
                bool parse_json_safely(const std::string &json, std::string &error);

                // ========================================================================
                // WEBSOCKET CHANNELS AND SUBSCRIPTION MANAGEMENT
                // ========================================================================

                enum class SubscriptionType
                {
                    TRADES,
                    LEVEL2,
                    TICKER,
                    HEARTBEAT
                };

                struct SubscriptionInfo
                {
                    SubscriptionType type;
                    std::string product_id;
                    bool active;
                    uint64_t subscribed_at;

                    SubscriptionInfo() : type(SubscriptionType::TRADES), product_id(""), active(false), subscribed_at(0) {}
                    SubscriptionInfo(SubscriptionType t, const std::string &pid)
                        : type(t), product_id(pid), active(false), subscribed_at(0) {}
                };

                void add_subscription(SubscriptionType type, const std::string &product_id);
                void remove_subscription(SubscriptionType type, const std::string &product_id);
                bool has_subscription(SubscriptionType type, const std::string &product_id) const;
                std::vector<std::string> get_subscribed_products_for_type(SubscriptionType type) const;

                // ========================================================================
                // PRIVATE MEMBER VARIABLES
                // ========================================================================

                // HTTP client for REST API calls
                std::unique_ptr<http::IHttpClient> http_client_;

                // WebSocket client for real-time data
                std::unique_ptr<feed::coinbase::WebSocketClient> websocket_client_; // Re-enabled

                // Authentication
                std::unique_ptr<auth::JWTAuthenticator> authenticator_;
                auth::CDPCredentials credentials_;

                // Symbol mapping tables (Coinbase-specific format)
                std::unordered_map<std::string, std::string> coinbase_to_normalized_;
                std::unordered_map<std::string, std::string> normalized_to_coinbase_;
                std::unordered_set<std::string> available_symbols_;
                mutable std::mutex symbol_mappings_mutex_;

                // WebSocket connection state
                std::atomic<bool> connected_;
                std::atomic<bool> should_stop_;
                std::atomic<bool> websocket_connected_;
                int socket_fd_;
                std::string websocket_host_;
                std::string websocket_path_;
                uint16_t websocket_port_;

                // Threading
                std::thread websocket_worker_thread_;
                std::thread ping_thread_;
                std::thread reconnect_thread_;

                // Subscription management
                mutable std::mutex subscriptions_mutex_;
                std::unordered_map<std::string, SubscriptionInfo> subscriptions_; // key: type_productid
                std::vector<std::string> subscribed_symbols_;                     // Cache for quick access

                // Message queues and synchronization
                std::mutex send_queue_mutex_;
                std::queue<std::string> send_queue_;
                std::condition_variable send_cv_;

                std::mutex receive_queue_mutex_;
                std::queue<std::string> receive_queue_;
                std::condition_variable receive_cv_;

                // Reconnection logic
                std::atomic<bool> should_reconnect_;
                std::atomic<uint64_t> last_successful_connection_;
                std::atomic<uint64_t> reconnect_attempts_;
                std::atomic<uint64_t> next_reconnect_time_;
                static constexpr uint64_t INITIAL_RECONNECT_DELAY_MS = 1000;
                static constexpr uint64_t MAX_RECONNECT_DELAY_MS = 30000;
                static constexpr uint64_t MAX_RECONNECT_ATTEMPTS = 10;

                // Statistics and monitoring
                std::atomic<uint64_t> messages_received_;
                std::atomic<uint64_t> messages_sent_;
                std::atomic<uint64_t> last_message_time_;
                std::atomic<uint64_t> last_heartbeat_time_;
                std::atomic<uint64_t> total_trades_received_;
                std::atomic<uint64_t> total_level2_updates_;
                std::atomic<uint64_t> connection_uptime_start_;

                // WebSocket constants
                static constexpr const char *WEBSOCKET_HOST = "ws-feed.exchange.coinbase.com";
                static constexpr uint16_t WEBSOCKET_PORT = 443;
                static constexpr const char *WEBSOCKET_PATH = "/";
                static constexpr uint64_t PING_INTERVAL_MS = 30000;     // 30 seconds
                static constexpr uint64_t HEARTBEAT_TIMEOUT_MS = 60000; // 1 minute

                // Coinbase WebSocket channels
                static constexpr const char *CHANNEL_TRADES = "matches";
                static constexpr const char *CHANNEL_LEVEL2 = "level2";
                static constexpr const char *CHANNEL_TICKER = "ticker";
                static constexpr const char *CHANNEL_HEARTBEAT = "heartbeat";

                // Rate limiting
                std::atomic<uint64_t> last_request_time_;
                static constexpr uint64_t MIN_REQUEST_INTERVAL_MS = 100; // 10 requests/second max

                // Error tracking
                mutable std::mutex error_mutex_;
                std::string last_error_;
                uint64_t last_error_time_;
                std::unordered_map<std::string, uint64_t> error_counts_;
            };

            // ========================================================================
            // COINBASE PROTOCOL CONSTANTS AND UTILITIES
            // ========================================================================

            namespace protocol
            {

                /** WebSocket message types from Coinbase */
                enum class MessageType
                {
                    SUBSCRIBE,
                    UNSUBSCRIBE,
                    SUBSCRIPTIONS,
                    MATCH,    // Trade data
                    SNAPSHOT, // Level2 snapshot
                    L2UPDATE, // Level2 update
                    TICKER,
                    HEARTBEAT,
                    ERROR,
                    UNKNOWN
                };

                /** Parse message type from JSON */
                MessageType parse_message_type(const std::string &message);

                /** Create subscription message JSON */
                std::string create_subscription_json(const std::vector<std::string> &channels,
                                                     const std::vector<std::string> &product_ids);

                /** Create unsubscription message JSON */
                std::string create_unsubscription_json(const std::vector<std::string> &channels,
                                                       const std::vector<std::string> &product_ids);

                // ========================================================================
                // WEBSOCKET CALLBACK METHODS
                // ========================================================================

                /** Handle received trade data from WebSocket */
                void on_trade_received(const base::MarketTrade &trade);

                /** Handle received level2 data from WebSocket */
                void on_level2_received(const base::MarketLevel2 &level2);

                /** Validate Coinbase product ID format */
                bool is_valid_product_id(const std::string &product_id);

                /** Extract product ID from Coinbase symbol */
                std::string extract_product_id(const std::string &symbol);

            } // namespace protocol

        } // namespace coinbase
    } // namespace exchanges
} // namespace open_dtc_server