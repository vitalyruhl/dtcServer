#pragma once

#include "coinbase_dtc_core/core/dtc/protocol.hpp"
#include "coinbase_dtc_core/exchanges/base/exchange_feed.hpp"
#include "coinbase_dtc_core/exchanges/factory/exchange_factory.hpp"
#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <functional>

namespace coinbase_dtc_core
{
    namespace core
    {
        namespace server
        {

            // Forward declarations
            class ClientConnection;

            /**
             * DTC Server Configuration
             */
            struct ServerConfig
            {
                std::string bind_address = "0.0.0.0";
                uint16_t port = 11099;
                std::string server_name = "CoinbaseDTCServer";
                std::string password = "";
                bool require_authentication = false;
                uint16_t protocol_version = 8;
                int max_clients = 100;

                // Exchange configuration
                std::vector<open_dtc_server::exchanges::base::ExchangeConfig> exchanges;

                // Logging
                bool enable_logging = true;
                std::string log_level = "INFO";
            };

            /**
             * Client session information
             */
            struct ClientSession
            {
                std::string client_info;
                std::string username;
                bool authenticated = false;
                std::chrono::steady_clock::time_point connect_time;
                std::chrono::steady_clock::time_point last_heartbeat;
                std::vector<std::string> subscribed_symbols;
                uint32_t next_symbol_id = 1;
                std::unordered_map<std::string, uint32_t> symbol_to_id;
                std::unordered_map<uint32_t, std::string> id_to_symbol;
            };

            /**
             * Main DTC Server class.
             *
             * Handles client connections, authentication, market data distribution,
             * and exchange management.
             */
            class DTCServer
            {
            public:
                explicit DTCServer(const ServerConfig &config);
                ~DTCServer();

                // ========================================================================
                // SERVER LIFECYCLE
                // ========================================================================

                /**
                 * Start the DTC server.
                 * @return true if server started successfully, false otherwise
                 */
                bool start();

                /**
                 * Stop the DTC server.
                 */
                void stop();

                /**
                 * Check if server is running.
                 * @return true if server is running, false otherwise
                 */
                bool is_running() const { return server_running_; }

                // ========================================================================
                // EXCHANGE MANAGEMENT
                // ========================================================================

                /**
                 * Add an exchange to the server.
                 * @param exchange_config Configuration for the exchange
                 * @return true if exchange was added successfully, false otherwise
                 */
                bool add_exchange(const open_dtc_server::exchanges::base::ExchangeConfig &exchange_config);

                /**
                 * Remove an exchange from the server.
                 * @param exchange_name Name of the exchange to remove
                 * @return true if exchange was removed successfully, false otherwise
                 */
                bool remove_exchange(const std::string &exchange_name);

                /**
                 * Get list of active exchanges.
                 * @return Vector of exchange names
                 */
                std::vector<std::string> get_active_exchanges() const;

                // ========================================================================
                // SYMBOL MANAGEMENT
                // ========================================================================

                /**
                 * Subscribe to a symbol on specific exchange for all clients.
                 * @param symbol Normalized symbol (e.g., "BTC/USD")
                 * @param exchange Exchange name (empty = all exchanges)
                 * @return true if subscription was successful, false otherwise
                 */
                bool subscribe_symbol(const std::string &symbol, const std::string &exchange = "");

                /**
                 * Unsubscribe from a symbol on specific exchange.
                 * @param symbol Normalized symbol
                 * @param exchange Exchange name (empty = all exchanges)
                 * @return true if unsubscription was successful, false otherwise
                 */
                bool unsubscribe_symbol(const std::string &symbol, const std::string &exchange = "");

                /**
                 * Get all currently subscribed symbols.
                 * @return Vector of subscribed symbols
                 */
                std::vector<std::string> get_subscribed_symbols() const;

                // ========================================================================
                // SERVER STATUS AND MONITORING
                // ========================================================================

                /**
                 * Get server status information.
                 * @return Status string with server and exchange information
                 */
                std::string get_status() const;

