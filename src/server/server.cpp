#include "coinbase_dtc_core/server/server.hpp"
#include "coinbase_dtc_core/util/log.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <cstdio>

#ifdef _WIN32
#define CLOSE_SOCKET closesocket
#else
#define CLOSE_SOCKET close
#endif

namespace coinbase_dtc_core {
namespace server {

// ClientSession Implementation
ClientSession::ClientSession(int socket, const std::string& remote_addr)
    : socket_(socket), remote_addr_(remote_addr), state_(ClientState::CONNECTED),
      connection_time_(Server::get_current_timestamp()) {
    
    // Keep socket as blocking for simpler message handling
    // Note: For production, you'd want proper async I/O or message buffering
    util::log("[CONN] New client session created from " + remote_addr_);
}

ClientSession::~ClientSession() {
    if (socket_ != -1) {
        CLOSE_SOCKET(socket_);
        socket_ = -1;
    }
    util::log("Client session destroyed: " + username_ + " (" + remote_addr_ + ")");
}

bool ClientSession::send_message(const dtc::DTCMessage& message) {
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    auto serialized = protocol_.create_message(message);
    return send_raw_data(serialized);
}

bool ClientSession::send_raw_data(const std::vector<uint8_t>& data) {
    if (socket_ == -1 || data.empty()) return false;
    
    util::log("[SEND] Sending " + std::to_string(data.size()) + " bytes to " + remote_addr_);
    
    size_t total_sent = 0;
    while (total_sent < data.size()) {
        int sent = send(socket_, reinterpret_cast<const char*>(data.data() + total_sent), 
                       static_cast<int>(data.size() - total_sent), 0);
        
        if (sent == -1) {
            util::log("[ERROR] Send failed for client " + remote_addr_);
            return false;
        }
        total_sent += sent;
    }
    
    util::log("[OK] Sent " + std::to_string(total_sent) + " bytes to " + remote_addr_);
    return true;
}

// Server Implementation
Server::Server(const ServerConfig& config) 
    : config_(config), running_(false), shutdown_requested_(false), server_socket_(-1) {
    
    #ifdef _WIN32
    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        util::log("WSAStartup failed: " + std::to_string(result));
        throw std::runtime_error("Failed to initialize Winsock");
    }
    #endif
    
    util::log("DTC Server initialized on port " + std::to_string(config_.port));
}

Server::~Server() {
    stop();
    
    #ifdef _WIN32
    WSACleanup();
    #endif
}

bool Server::start() {
    if (running_) {
        util::log("Server is already running");
        return false;
    }
    
    if (!init_socket()) {
        util::log("Failed to initialize server socket");
        return false;
    }
    
    // Start listening
    if (listen(server_socket_, config_.max_clients) == -1) {
        util::log("Failed to listen on socket");
        cleanup_socket();
        return false;
    }
    
    running_ = true;
    shutdown_requested_ = false;
    
    // Start accept thread
    accept_thread_ = std::thread(&Server::accept_loop, this);
    
    util::log("DTC Server started successfully on port " + std::to_string(config_.port));
    return true;
}

void Server::stop() {
    if (!running_) return;
    
    util::log("Stopping DTC Server...");
    shutdown_requested_ = true;
    
    // Disconnect all clients
    disconnect_all_clients();
    
    // Stop accepting new connections
    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }
    
    // Wait for client threads to finish
    for (auto& thread : client_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    client_threads_.clear();
    
    // Cleanup
    cleanup_socket();
    running_ = false;
    
    util::log("DTC Server stopped");
}

std::string Server::status() const {
    std::ostringstream oss;
    oss << "DTC Server Status:\n";
    oss << "  Running: " << (running_ ? "Yes" : "No") << "\n";
    oss << "  Port: " << config_.port << "\n";
    oss << "  Connected Clients: " << get_client_count() << "/" << config_.max_clients << "\n";
    oss << "  Local IP: " << get_local_ip() << "\n";
    return oss.str();
}

bool Server::init_socket() {
    // Create socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ == -1) {
        util::log("Failed to create socket");
        return false;
    }
    
    // Set socket options
    if (!set_socket_options(server_socket_)) {
        cleanup_socket();
        return false;
    }
    
    // Setup server address
    memset(&server_addr_, 0, sizeof(server_addr_));
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_addr.s_addr = INADDR_ANY;
    server_addr_.sin_port = htons(config_.port);
    
    // Bind socket
    if (bind(server_socket_, reinterpret_cast<struct sockaddr*>(&server_addr_), 
             sizeof(server_addr_)) == -1) {
        util::log("Failed to bind socket to port " + std::to_string(config_.port));
        cleanup_socket();
        return false;
    }
    
