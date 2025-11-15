#include "coinbase_dtc_core/core/server/server.hpp"
#include "coinbase_dtc_core/core/util/log.hpp"
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
                open_dtc_server::util::log("DTCServer initialized with config: " + config_.server_name);
            }

            DTCServer::~DTCServer()
            {
                stop();
                open_dtc_server::util::log("DTCServer destroyed");
            }

            bool DTCServer::start()
            {
                if (server_running_)
                {
                    open_dtc_server::util::log("Server is already running");
                    return false;
                }

                // Initialize sockets
                if (!initialize_sockets())
                {
                    open_dtc_server::util::log("Failed to initialize sockets");
                    return false;
                }

                // Create server socket
                if (!create_server_socket())
                {
                    open_dtc_server::util::log("Failed to create server socket");
                    cleanup_sockets();
                    return false;
                }

                // Start server
                server_running_ = true;
                should_shutdown_ = false;
                server_start_time_ = std::chrono::steady_clock::now();

                // Start server thread
                server_thread_ = std::thread(&DTCServer::server_thread_function, this);

                open_dtc_server::util::log("DTC Server started successfully on port " + std::to_string(config_.port));
                return true;
            }

            void DTCServer::stop()
            {
                if (!server_running_)
                {
                    return;
                }

                open_dtc_server::util::log("Stopping DTC Server...");

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

                open_dtc_server::util::log("DTC Server stopped");
            }

            bool DTCServer::add_exchange(const open_dtc_server::exchanges::base::ExchangeConfig &exchange_config)
            {
                open_dtc_server::util::log("Adding exchange: " + exchange_config.name);
                // TODO: Implement exchange addition
                return true;
            }

            bool DTCServer::remove_exchange(const std::string &exchange_name)
            {
                open_dtc_server::util::log("Removing exchange: " + exchange_name);
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
                open_dtc_server::util::log("Subscribing to symbol: " + symbol + " on exchange: " + exchange);
                // TODO: Implement symbol subscription
                return true;
            }

            bool DTCServer::unsubscribe_symbol(const std::string &symbol, const std::string &exchange)
            {
                open_dtc_server::util::log("Unsubscribing from symbol: " + symbol + " on exchange: " + exchange);
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
                    open_dtc_server::util::log("WSAStartup failed: " + std::to_string(result));
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
                    open_dtc_server::util::log("Failed to create socket");
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
                    open_dtc_server::util::log("Failed to set socket options");
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
                        open_dtc_server::util::log("Invalid bind address: " + config_.bind_address);
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
                    open_dtc_server::util::log("Failed to bind socket to port " + std::to_string(config_.port));
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
                    open_dtc_server::util::log("Failed to listen on socket");
                    close_server_socket();
                    return false;
                }

                open_dtc_server::util::log("Server socket listening on " + config_.bind_address + ":" + std::to_string(config_.port));
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
                open_dtc_server::util::log("Server thread started, accepting connections...");

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
                            open_dtc_server::util::log("Accept failed: " + std::to_string(WSAGetLastError()));
                        }
                        break;
                    }
#else
                    int client_socket = accept(server_socket_, (sockaddr *)&client_addr, &client_len);
                    if (client_socket < 0)
                    {
                        if (server_running_)
                        {
                            open_dtc_server::util::log("Accept failed");
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

                    open_dtc_server::util::log("New client connection from " + std::string(client_ip) + " (ID: " + std::to_string(client_id) + ")");

                    // Add to client list
                    add_client(client);

                    // Start client handler thread
                    std::thread client_thread(&DTCServer::client_handler_thread, this, client);
                    client_thread.detach();
                }

                open_dtc_server::util::log("Server thread ending");
            }

            void DTCServer::client_handler_thread(std::shared_ptr<ClientConnection> client)
            {
                open_dtc_server::util::log("Client handler thread started for client " + std::to_string(client->get_client_id()));

                // Simple echo server for now - just accept the connection
                while (server_running_ && !should_shutdown_)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }

                // Client disconnected or server shutdown
                remove_client(client);
                open_dtc_server::util::log("Client " + std::to_string(client->get_client_id()) + " disconnected");
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

        } // namespace server
    } // namespace core
} // namespace coinbase_dtc_core