                /**
                 * Get number of connected clients.
                 * @return Number of connected clients
                 */
                int get_client_count() const;

                /**
                 * Get server statistics.
                 * @return Statistics string
                 */
                std::string get_statistics() const;

            private:
                // ========================================================================
                // SERVER INTERNALS
                // ========================================================================

                void server_thread_function();
                void client_handler_thread(std::shared_ptr<ClientConnection> client);
                void heartbeat_monitor_thread();

                // Client management
                void add_client(std::shared_ptr<ClientConnection> client);
                void remove_client(std::shared_ptr<ClientConnection> client);
                void broadcast_to_all_clients(const std::vector<uint8_t> &message);
                void send_to_client(std::shared_ptr<ClientConnection> client, const std::vector<uint8_t> &message);

                // Message processing
                void process_client_message(std::shared_ptr<ClientConnection> client, const std::vector<uint8_t> &data);
                void handle_logon_request(std::shared_ptr<ClientConnection> client, const std::vector<uint8_t> &data);
                void handle_market_data_request(std::shared_ptr<ClientConnection> client, const std::vector<uint8_t> &data);
                void handle_heartbeat(std::shared_ptr<ClientConnection> client, const std::vector<uint8_t> &data);

                // Exchange callbacks
                void on_trade_data(const open_dtc_server::exchanges::base::MarketTrade &trade);
                void on_level2_data(const open_dtc_server::exchanges::base::MarketLevel2 &level2);
                void on_exchange_connection(bool connected, const std::string &exchange);
                void on_exchange_error(const std::string &error, const std::string &exchange);

                // Utilities
                uint32_t get_or_create_symbol_id(std::shared_ptr<ClientConnection> client, const std::string &symbol);
                std::string normalize_symbol_for_client(const std::string &symbol);

                // ========================================================================
                // MEMBER VARIABLES
                // ========================================================================

                ServerConfig config_;
                std::atomic<bool> server_running_{false};
                std::atomic<bool> should_shutdown_{false};

                // Threading
                std::thread server_thread_;
                std::thread heartbeat_thread_;

                // Protocol handling
                std::unique_ptr<open_dtc_server::core::dtc::Protocol> protocol_;

                // Exchange management
                std::unique_ptr<open_dtc_server::exchanges::base::MultiExchangeFeed> multi_feed_;
                std::mutex exchanges_mutex_;

                // Client management
                std::vector<std::shared_ptr<ClientConnection>> clients_;
                std::mutex clients_mutex_;
                std::atomic<int> next_client_id_{1};

                // Symbol management
                std::unordered_map<std::string, uint32_t> global_symbol_to_id_;
                std::unordered_map<uint32_t, std::string> global_id_to_symbol_;
                std::atomic<uint32_t> next_global_symbol_id_{1};
                std::mutex symbols_mutex_;

                // Statistics
                std::atomic<uint64_t> total_messages_sent_{0};
                std::atomic<uint64_t> total_messages_received_{0};
                std::atomic<uint64_t> total_trade_updates_sent_{0};
                std::atomic<uint64_t> total_level2_updates_sent_{0};
                std::chrono::steady_clock::time_point server_start_time_;
            };

            /**
             * Represents a client connection to the DTC server.
             */
            class ClientConnection
            {
            public:
                ClientConnection(int socket_fd, int client_id);
                ~ClientConnection();

                // Connection management
                bool is_connected() const { return connected_; }
                void disconnect();

                // Message I/O
                bool send_message(const std::vector<uint8_t> &message);
                std::vector<uint8_t> receive_message();

                // Client information
                int get_client_id() const { return client_id_; }
                const ClientSession &get_session() const { return session_; }
                ClientSession &get_session() { return session_; }

                std::string get_client_info() const;

            private:
                int socket_fd_;
                int client_id_;
                std::atomic<bool> connected_{true};
                ClientSession session_;
                std::mutex send_mutex_;
                std::mutex receive_mutex_;
            };

        } // namespace server
    } // namespace core
} // namespace open_dtc_server