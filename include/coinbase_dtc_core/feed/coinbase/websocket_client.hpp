#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <memory>

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

#include "../../util/log.hpp"

namespace coinbase_dtc_core {
namespace feed {
namespace coinbase {

// Coinbase market data structures
struct TradeData {
    std::string product_id;     // "BTC-USD"
    double price;
    double size;
    std::string side;           // "buy" or "sell"
    uint64_t timestamp;         // Unix timestamp in microseconds
};

struct Level2Data {
    std::string product_id;     // "BTC-USD"
    double bid_price;
    double bid_size;
    double ask_price;
    double ask_size;
    uint64_t timestamp;
};

// Callback function types
using TradeCallback = std::function<void(const TradeData&)>;
using Level2Callback = std::function<void(const Level2Data&)>;

class WebSocketClient {
public:
    WebSocketClient();
    ~WebSocketClient();
    
    // Connection management
    bool connect(const std::string& host = "ws-feed.exchange.coinbase.com", uint16_t port = 443);
    void disconnect();
    bool is_connected() const { return connected_.load(); }
    
    // Subscription management
    bool subscribe_trades(const std::string& product_id = "BTC-USD");
    bool subscribe_level2(const std::string& product_id = "BTC-USD");
    bool subscribe_multiple_symbols(const std::vector<std::string>& product_ids);
    bool unsubscribe(const std::string& product_id = "BTC-USD");
    std::vector<std::string> get_subscribed_symbols() const;
    
    // Callback registration
    void set_trade_callback(TradeCallback callback) { 
        std::lock_guard<std::mutex> lock(callback_mutex_);
        trade_callback_ = callback; 
    }
    
    void set_level2_callback(Level2Callback callback) { 
        std::lock_guard<std::mutex> lock(callback_mutex_);
        level2_callback_ = callback; 
    }
    
    // Status
    std::string get_status() const;
    
private:
    // Connection state
    std::atomic<bool> connected_;
    std::atomic<bool> should_stop_;
    int socket_;
    std::string host_;
    uint16_t port_;
    
    // Threading
    std::thread worker_thread_;
    std::thread ping_thread_;
    
    // Callbacks
    std::mutex callback_mutex_;
    TradeCallback trade_callback_;
    Level2Callback level2_callback_;
    
    // Message queues
    std::mutex send_queue_mutex_;
    std::queue<std::string> send_queue_;
    
    // Subscription tracking
    std::mutex subscriptions_mutex_;
    std::vector<std::string> subscribed_symbols_;
    
    // Statistics
    std::atomic<uint64_t> messages_received_;
    std::atomic<uint64_t> messages_sent_;
    std::atomic<uint64_t> last_message_time_;
    
    // Internal methods
    void worker_loop();
    void ping_loop();
    bool send_websocket_message(const std::string& message);
    void process_received_message(const std::string& message);
    void parse_trade_message(const std::string& json);
    void parse_level2_message(const std::string& json);
    
    // WebSocket protocol helpers
    std::string create_websocket_handshake() const;
    bool perform_websocket_handshake();
    std::string encode_websocket_frame(const std::string& payload) const;
    std::string decode_websocket_frame(const uint8_t* data, size_t length) const;
    
    // JSON utilities
    std::string create_subscribe_message(const std::string& channel, const std::string& product_id) const;
    std::string create_unsubscribe_message(const std::string& product_id) const;
    
    // Utility methods
    uint64_t get_current_timestamp() const;
    void cleanup_socket();
};

} // namespace coinbase
} // namespace feed
} // namespace coinbase_dtc_core