    return true;
}

void Server::cleanup_socket() {
    if (server_socket_ != -1) {
        CLOSE_SOCKET(server_socket_);
        server_socket_ = -1;
    }
}

bool Server::set_socket_options(int socket) {
    // Enable address reuse
    int opt = 1;
    if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, 
                   reinterpret_cast<char*>(&opt), sizeof(opt)) == -1) {
        util::log("Failed to set SO_REUSEADDR");
        return false;
    }
    
    #ifdef SO_REUSEPORT
    if (setsockopt(socket, SOL_SOCKET, SO_REUSEPORT, 
                   reinterpret_cast<char*>(&opt), sizeof(opt)) == -1) {
        // Not critical if this fails
        util::log("Warning: Failed to set SO_REUSEPORT");
    }
    #endif
    
    return true;
}

void Server::accept_loop() {
    util::log("Accept loop started");
    
    while (running_ && !shutdown_requested_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket_, 
                                  reinterpret_cast<struct sockaddr*>(&client_addr), 
                                  &client_len);
        
        if (client_socket == -1) {
            if (!shutdown_requested_) {
                util::log("Accept failed");
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            continue;
        }
        
        // Check client limit
        if (get_client_count() >= config_.max_clients) {
            util::log("Client limit reached, rejecting connection");
            CLOSE_SOCKET(client_socket);
            continue;
        }
        
        // Get client address
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        std::string remote_addr = std::string(client_ip) + ":" + std::to_string(ntohs(client_addr.sin_port));
        
        // Create client session
        auto client = std::make_shared<ClientSession>(client_socket, remote_addr);
        add_client(client);
        
        // Start client handler thread
        client_threads_.emplace_back(&Server::handle_client, this, client);
        
        util::log("Accepted connection from " + remote_addr);
        
        // Call connection handler
        if (connection_handler_) {
            connection_handler_(client);
        }
    }
    
    util::log("Accept loop ended");
}

