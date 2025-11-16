#include "coinbase_dtc_core/core/server/server.hpp"
#include "coinbase_dtc_core/core/util/log.hpp"
#include "coinbase_dtc_core/exchanges/factory/exchange_factory.hpp"
#include "coinbase_dtc_core/exchanges/coinbase/rest_client.hpp"
#include "coinbase_dtc_core/core/auth/cdp_credentials.hpp"
#include "coinbase_dtc_core/core/auth/jwt_auth.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <cstdio>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace coinbase_dtc_core
{
    namespace core
    {
        namespace server
        {

            // DTCServer Implementation
            DTCServer::DTCServer(const ServerConfig &config)
                : config_(config), server_running_(false)
            {
                std::cout << "DTCServer initialized with config: " + config_.server_name << std::endl;

                // Initialize REST client for Coinbase API access
                try
                {
                    auto credentials = open_dtc_server::auth::CDPCredentials::from_json_file(config_.credentials_file_path);
                    if (credentials.is_valid())
                    {
                        rest_client_ = std::make_unique<open_dtc_server::exchanges::coinbase::CoinbaseRestClient>(credentials);
                        std::cout << "Coinbase REST client initialized successfully" << std::endl;
                    }
                    else
                    {
                        std::cout << "Warning: Invalid CDP credentials, REST client disabled" << std::endl;
                    }
                }
                catch (const std::exception &e)
                {
                    std::cout << "Warning: Failed to initialize REST client: " << e.what() << std::endl;
                }
            }

            DTCServer::~DTCServer()
            {
                stop();
                std::cout << "DTCServer destroyed" << std::endl;
            }

            bool DTCServer::start()
            {
                if (server_running_)
                {
                    std::cout << "Server is already running" << std::endl;
                    return false;
                }

                // Initialize sockets
                if (!initialize_sockets())
                {
                    std::cout << "Failed to initialize sockets" << std::endl;
                    return false;
                }

                // Create server socket
                if (!create_server_socket())
                {
                    std::cout << "Failed to create server socket" << std::endl;
                    cleanup_sockets();
                    return false;
                }

                // Start server
                server_running_ = true;
                should_shutdown_ = false;
                server_start_time_ = std::chrono::steady_clock::now();

                // Start server thread
                server_thread_ = std::thread(&DTCServer::server_thread_function, this);

                std::cout << "DTC Server started successfully on port " + std::to_string(config_.port) << std::endl;
                return true;
            }

            void DTCServer::stop()
            {
                if (!server_running_)
                {
                    return;
                }

                std::cout << "Stopping DTC Server..." << std::endl;

                // Signal shutdown
                should_shutdown_ = true;
                server_running_ = false;

                // Close server socket to break accept() loop
                close_server_socket();

                // Wait for server thread to finish
                if (server_thread_.joinable())
                {
                    server_thread_.join();
                }

                // Wait for heartbeat thread to finish
                if (heartbeat_thread_.joinable())
                {
                    heartbeat_thread_.join();
                }

                // Cleanup
                cleanup_sockets();

                std::cout << "DTC Server stopped" << std::endl;
            }

            bool DTCServer::add_exchange(const open_dtc_server::exchanges::base::ExchangeConfig &exchange_config)
            {
                std::cout << "Adding exchange: " + exchange_config.name << std::endl;

                try
                {
                    // Create exchange feed using factory
                    auto feed = open_dtc_server::exchanges::factory::ExchangeFactory::create_feed(exchange_config);
                    if (!feed)
                    {
                        std::cout << "Failed to create feed for exchange: " + exchange_config.name << std::endl;
                        return false;
                    }

                    // Set up exchange callbacks for market data
                    feed->set_trade_callback([this](const open_dtc_server::exchanges::base::MarketTrade &trade)
                                             { this->on_trade_data(trade); });

                    feed->set_level2_callback([this](const open_dtc_server::exchanges::base::MarketLevel2 &level2)
                                              { this->on_level2_data(level2); });

                    // Connect to the exchange
                    if (!feed->connect())
                    {
                        std::cout << "Failed to connect to exchange: " + exchange_config.name << std::endl;
                        return false;
                    }

                    // Subscribe to configured symbols via subscribe_symbol method
                    // This will be called separately from main.cpp

                    // Store the feed (simplified - using single exchange for now)
                    std::lock_guard<std::mutex> lock(exchanges_mutex_);
                    exchange_feeds_[exchange_config.name] = std::move(feed);

                    std::cout << "[SUCCESS] Successfully added and connected exchange: " + exchange_config.name << std::endl;
                    return true;
                }
                catch (const std::exception &e)
                {
                    std::cout << "Exception adding exchange " + exchange_config.name + ": " + e.what() << std::endl;
                    return false;
                }
            }

            bool DTCServer::remove_exchange(const std::string &exchange_name)
            {
                std::cout << "Removing exchange: " + exchange_name << std::endl;
                // TODO: Implement exchange removal
                return true;
            }

            std::vector<std::string> DTCServer::get_active_exchanges() const
            {
                // TODO: Return actual active exchanges
                return {};
            }

            bool DTCServer::subscribe_symbol(const std::string &symbol, const std::string &exchange)
            {
                std::cout << "Subscribing to symbol: " + symbol + " on exchange: " + exchange << std::endl;
                // TODO: Implement symbol subscription
                return true;
            }

            bool DTCServer::unsubscribe_symbol(const std::string &symbol, const std::string &exchange)
            {
                std::cout << "Unsubscribing from symbol: " + symbol + " on exchange: " + exchange << std::endl;
                // TODO: Implement symbol unsubscription
                return true;
            }

            std::vector<std::string> DTCServer::get_subscribed_symbols() const
            {
                // TODO: Return actual subscribed symbols
                return {};
            }

            std::string DTCServer::get_status() const
            {
                std::ostringstream status;
                status << "DTCServer Status:\n";
                status << "  Running: " << (server_running_ ? "Yes" : "No") << "\n";
                status << "  Port: " << config_.port << "\n";
                status << "  Server Name: " << config_.server_name << "\n";
                status << "  Client Count: " << get_client_count() << "\n";
                return status.str();
            }

            int DTCServer::get_client_count() const
            {
                // TODO: Return actual client count
                return 0;
            }

            // Socket implementation methods
            bool DTCServer::initialize_sockets()
            {
#ifdef _WIN32
                if (winsock_initialized_)
                    return true;

                WSADATA wsaData;
                int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
                if (result != 0)
                {
                    std::cout << "WSAStartup failed: " + std::to_string(result) << std::endl;
                    return false;
                }
                winsock_initialized_ = true;
#endif
                return true;
            }

            void DTCServer::cleanup_sockets()
            {
#ifdef _WIN32
                if (winsock_initialized_)
                {
                    WSACleanup();
                    winsock_initialized_ = false;
                }
#endif
            }

            bool DTCServer::create_server_socket()
            {
                // Create socket
                server_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifdef _WIN32
                if (server_socket_ == INVALID_SOCKET)
#else
                if (server_socket_ < 0)
#endif
                {
                    std::cout << "Failed to create socket" << std::endl;
                    return false;
                }

                // Set socket options
                int opt = 1;
#ifdef _WIN32
                if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) == SOCKET_ERROR)
#else
                if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
#endif
                {
                    std::cout << "Failed to set socket options" << std::endl;
                    close_server_socket();
                    return false;
                }

                // Bind socket
                sockaddr_in server_addr = {};
                server_addr.sin_family = AF_INET;
                server_addr.sin_port = htons(config_.port);

                if (config_.bind_address == "0.0.0.0")
                {
                    server_addr.sin_addr.s_addr = INADDR_ANY;
                }
                else
                {
                    if (inet_pton(AF_INET, config_.bind_address.c_str(), &server_addr.sin_addr) != 1)
                    {
                        std::cout << "Invalid bind address: " + config_.bind_address << std::endl;
                        close_server_socket();
                        return false;
                    }
                }

#ifdef _WIN32
                if (bind(server_socket_, (sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
#else
                if (bind(server_socket_, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
#endif
                {
                    std::cout << "Failed to bind socket to port " + std::to_string(config_.port) << std::endl;
                    close_server_socket();
                    return false;
                }

                // Listen for connections
#ifdef _WIN32
                if (listen(server_socket_, SOMAXCONN) == SOCKET_ERROR)
#else
                if (listen(server_socket_, SOMAXCONN) < 0)
#endif
                {
                    std::cout << "Failed to listen on socket" << std::endl;
                    close_server_socket();
                    return false;
                }

                std::cout << "Server socket listening on " + config_.bind_address + ":" + std::to_string(config_.port) << std::endl;
                return true;
            }

            void DTCServer::close_server_socket()
            {
#ifdef _WIN32
                if (server_socket_ != INVALID_SOCKET)
                {
                    closesocket(server_socket_);
                    server_socket_ = INVALID_SOCKET;
                }
#else
                if (server_socket_ >= 0)
                {
                    close(server_socket_);
                    server_socket_ = -1;
                }
#endif
            }

            void DTCServer::server_thread_function()
            {
                std::cout << "Server thread started, accepting connections..." << std::endl;

                while (server_running_ && !should_shutdown_)
                {
                    sockaddr_in client_addr = {};
                    socklen_t client_len = sizeof(client_addr);

#ifdef _WIN32
                    SOCKET client_socket = accept(server_socket_, (sockaddr *)&client_addr, &client_len);
                    if (client_socket == INVALID_SOCKET)
                    {
                        if (server_running_)
                        {
                            std::cout << "Accept failed: " + std::to_string(WSAGetLastError()) << std::endl;
                        }
                        break;
                    }
#else
                    int client_socket = accept(server_socket_, (sockaddr *)&client_addr, &client_len);
                    if (client_socket < 0)
                    {
                        if (server_running_)
                        {
                            std::cout << "Accept failed" << std::endl;
                        }
                        break;
                    }
#endif

                    // Create client connection
                    int client_id = next_client_id_++;
                    auto client = std::make_shared<ClientConnection>(client_socket, client_id);

                    // Get client IP
                    char client_ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

                    std::cout << "New client connection from " + std::string(client_ip) + " (ID: " + std::to_string(client_id) + ")" << std::endl;

                    // Add to client list
                    add_client(client);

                    // Start client handler thread
                    std::thread client_thread(&DTCServer::client_handler_thread, this, client);
                    client_thread.detach();
                }

                std::cout << "Server thread ending" << std::endl;
            }

            void DTCServer::client_handler_thread(std::shared_ptr<ClientConnection> client)
            {
                std::cout << "Client handler thread started for client " + std::to_string(client->get_client_id()) << std::endl;

                // Create DTC protocol handler for this client
                open_dtc_server::core::dtc::Protocol protocol_handler;
                std::vector<uint8_t> incoming_buffer;

                // Process DTC messages while connected
                while (server_running_ && !should_shutdown_ && client->is_connected())
                {
                    // Receive data from client
                    std::vector<uint8_t> data = client->receive_message();
                    if (data.empty())
                    {
                        // Client disconnected or no data - wait a bit before retrying
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        continue;
                    }

                    // Add received data to buffer
                    incoming_buffer.insert(incoming_buffer.end(), data.begin(), data.end());

                    // Process complete DTC messages from buffer
                    while (incoming_buffer.size() >= 4) // Minimum DTC message size (header)
                    {
                        // Check if we have a complete message
                        if (incoming_buffer.size() < 4)
                            break;

                        uint16_t message_size = *reinterpret_cast<const uint16_t *>(incoming_buffer.data());
                        if (message_size < 4 || message_size > 65535)
                        {
                            std::cout << "Invalid DTC message size: " + std::to_string(message_size) << std::endl;
                            break;
                        }

                        if (incoming_buffer.size() < message_size)
                        {
                            // Wait for more data
                            break;
                        }

                        // Parse DTC message
                        try
                        {
                            auto dtc_message = protocol_handler.parse_message(incoming_buffer.data(), message_size);
                            if (dtc_message)
                            {
                                process_dtc_message(client, std::move(dtc_message), protocol_handler);
                            }
                        }
                        catch (const std::exception &e)
                        {
                            std::cout << "Error parsing DTC message: " + std::string(e.what()) << std::endl;
                        }

                        // Remove processed message from buffer
                        incoming_buffer.erase(incoming_buffer.begin(), incoming_buffer.begin() + message_size);
                    }
                }

                // Client disconnected or server shutdown
                remove_client(client);
                std::cout << "Client " + std::to_string(client->get_client_id()) + " disconnected" << std::endl;
            }

            void DTCServer::add_client(std::shared_ptr<ClientConnection> client)
            {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                clients_.push_back(client);
            }

            void DTCServer::remove_client(std::shared_ptr<ClientConnection> client)
            {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                clients_.erase(std::remove(clients_.begin(), clients_.end(), client), clients_.end());
            }

            // ========================================================================
            // EXCHANGE CALLBACK IMPLEMENTATIONS
            // ========================================================================

            void DTCServer::on_trade_data(const open_dtc_server::exchanges::base::MarketTrade &trade)
            {
                // Broadcast trade data to connected clients via DTC protocol
                if (trade.symbol.empty() == false && trade.price > 0)
                {
                    std::lock_guard<std::mutex> lock(clients_mutex_);

                    // Create DTC protocol instance
                    open_dtc_server::core::dtc::Protocol protocol;

                    // Find clients subscribed to this symbol
                    int broadcasts = 0;
                    for (auto client : clients_)
                    {
                        if (!client || !client->is_connected())
                            continue;

                        auto &subscriptions = client->get_session().subscribed_symbols;
                        if (std::find(subscriptions.begin(), subscriptions.end(), trade.symbol) != subscriptions.end())
                        {
                            // Get symbol ID for this client
                            auto symbol_id_it = client->get_session().symbol_to_id.find(trade.symbol);
                            if (symbol_id_it != client->get_session().symbol_to_id.end())
                            {
                                // Create trade update message
                                auto trade_update = protocol.create_trade_update(
                                    symbol_id_it->second,                                         // symbol_id
                                    trade.price,                                                  // price
                                    trade.volume,                                                 // volume
                                    open_dtc_server::core::dtc::Protocol::get_current_timestamp() // timestamp
                                );

                                auto message_data = protocol.create_message(*trade_update);
                                client->send_message(message_data);
                                broadcasts++;
                            }
                        }
                    }

                    if (broadcasts > 0)
                    {
                        std::cout << "[TRADE] Trade broadcasted: " + trade.symbol + " = $" + std::to_string(trade.price) + " to " + std::to_string(broadcasts) + " clients" << std::endl;
                    }
                }
            }

            void DTCServer::on_level2_data(const open_dtc_server::exchanges::base::MarketLevel2 &level2)
            {
                // Broadcast level2 data to connected clients
                if (level2.symbol.empty() == false && level2.bid_price > 0)
                {
                    std::lock_guard<std::mutex> lock(clients_mutex_);

                    // Create DTC protocol instance
                    open_dtc_server::core::dtc::Protocol protocol;

                    // Find clients subscribed to this symbol
                    int broadcasts = 0;
                    for (auto client : clients_)
                    {
                        if (!client || !client->is_connected())
                            continue;

                        auto &subscriptions = client->get_session().subscribed_symbols;
                        if (std::find(subscriptions.begin(), subscriptions.end(), level2.symbol) != subscriptions.end())
                        {
                            // Get symbol ID for this client
                            auto symbol_id_it = client->get_session().symbol_to_id.find(level2.symbol);
                            if (symbol_id_it != client->get_session().symbol_to_id.end())
                            {
                                // Create bid/ask update message
                                auto bid_ask_update = protocol.create_bid_ask_update(
                                    symbol_id_it->second,                                         // symbol_id
                                    level2.bid_price,                                             // bid_price
                                    level2.bid_size,                                              // bid_quantity (fixed field name)
                                    level2.ask_price,                                             // ask_price
                                    level2.ask_size,                                              // ask_quantity (fixed field name)
                                    open_dtc_server::core::dtc::Protocol::get_current_timestamp() // timestamp
                                );

                                auto message_data = protocol.create_message(*bid_ask_update);
                                client->send_message(message_data);
                                broadcasts++;
                            }
                        }
                    }

                    if (broadcasts > 0)
                    {
                        std::cout << "[LEVEL2] Level2 broadcasted: " + level2.symbol + " Bid=$" + std::to_string(level2.bid_price) + " Ask=$" + std::to_string(level2.ask_price) + " to " + std::to_string(broadcasts) + " clients" << std::endl;
                    }
                }
            }

            void DTCServer::on_exchange_connection(bool connected, const std::string &exchange)
            {
                if (connected)
                {
                    std::cout << "Exchange connected: " + exchange << std::endl;
                }
                else
                {
                    std::cout << "Exchange disconnected: " + exchange << std::endl;
                }
            }

            void DTCServer::on_exchange_error(const std::string &error, const std::string &exchange)
            {
                std::cout << "Exchange error [" + exchange + "]: " + error << std::endl;
            } // ========================================================================
            // DTC MESSAGE PROCESSING
            // ========================================================================

            void DTCServer::process_dtc_message(std::shared_ptr<ClientConnection> client,
                                                std::unique_ptr<open_dtc_server::core::dtc::DTCMessage> message,
                                                open_dtc_server::core::dtc::Protocol &protocol)
            {
                if (!message)
                    return;

                // std::cout << "Processing DTC message type: " + open_dtc_server::core::dtc::Protocol::message_type_to_string(message->get_type()) + " from client " + std::to_string(client->get_client_id()) << std::endl;

                switch (message->get_type())
                {
                case open_dtc_server::core::dtc::MessageType::LOGON_REQUEST:
                {
                    // Cast to LogonRequest and handle
                    auto *logon_req = static_cast<open_dtc_server::core::dtc::LogonRequest *>(message.get());

                    std::cout << "LogonRequest from: " + logon_req->client_name + " (user: " + logon_req->username + ")" << std::endl;

                    // Create successful logon response
                    auto logon_response = protocol.create_logon_response(true, "Login successful");
                    logon_response->server_name = config_.server_name;
                    logon_response->market_depth_updates_best_bid_and_ask = 1;
                    logon_response->trading_is_supported = 1;
                    logon_response->security_definitions_supported = 1;
                    logon_response->market_depth_is_supported = 1;

                    // Serialize and send response
                    auto response_data = protocol.create_message(*logon_response);
                    client->send_message(response_data);

                    std::cout << "LogonResponse sent to client " + std::to_string(client->get_client_id()) << std::endl;

                    // Send real account data after successful login
                    send_account_data_to_client(client);
                    break;
                }

                case open_dtc_server::core::dtc::MessageType::SECURITY_DEFINITION_FOR_SYMBOL_REQUEST:
                {
                    auto *symbol_req = static_cast<open_dtc_server::core::dtc::SecurityDefinitionForSymbolRequest *>(message.get());

                    std::cout << "SecurityDefinitionRequest: " + symbol_req->symbol + " on " + symbol_req->exchange + " (product_type: " + symbol_req->product_type + ")" << std::endl;

                    // Determine product type filter
                    open_dtc_server::exchanges::coinbase::ProductType product_filter = open_dtc_server::exchanges::coinbase::ProductType::ALL;
                    if (symbol_req->product_type == "SPOT")
                    {
                        product_filter = open_dtc_server::exchanges::coinbase::ProductType::SPOT;
                    }
                    else if (symbol_req->product_type == "FUTURE")
                    {
                        product_filter = open_dtc_server::exchanges::coinbase::ProductType::FUTURE;
                    }

                    // Fetch filtered products from Coinbase API
                    std::vector<open_dtc_server::exchanges::coinbase::Product> products;
                    std::vector<std::string> symbols;

                    if (rest_client_ && rest_client_->get_products_filtered(products, product_filter))
                    {
                        // Extract symbols from products
                        for (const auto &product : products)
                        {
                            symbols.push_back(product.product_id);
                        }

                        std::cout << "Retrieved " + std::to_string(symbols.size()) + " " + symbol_req->product_type + " symbols from Coinbase API" << std::endl;

                        // Limit to reasonable number for UI
                        if (symbols.size() > 20)
                        {
                            symbols.resize(20);
                            std::cout << "Limited to first 20 symbols for GUI performance" << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "Failed to fetch symbols from Coinbase API, using fallback list" << std::endl;
                        symbols = {"BTC-USD"};
                    }

                    for (const auto &symbol : symbols)
                    {
                        auto symbol_response = protocol.create_security_definition_response(
                            symbol_req->request_id, symbol, "coinbase");

                        auto response_data = protocol.create_message(*symbol_response);
                        client->send_message(response_data);
                    }

                    std::cout << "SecurityDefinition responses sent for " + std::to_string(symbols.size()) + " " + symbol_req->product_type + " symbols" << std::endl;
                    break;
                }
                case open_dtc_server::core::dtc::MessageType::MARKET_DATA_REQUEST:
                {
                    auto *market_req = static_cast<open_dtc_server::core::dtc::MarketDataRequest *>(message.get());

                    std::cout << "MarketDataRequest: " + std::string(market_req->request_action == open_dtc_server::core::dtc::RequestAction::SUBSCRIBE ? "SUBSCRIBE" : "UNSUBSCRIBE") + " to " + market_req->symbol + " on " + market_req->exchange << std::endl;

                    // Add symbol to client's subscription list
                    if (market_req->request_action == open_dtc_server::core::dtc::RequestAction::SUBSCRIBE)
                    {
                        // Assign symbol ID if not provided
                        if (market_req->symbol_id == 0)
                        {
                            market_req->symbol_id = client->get_session().next_symbol_id++;
                        }

                        // Store symbol mapping
                        client->get_session().symbol_to_id[market_req->symbol] = market_req->symbol_id;
                        client->get_session().id_to_symbol[market_req->symbol_id] = market_req->symbol;

                        // Add to subscriptions
                        auto &subscriptions = client->get_session().subscribed_symbols;
                        if (std::find(subscriptions.begin(), subscriptions.end(), market_req->symbol) == subscriptions.end())
                        {
                            subscriptions.push_back(market_req->symbol);
                        }

                        bool client_subscription = true;
                        if (client_subscription)
                        {
                            std::cout << "[SUCCESS] Client " + std::to_string(client->get_client_id()) + " subscribed to " + market_req->symbol + " (ID: " + std::to_string(market_req->symbol_id) + ")" << std::endl;
                        }
                    }
                    else if (market_req->request_action == open_dtc_server::core::dtc::RequestAction::UNSUBSCRIBE)
                    {
                        // Remove from subscriptions
                        auto &subscriptions = client->get_session().subscribed_symbols;
                        subscriptions.erase(std::remove(subscriptions.begin(), subscriptions.end(), market_req->symbol), subscriptions.end());

                        bool client_unsubscription = true;
                        if (client_unsubscription)
                        {
                            std::cout << "[INFO] Client " + std::to_string(client->get_client_id()) + " unsubscribed from " + market_req->symbol << std::endl;
                        }
                    }
                    break;
                }

                case open_dtc_server::core::dtc::MessageType::CURRENT_POSITIONS_REQUEST:
                {
                    auto *positions_req = static_cast<open_dtc_server::core::dtc::CurrentPositionsRequest *>(message.get());

                    std::cout << "CurrentPositionsRequest from client " + std::to_string(client->get_client_id()) + " for account: " + positions_req->trade_account << std::endl;

                    // Send real account data
                    send_account_data_to_client(client);
                    break;
                }

                case open_dtc_server::core::dtc::MessageType::HEARTBEAT:
                {
                    // Echo heartbeat back
                    auto *heartbeat = static_cast<open_dtc_server::core::dtc::Heartbeat *>(message.get());
                    auto heartbeat_response = protocol.create_heartbeat(heartbeat->num_drops);
                    auto response_data = protocol.create_message(*heartbeat_response);
                    client->send_message(response_data);
                    break;
                }

                default:
                    std::cout << "[WARNING] Unhandled DTC message type: " + std::to_string(static_cast<uint16_t>(message->get_type())) << std::endl;
                    break;
                }
            }

        } // namespace server
    } // namespace core
} // namespace coinbase_dtc_core

// ========================================================================
// ClientConnection Implementation
// ========================================================================

namespace coinbase_dtc_core
{
    namespace core
    {
        namespace server
        {

            ClientConnection::ClientConnection(int socket_fd, int client_id)
                : socket_fd_(socket_fd), client_id_(client_id), connected_(true)
            {
                session_.connect_time = std::chrono::steady_clock::now();
                session_.last_heartbeat = session_.connect_time;
            }

            ClientConnection::~ClientConnection()
            {
                disconnect();
            }

            void ClientConnection::disconnect()
            {
                if (!connected_)
                    return;

                connected_ = false;
#ifdef _WIN32
                if (socket_fd_ != INVALID_SOCKET)
                {
                    closesocket(socket_fd_);
                    socket_fd_ = INVALID_SOCKET;
                }
#else
                if (socket_fd_ >= 0)
                {
                    close(socket_fd_);
                    socket_fd_ = -1;
                }
#endif
            }

            bool ClientConnection::send_message(const std::vector<uint8_t> &message)
            {
                if (!connected_)
                    return false;

                std::lock_guard<std::mutex> lock(send_mutex_);

#ifdef _WIN32
                int result = send(socket_fd_, (const char *)message.data(), message.size(), 0);
                return result != SOCKET_ERROR;
#else
                ssize_t result = send(socket_fd_, message.data(), message.size(), 0);
                return result >= 0;
#endif
            }

            std::vector<uint8_t> ClientConnection::receive_message()
            {
                std::vector<uint8_t> buffer(4096);
                std::lock_guard<std::mutex> lock(receive_mutex_);

                if (!connected_)
                    return {};

#ifdef _WIN32
                int bytes_received = recv(socket_fd_, (char *)buffer.data(), buffer.size(), 0);
                if (bytes_received <= 0)
                {
                    connected_ = false;
                    return {};
                }
#else
                ssize_t bytes_received = recv(socket_fd_, buffer.data(), buffer.size(), 0);
                if (bytes_received <= 0)
                {
                    connected_ = false;
                    return {};
                }
#endif

                buffer.resize(bytes_received);
                return buffer;
            }

            std::string ClientConnection::get_client_info() const
            {
                return "Client " + std::to_string(client_id_) + " - " + session_.client_info;
            }

            void DTCServer::send_account_data_to_client(std::shared_ptr<ClientConnection> client)
            {
                std::cout << "[ACCOUNT] Fetching real Coinbase account data for client " + std::to_string(client->get_client_id()) << std::endl;

                try
                {
                    // Load Coinbase credentials using the CDP format from configured path
                    auto credentials = open_dtc_server::auth::CDPCredentials::from_json_file(config_.credentials_file_path);

                    if (!credentials.is_valid())
                    {
                        credentials = open_dtc_server::auth::CDPCredentials::from_environment();
                    }

                    if (!credentials.is_valid())
                    {
                        std::cout << "[ERROR] No Coinbase CDP credentials available - cannot fetch account data" << std::endl;
                        std::cout << "[INFO] Tried credentials file: " + config_.credentials_file_path << std::endl;
                        std::cout << "[INFO] Use --credentials <path> to specify credentials file location" << std::endl;
                        std::cout << "[INFO] Account data request failed - no credentials configured" << std::endl;
                        return;
                    }

                    // Create REST client and fetch account data
                    open_dtc_server::exchanges::coinbase::CoinbaseRestClient rest_client(credentials);
                    rest_client.set_sandbox_mode(false); // Use production API

                    std::vector<open_dtc_server::exchanges::coinbase::AccountBalance> accounts;
                    if (rest_client.get_accounts(accounts))
                    {
                        std::cout << "[SUCCESS] Retrieved " + std::to_string(accounts.size()) + " account balances from Coinbase" << std::endl;

                        // Send real account data to client via DTC PositionUpdate messages
                        for (const auto &account : accounts)
                        {
                            double balance = std::stod(account.total_balance);
                            if (balance > 0.0) // Only send non-zero balances
                            {
                                std::cout << "[REAL DATA] " + account.currency + ": " + account.total_balance +
                                                 " (Available: " + account.available + ", Hold: " + account.hold + ")"
                                          << std::endl;

                                // Send DTC PositionUpdate message to client
                                send_position_update_to_client(client, account.currency, account.total_balance, account.available);
                            }
                        }

                        std::cout << "[SUCCESS] Sent real Coinbase account data to client via DTC protocol" << std::endl;
                    }
                    else
                    {
                        std::cout << "[ERROR] Failed to fetch account data from Coinbase: " + rest_client.get_last_error() << std::endl;
                        std::cout << "[ERROR] Account data request failed - API error" << std::endl;
                    }
                }
                catch (const std::exception &e)
                {
                    std::cout << "[ERROR] Exception fetching account data: " + std::string(e.what()) << std::endl;
                    std::cout << "[ERROR] Account data request failed - exception occurred" << std::endl;
                }
            }

            void DTCServer::send_position_update_to_client(std::shared_ptr<ClientConnection> client,
                                                           const std::string &currency, const std::string &total_balance, const std::string &available)
            {
                try
                {
                    // Use currency directly from Coinbase API - no mapping
                    // Client decides which symbols to request via SecurityDefinitionRequest
                    std::string symbol = currency;

                    double quantity = std::stod(total_balance);
                    double available_amount = std::stod(available);

                    std::cout << "[DTC] Sending PositionUpdate to client for " + symbol + ": " + std::to_string(quantity) << std::endl;

                    // Create and send real DTC PositionUpdate message
                    open_dtc_server::core::dtc::PositionUpdate position_update;
                    position_update.trade_account = "COINBASE";
                    position_update.symbol = symbol;
                    position_update.quantity = quantity;
                    position_update.average_price = 0.0; // Not provided by Coinbase accounts API
                    position_update.position_identifier = currency;

                    // Serialize and send the DTC message
                    auto message_data = position_update.serialize();
                    client->send_message(message_data);

                    std::cout << "[CLIENT MESSAGE] Sent DTC PositionUpdate for " + symbol + ": " + std::to_string(quantity) + " (Available: " + std::to_string(available_amount) + ")" << std::endl;
                }
                catch (const std::exception &e)
                {
                    std::cout << "[ERROR] Failed to send position update: " + std::string(e.what()) << std::endl;
                }
            }

        } // namespace server
    } // namespace core
} // namespace coinbase_dtc_core
