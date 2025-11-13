#include "coinbase_dtc_core/dtc/protocol.hpp"
#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <unordered_map>

// Symbol information for better logging
struct ClientSymbolInfo {
    std::string symbol;
    std::string display_name;
    std::string exchange;
};

// Known symbols mapping (should match server's SymbolManager)
static std::unordered_map<uint32_t, ClientSymbolInfo> symbol_map = {
    {1, {"STRK-USDC", "Starknet/USDC", "coinbase"}},
    {2, {"USDC-EUR", "USDC/EUR", "coinbase"}},
    {3, {"SOL-USDC", "Solana/USDC", "coinbase"}},
    {4, {"BTC-USDC", "Bitcoin/USDC", "coinbase"}},
    {5, {"ETH-USDC", "Ethereum/USDC", "coinbase"}},
    {6, {"LTC-USDC", "Litecoin/USDC", "coinbase"}},
    {7, {"LINK-USDC", "Chainlink/USDC", "coinbase"}},
    {8, {"XRP-USDC", "XRP/USDC", "coinbase"}},
    {9, {"ADA-USDC", "Cardano/USDC", "coinbase"}}
};

// Helper function to get symbol info
static std::string get_symbol_info(uint32_t symbol_id) {
    auto it = symbol_map.find(symbol_id);
    if (it != symbol_map.end()) {
        return it->second.symbol + " [" + it->second.exchange + "]";
    }
    return "Unknown Symbol " + std::to_string(symbol_id);
}

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define closesocket close
#endif

class DTCTestClient {
public:
    DTCTestClient(const std::string& host = "127.0.0.1", uint16_t port = 11099)
        : host_(host), port_(port), socket_(-1) {
        
        #ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        #endif
    }
    
    ~DTCTestClient() {
        disconnect();
        #ifdef _WIN32
        WSACleanup();
        #endif
    }
    
