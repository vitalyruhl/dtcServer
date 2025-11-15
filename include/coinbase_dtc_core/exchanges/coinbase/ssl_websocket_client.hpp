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
#include <memory>

// OpenSSL includes
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

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
             * SSL WebSocket Client for Coinbase Advanced Trade API
             *
             * Features:
             * - SSL/TLS encrypted connections using OpenSSL
             * - JWT authentication for Coinbase Advanced Trade API
             * - Real-time market data streaming over secure WebSocket
             * - Automatic reconnection with exponential backoff
             * - Thread-safe message handling
             * - Full WebSocket protocol implementation (RFC 6455)
             */
            class SSLWebSocketClient
            {
            public:
                SSLWebSocketClient();
                ~SSLWebSocketClient();

                // Connection management
                bool connect(const std::string &host = "ws-feed.exchange.coinbase.com",
                             uint16_t port = 443);
                void disconnect();
                bool is_connected() const { return connected_.load(); }

                // Message handling
                bool send_message(const std::string &message);
                void set_message_callback(std::function<void(const std::string &)> callback);
                void set_connection_callback(std::function<void(bool)> callback);
                void set_error_callback(std::function<void(const std::string &)> callback);

                // Coinbase-specific methods
                bool subscribe_to_ticker(const std::vector<std::string> &symbols);
                bool subscribe_to_level2(const std::vector<std::string> &symbols);
                bool authenticate_with_jwt();

                // Statistics
                uint64_t get_messages_received() const { return messages_received_.load(); }
                uint64_t get_messages_sent() const { return messages_sent_.load(); }
                std::chrono::steady_clock::time_point get_last_message_time() const;

            private:
                // SSL/TLS methods
                bool init_ssl();
                void cleanup_ssl();
                bool create_ssl_socket();
                bool perform_ssl_handshake();
                bool verify_certificate();

                // WebSocket protocol methods
                bool perform_websocket_handshake();
                std::string generate_websocket_key();
                bool validate_websocket_response(const std::string &response);

                // JWT authentication
                std::string generate_jwt_token();
                std::string load_private_key();
                std::string load_api_key_id();

                // Network operations
                bool connect_tcp_socket();
                bool send_ssl_data(const char *data, size_t len);
                int receive_ssl_data(char *buffer, size_t buffer_size);

                // WebSocket frame handling
                std::vector<uint8_t> create_websocket_frame(const std::string &payload, uint8_t opcode = 0x1);
                std::string parse_websocket_frame(const std::vector<uint8_t> &frame);

                // Worker threads
                void worker_loop();
                void ping_loop();
                void process_incoming_messages();

                // Connection state
                std::atomic<bool> connected_;
                std::atomic<bool> should_stop_;
                std::string host_;
                uint16_t port_;

                // SSL context
                SSL_CTX *ssl_ctx_;
                SSL *ssl_;
                BIO *bio_;
                bool ssl_initialized_;

                // Socket
                int socket_fd_;

                // Threading
                std::thread worker_thread_;
                std::thread ping_thread_;
                std::mutex send_mutex_;
                std::mutex receive_mutex_;

                // Message handling
                std::function<void(const std::string &)> message_callback_;
                std::function<void(bool)> connection_callback_;
                std::function<void(const std::string &)> error_callback_;

                // Message queue and processing
                std::queue<std::string> outgoing_messages_;
                std::mutex message_queue_mutex_;
                std::vector<uint8_t> incoming_buffer_;

                // Statistics
                std::atomic<uint64_t> messages_received_;
                std::atomic<uint64_t> messages_sent_;
                std::atomic<std::chrono::steady_clock::time_point::rep> last_message_time_;

                // Reconnection
                std::atomic<int> reconnect_attempts_;
                static constexpr int MAX_RECONNECT_ATTEMPTS = 10;
                static constexpr int INITIAL_RECONNECT_DELAY_MS = 1000;

                // JWT credentials
                std::string api_key_id_;
                std::string private_key_;
                bool credentials_loaded_;
            };

        } // namespace coinbase
    } // namespace feed
} // namespace open_dtc_server