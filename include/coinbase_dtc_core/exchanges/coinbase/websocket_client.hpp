#pragma once

#include "../../core/util/log.hpp"
#include "../base/exchange_feed.hpp"
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <functional>
#include <unordered_map>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif

namespace open_dtc_server
{
    namespace feed
    {
        namespace coinbase
        {

            /**
             * WebSocket Client for Coinbase Pro/Advanced Trade API
             *
             * This implements a real WebSocket connection to:
             * wss://ws-feed.exchange.coinbase.com
             *
             * Features:
             * - Real-time market data streaming
             * - Trade and level2 order book subscriptions
             * - Automatic reconnection on failures
             * - Thread-safe message handling
             * - Coinbase Pro WebSocket protocol implementation
             */
            class WebSocketClient
            {
            public:
                using TradeCallback = std::function<void(const exchanges::base::MarketTrade &)>;
                using Level2Callback = std::function<void(const exchanges::base::MarketLevel2 &)>;

                WebSocketClient();
                ~WebSocketClient();

                // Connection management
                bool connect(const std::string &host = "ws-feed.exchange.coinbase.com", uint16_t port = 443);
                void disconnect();
                bool is_connected() const { return connected_.load(); }

                // Subscription management
                bool subscribe_trades(const std::string &product_id);
                bool subscribe_level2(const std::string &product_id);
                bool unsubscribe(const std::string &product_id);
                bool subscribe_multiple_symbols(const std::vector<std::string> &product_ids);

                // Callbacks
                void set_trade_callback(TradeCallback callback) { trade_callback_ = callback; }
                void set_level2_callback(Level2Callback callback) { level2_callback_ = callback; }

                // Status
                std::vector<std::string> get_subscribed_symbols() const;
                std::string get_status() const;

            private:
                // WebSocket protocol implementation
                void worker_loop();
                void ping_loop();
                void cleanup_socket();

                // Real WebSocket implementation
                bool establish_websocket_connection();
                bool perform_websocket_handshake();
                bool send_websocket_frame(const std::string &payload);
                bool receive_websocket_frame(std::string &payload);

                // Message handling
                std::string create_subscribe_message(const std::string &channel, const std::string &product_id) const;
                std::string create_unsubscribe_message(const std::string &product_id) const;
                void process_received_message(const std::string &message);
                void parse_trade_message(const std::string &message);
                void parse_level2_message(const std::string &message);

                // WebSocket protocol helpers
                std::string create_websocket_handshake() const;
                std::string encode_websocket_frame(const std::string &payload) const;
                std::string decode_websocket_frame(const std::string &frame) const;

                // Utility functions
                uint64_t get_current_timestamp() const;
                std::string generate_websocket_key() const;
                std::string calculate_websocket_accept(const std::string &key) const;

                // Connection state
                std::atomic<bool> connected_;
                std::atomic<bool> should_stop_;
                int socket_;
                std::string host_;
                uint16_t port_;

                // Threading
                std::thread worker_thread_;
                std::thread ping_thread_;

                // Subscriptions
                std::vector<std::string> subscribed_symbols_;
                mutable std::mutex subscriptions_mutex_;

                // Message queues
                std::queue<std::string> send_queue_;
                std::mutex send_queue_mutex_;
                std::mutex callback_mutex_;

                // Callbacks
                TradeCallback trade_callback_;
                Level2Callback level2_callback_;

                // Statistics
                std::atomic<int> messages_received_;
                std::atomic<int> messages_sent_;
                std::atomic<uint64_t> last_message_time_;
            };

        } // namespace coinbase
    } // namespace feed
} // namespace open_dtc_server