    bool connect() {
        socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_ == -1) {
            std::cout << "[ERROR] Failed to create socket" << std::endl;
            return false;
        }
        
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);
        
        if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
            std::cout << "[ERROR] Invalid address: " << host_ << std::endl;
            return false;
        }
        
        if (::connect(socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cout << "[ERROR] Connection failed to " << host_ << ":" << port_ << std::endl;
            return false;
        }
        
        // Set socket timeout for recv operations
        #ifdef _WIN32
        DWORD timeout = 1000; // 1 second timeout
        setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
        #else
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        #endif
        
        std::cout << "[OK] Connected to DTC server at " << host_ << ":" << port_ << std::endl;
        return true;
    }
    
    void disconnect() {
        if (socket_ != -1) {
            closesocket(socket_);
            socket_ = -1;
        }
    }
    
    bool send_logon_request(const std::string& username, const std::string& password = "testpass") {
        using namespace coinbase_dtc_core::dtc;
        
        LogonRequest logon_req;
        strncpy(logon_req.username, username.c_str(), sizeof(logon_req.username) - 1);
        strncpy(logon_req.password, password.c_str(), sizeof(logon_req.password) - 1);
        strncpy(logon_req.general_text_data, "DTC Test Client v1.0", sizeof(logon_req.general_text_data) - 1);
        
        auto data = logon_req.serialize();
        
        int sent = send(socket_, reinterpret_cast<const char*>(data.data()), 
                       static_cast<int>(data.size()), 0);
        
        if (sent == static_cast<int>(data.size())) {
            std::cout << "[SENT] Logon request for user: " << username << std::endl;
            return true;
        } else {
            std::cout << "[ERROR] Failed to send logon request" << std::endl;
            return false;
        }
    }
    
    bool send_market_data_request(const std::string& symbol) {
        using namespace coinbase_dtc_core::dtc;
        
        MarketDataRequest md_req;
        md_req.symbol_id = 1;
        md_req.request_action = 1; // Subscribe
        strncpy(md_req.symbol, symbol.c_str(), sizeof(md_req.symbol) - 1);
        
        auto data = md_req.serialize();
        
        int sent = send(socket_, reinterpret_cast<const char*>(data.data()), 
                       static_cast<int>(data.size()), 0);
        
        if (sent == static_cast<int>(data.size())) {
            std::cout << "[SENT] Market data request for: " << symbol << std::endl;
            return true;
        } else {
            std::cout << "[ERROR] Failed to send market data request" << std::endl;
            return false;
        }
    }
    
    bool receive_messages(int timeout_seconds = 30) {
        using namespace coinbase_dtc_core::dtc;
        
        Protocol protocol;
        std::vector<uint8_t> buffer(8192);
        
        std::cout << "[LISTEN] Listening for messages (timeout: " << timeout_seconds << "s)..." << std::endl;
        
        // Set socket timeout
        #ifdef _WIN32
        DWORD timeout = timeout_seconds * 1000;
        setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        #else
        struct timeval tv;
        tv.tv_sec = timeout_seconds;
        tv.tv_usec = 0;
        setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        #endif
        
        int messages_received = 0;
        time_t start_time = time(nullptr);
        
        while (time(nullptr) - start_time < timeout_seconds) {
            int bytes_received = recv(socket_, reinterpret_cast<char*>(buffer.data()), 
                                    static_cast<int>(buffer.size()), 0);
            
            if (bytes_received > 0) {
                std::cout << "[DEBUG] Received " << bytes_received << " bytes from server" << std::endl;
                
                // Debug: Print raw bytes
                std::cout << "[DEBUG] Raw data: ";
                int max_bytes = (bytes_received < 16) ? bytes_received : 16;
                for (int i = 0; i < max_bytes; ++i) {
                    printf("%02X ", buffer[i]);
                }
                std::cout << std::endl;
                
                auto message = protocol.parse_message(buffer.data(), static_cast<uint16_t>(bytes_received));
                
                if (message) {
                    messages_received++;
                    auto type = message->get_type();
                    std::cout << "[RECV] Successfully parsed message type: " << static_cast<int>(type) << std::endl;
                    
                    switch (type) {
                        case MessageType::LOGON_RESPONSE: {
                            auto* response = static_cast<LogonResponse*>(message.get());
                            std::cout << "[RECV] Logon Response: " 
                                     << (response->result ? "[OK] Success" : "[ERROR] Failed") 
                                     << " - " << response->result_text << std::endl;
                            break;
                        }
                        case MessageType::MARKET_DATA_UPDATE_TRADE: {
                            auto* trade = static_cast<MarketDataUpdateTrade*>(message.get());
                            std::cout << "[TRADE] Trade Update: " << get_symbol_info(trade->symbol_id)
                                     << " - Price: $" << trade->price 
                                     << " Vol: " << trade->volume << std::endl;
                            break;
                        }
                        case MessageType::MARKET_DATA_UPDATE_BID_ASK: {
                            auto* book = static_cast<MarketDataUpdateBidAsk*>(message.get());
                            std::cout << "[BOOK] OrderBook: " << get_symbol_info(book->symbol_id)
                                     << " - Bid: $" << book->bid_price << " x " << book->bid_quantity
                                     << " | Ask: $" << book->ask_price << " x " << book->ask_quantity 
                                     << std::endl;
                            break;
                        }
                        default:
                            std::cout << "[RECV] Unknown message type: " << static_cast<int>(type) << std::endl;
                            break;
                    }
                } else {
                    std::cout << "[DEBUG] Failed to parse message (" << bytes_received << " bytes)" << std::endl;
                }
            } else if (bytes_received == 0) {
                std::cout << "[DISC] Server closed connection" << std::endl;
                break;
            } else {
                // Check for timeout or error
                #ifdef _WIN32
                int error = WSAGetLastError();
                if (error == WSAETIMEDOUT || error == WSAEWOULDBLOCK) {
                    std::cout << "[DEBUG] Timeout waiting for data..." << std::endl;
                    continue;
                }
                #else
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    std::cout << "[DEBUG] Timeout waiting for data..." << std::endl;
                    continue;
                }
                #endif
                std::cout << "[ERROR] Receive error" << std::endl;
                break;
            }
        }
        
        std::cout << "[OK] Received " << messages_received << " messages total" << std::endl;
        return messages_received > 0;
    }
    
private:
    std::string host_;
    uint16_t port_;
    int socket_;
};

int main() {
    std::cout << "[TEST] DTC Test Client Starting..." << std::endl;
    
    DTCTestClient client;
    
    if (!client.connect()) {
        return 1;
    }
    
    // Test 1: Send logon request
    if (!client.send_logon_request("testuser", "testpass")) {
        return 1;
    }
    
    // Test 2: Send market data request
    if (!client.send_market_data_request("BTC-USD")) {
        return 1;
    }
    
    // Test 3: Listen for responses and market data
    client.receive_messages(15);
    
    std::cout << "[OK] DTC Test Client completed successfully!" << std::endl;
    return 0;
}