void Server::handle_client(std::shared_ptr<ClientSession> client) {
    util::log("Client handler started for " + client->get_remote_address());
    
    while (running_ && client->is_connected() && !shutdown_requested_) {
        if (!process_client_messages(client)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Cleanup
    client->set_state(ClientState::DISCONNECTED);
    remove_client(client);
    
    if (disconnection_handler_) {
        disconnection_handler_(client);
    }
    
    util::log("Client handler ended for " + client->get_remote_address());
}

bool Server::process_client_messages(std::shared_ptr<ClientSession> client) {
    std::vector<uint8_t> buffer(config_.buffer_size);
    
    int bytes_received = recv(client->get_socket(), 
                             reinterpret_cast<char*>(buffer.data()), 
                             static_cast<int>(buffer.size()), 0);
    
    if (bytes_received == -1) {
        util::log("[ERROR] Receive failed for client " + client->get_remote_address());
        return false;
    }
    
    if (bytes_received == 0) {
        util::log("[DISC] Client disconnected: " + client->get_remote_address());
        return false;
    }
    
    // Debug: Log received data
    util::log("[RECV] Received " + std::to_string(bytes_received) + " bytes from " + client->get_remote_address());
    
    // Parse DTC messages
    auto message = protocol_.parse_message(buffer.data(), static_cast<uint16_t>(bytes_received));
    if (message) {
        util::log("[PARSE] Successfully parsed message type: " + std::to_string(static_cast<int>(message->get_type())));
        
        // Handle message based on type
        auto type = message->get_type();
        
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        auto handler_it = message_handlers_.find(type);
        if (handler_it != message_handlers_.end()) {
            handler_it->second(client, std::move(message));
        } else {
            // Default message handling
            switch (type) {
                case dtc::MessageType::LOGON_REQUEST:
                    handle_logon_request(client, std::move(message));
                    break;
                case dtc::MessageType::MARKET_DATA_REQUEST:
                    handle_market_data_request(client, std::move(message));
                    break;
                default:
                    util::log("[WARNING] Unhandled message type: " + std::to_string(static_cast<int>(type)));
                    break;
            }
        }
    } else {
        util::log("[ERROR] Failed to parse message from " + client->get_remote_address() + 
                 " (" + std::to_string(bytes_received) + " bytes)");
        
        // Debug: Show hex dump of first few bytes
        std::string hex_dump = "Raw data: ";
        int dump_size = (bytes_received < 16) ? bytes_received : 16;
        for (int i = 0; i < dump_size; ++i) {
            char hex_str[4];
            sprintf(hex_str, "%02X ", buffer[i]);
            hex_dump += hex_str;
        }
        util::log(hex_dump);
    }
    
    return true;
}

void Server::handle_logon_request(std::shared_ptr<ClientSession> client, std::unique_ptr<dtc::DTCMessage> message) {
    auto* logon_req = static_cast<dtc::LogonRequest*>(message.get());
    
    std::string username = logon_req->username;
    std::string password = logon_req->password;
    
    util::log("[AUTH] Logon request from " + client->get_remote_address() + " - Username: '" + username + "'");
    
    // Simple authentication (extend this for real authentication)
    bool auth_success = !username.empty(); // Accept any non-empty username
    
    if (auth_success) {
        client->set_username(username);
        client->set_state(ClientState::AUTHENTICATED);
        util::log("[OK] Authentication successful for user: " + username);
    } else {
        util::log("[ERROR] Authentication failed for user: " + username);
    }
    
    // Send logon response
    auto response = protocol_.create_logon_response(auth_success, 
        auth_success ? "Authentication successful" : "Authentication failed");
        
    util::log("[SEND] Sending logon response to " + username + " - Success: " + (auth_success ? "Yes" : "No"));
    
    if (!client->send_message(*response)) {
        util::log("[ERROR] Failed to send logon response to " + client->get_remote_address());
    }
}

void Server::handle_market_data_request(std::shared_ptr<ClientSession> client, std::unique_ptr<dtc::DTCMessage> message) {
    if (!client->is_authenticated()) {
        util::log("[ERROR] Market data request from unauthenticated client: " + client->get_remote_address());
        return;
    }
    
    auto* md_req = static_cast<dtc::MarketDataRequest*>(message.get());
    
    util::log("[MARKET] Market data request for symbol: '" + std::string(md_req->symbol) + 
              "' from user: " + client->get_username());
    
    client->set_state(ClientState::SUBSCRIBED);
    
    // Here you would typically store the subscription and start sending market data
    // For now, we'll just acknowledge the subscription
    util::log("[OK] Client subscribed to market data: " + client->get_username());
}

// Client management functions
void Server::add_client(std::shared_ptr<ClientSession> client) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_.push_back(client);
}

void Server::remove_client(std::shared_ptr<ClientSession> client) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_.erase(
        std::remove_if(clients_.begin(), clients_.end(),
                      [client](const std::weak_ptr<ClientSession>& weak_client) {
                          auto shared_client = weak_client.lock();
                          return !shared_client || shared_client == client;
                      }),
        clients_.end()
    );
}

std::vector<std::shared_ptr<ClientSession>> Server::get_clients() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    return clients_;
}

size_t Server::get_client_count() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    return clients_.size();
}

void Server::disconnect_all_clients() {
    auto clients = get_clients();
    for (auto& client : clients) {
        client->set_state(ClientState::DISCONNECTED);
    }
}

// Broadcasting functions
void Server::broadcast_trade_update(uint32_t symbol_id, double price, double volume, uint64_t timestamp) {
    auto trade_update = protocol_.create_trade_update(symbol_id, price, volume, timestamp);
    
    auto clients = get_clients();
    for (auto& client : clients) {
        if (client->is_authenticated() && client->get_state() == ClientState::SUBSCRIBED) {
            client->send_message(*trade_update);
        }
    }
}

void Server::broadcast_bid_ask_update(uint32_t symbol_id, double bid_price, double bid_qty, 
                                      double ask_price, double ask_qty, uint64_t timestamp) {
    auto bid_ask_update = protocol_.create_bid_ask_update(symbol_id, bid_price, bid_qty, 
                                                         ask_price, ask_qty, timestamp);
    
    auto clients = get_clients();
    for (auto& client : clients) {
        if (client->is_authenticated() && client->get_state() == ClientState::SUBSCRIBED) {
            client->send_message(*bid_ask_update);
        }
    }
}

// Utility functions
std::string Server::get_local_ip() {
    // Simple implementation - returns localhost for now
    // In production, you'd want to get the actual local IP
    return "127.0.0.1";
}

uint64_t Server::get_current_timestamp() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// Handler setters
void Server::set_message_handler(dtc::MessageType type, MessageHandler handler) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    message_handlers_[type] = handler;
}

void Server::set_connection_handler(ConnectionHandler handler) {
    connection_handler_ = handler;
}

void Server::set_disconnection_handler(ConnectionHandler handler) {
    disconnection_handler_ = handler;
}

} // namespace server
} // namespace coinbase_dtc_core
