#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace coinbase_dtc_core {
namespace dtc {

// DTC Protocol Version
constexpr uint16_t DTC_PROTOCOL_VERSION = 8;

// Core DTC Message Types (from DTC specification)
enum class MessageType : uint16_t {
    // Logon/Connection Messages
    LOGON_REQUEST = 1,
    LOGON_RESPONSE = 2,
    HEARTBEAT = 3,
    LOGOFF = 5,
    
    // Market Data Messages  
    MARKET_DATA_REQUEST = 101,
    MARKET_DATA_REJECT = 103,
    MARKET_DATA_SNAPSHOT = 104,
    MARKET_DATA_UPDATE_TRADE = 107,
    MARKET_DATA_UPDATE_TRADE_COMPACT = 112,
    MARKET_DATA_UPDATE_LAST_TRADE_SNAPSHOT = 134,
    MARKET_DATA_UPDATE_BID_ASK = 108,
    MARKET_DATA_UPDATE_BID_ASK_COMPACT = 117,
    
    // Trading Messages
    SUBMIT_NEW_SINGLE_ORDER = 208,
    ORDER_UPDATE = 210,
    OPEN_ORDERS_REQUEST = 300,
    OPEN_ORDERS_REJECT = 302,
    CURRENT_POSITIONS_REQUEST = 400,
    CURRENT_POSITIONS_REJECT = 402,
    
    // System Messages
    SECURITY_DEFINITION_FOR_SYMBOL_REQUEST = 501,
    SECURITY_DEFINITION_RESPONSE = 502
};

// DTC Message Header (all messages start with this)
#pragma pack(push, 1)
struct MessageHeader {
    uint16_t size;        // Total message size including header
    uint16_t type;        // MessageType enum value
    
    MessageHeader() : size(0), type(0) {}
    MessageHeader(uint16_t msg_size, MessageType msg_type) 
        : size(msg_size), type(static_cast<uint16_t>(msg_type)) {}
};
#pragma pack(pop)

// Base class for all DTC messages
class DTCMessage {
public:
    virtual ~DTCMessage() = default;
    virtual MessageType get_type() const = 0;
    virtual uint16_t get_size() const = 0;
    virtual std::vector<uint8_t> serialize() const = 0;
    virtual bool deserialize(const uint8_t* data, uint16_t size) = 0;
    
    // Helper to get message header
    MessageHeader get_header() const {
        return MessageHeader(get_size(), get_type());
    }
};

// Logon Request Message
#pragma pack(push, 1)
struct LogonRequest : public DTCMessage {
    MessageHeader header;
    uint32_t protocol_version;
    char username[64];
    char password[64];
    char general_text_data[128];
    
    LogonRequest() : protocol_version(DTC_PROTOCOL_VERSION) {
        // Calculate actual data size without vtable
        uint16_t data_size = sizeof(MessageHeader) + sizeof(protocol_version) + 
                            sizeof(username) + sizeof(password) + sizeof(general_text_data);
        header = MessageHeader(data_size, MessageType::LOGON_REQUEST);
        memset(username, 0, sizeof(username));
        memset(password, 0, sizeof(password));
        memset(general_text_data, 0, sizeof(general_text_data));
    }
    
    MessageType get_type() const override { return MessageType::LOGON_REQUEST; }
    uint16_t get_size() const override { 
        return sizeof(MessageHeader) + sizeof(protocol_version) + 
               sizeof(username) + sizeof(password) + sizeof(general_text_data);
    }
    std::vector<uint8_t> serialize() const override;
    bool deserialize(const uint8_t* data, uint16_t size) override;
};
#pragma pack(pop)

// Logon Response Message
#pragma pack(push, 1)
struct LogonResponse : public DTCMessage {
    MessageHeader header;
    uint32_t protocol_version;
    uint32_t result;  // 1 = success, 0 = failure
    char result_text[96];
    char reconnect_address[64];
    uint32_t integer_to_float_price_divisor;
    
    LogonResponse() : protocol_version(DTC_PROTOCOL_VERSION), result(0), integer_to_float_price_divisor(1) {
        uint16_t data_size = sizeof(MessageHeader) + sizeof(protocol_version) + sizeof(result) + 
                            sizeof(result_text) + sizeof(reconnect_address) + sizeof(integer_to_float_price_divisor);
        header = MessageHeader(data_size, MessageType::LOGON_RESPONSE);
        memset(result_text, 0, sizeof(result_text));
        memset(reconnect_address, 0, sizeof(reconnect_address));
    }
    
    MessageType get_type() const override { return MessageType::LOGON_RESPONSE; }
    uint16_t get_size() const override { 
        return sizeof(MessageHeader) + sizeof(protocol_version) + sizeof(result) + 
               sizeof(result_text) + sizeof(reconnect_address) + sizeof(integer_to_float_price_divisor);
    }
    std::vector<uint8_t> serialize() const override;
    bool deserialize(const uint8_t* data, uint16_t size) override;
};
#pragma pack(pop)

// Market Data Request Message
#pragma pack(push, 1) 
struct MarketDataRequest : public DTCMessage {
    MessageHeader header;
    uint32_t request_action;  // 1 = subscribe, 2 = unsubscribe
    uint32_t symbol_id;
    char symbol[64];
    
    MarketDataRequest() : request_action(1), symbol_id(0) {
        uint16_t data_size = sizeof(MessageHeader) + sizeof(request_action) + 
                            sizeof(symbol_id) + sizeof(symbol);
        header = MessageHeader(data_size, MessageType::MARKET_DATA_REQUEST);
        memset(symbol, 0, sizeof(symbol));
    }
    
    MessageType get_type() const override { return MessageType::MARKET_DATA_REQUEST; }
    uint16_t get_size() const override { 
        return sizeof(MessageHeader) + sizeof(request_action) + 
               sizeof(symbol_id) + sizeof(symbol);
    }
    std::vector<uint8_t> serialize() const override;
    bool deserialize(const uint8_t* data, uint16_t size) override;
};
#pragma pack(pop)

// Market Data Update Trade Message
#pragma pack(push, 1)
struct MarketDataUpdateTrade : public DTCMessage {
    MessageHeader header;
    uint32_t symbol_id;
    double price;
    double volume;
    uint64_t date_time;  // Unix timestamp
    
    MarketDataUpdateTrade() : symbol_id(0), price(0.0), volume(0.0), date_time(0) {
        uint16_t data_size = sizeof(MessageHeader) + sizeof(symbol_id) + 
                            sizeof(price) + sizeof(volume) + sizeof(date_time);
        header = MessageHeader(data_size, MessageType::MARKET_DATA_UPDATE_TRADE);
    }
    
    MessageType get_type() const override { return MessageType::MARKET_DATA_UPDATE_TRADE; }
    uint16_t get_size() const override { 
        return sizeof(MessageHeader) + sizeof(symbol_id) + 
               sizeof(price) + sizeof(volume) + sizeof(date_time);
    }
    std::vector<uint8_t> serialize() const override;
    bool deserialize(const uint8_t* data, uint16_t size) override;
};
#pragma pack(pop)

// Market Data Update Bid/Ask Message
#pragma pack(push, 1)
struct MarketDataUpdateBidAsk : public DTCMessage {
    MessageHeader header;
    uint32_t symbol_id;
    double bid_price;
    double bid_quantity;
    double ask_price;
    double ask_quantity;
    uint64_t date_time;
    
    MarketDataUpdateBidAsk() : symbol_id(0), bid_price(0.0), bid_quantity(0.0), 
                              ask_price(0.0), ask_quantity(0.0), date_time(0) {
        uint16_t data_size = sizeof(MessageHeader) + sizeof(symbol_id) + 
                            sizeof(bid_price) + sizeof(bid_quantity) + 
                            sizeof(ask_price) + sizeof(ask_quantity) + sizeof(date_time);
        header = MessageHeader(data_size, MessageType::MARKET_DATA_UPDATE_BID_ASK);
    }
    
    MessageType get_type() const override { return MessageType::MARKET_DATA_UPDATE_BID_ASK; }
    uint16_t get_size() const override { 
        return sizeof(MessageHeader) + sizeof(symbol_id) + 
               sizeof(bid_price) + sizeof(bid_quantity) + 
               sizeof(ask_price) + sizeof(ask_quantity) + sizeof(date_time);
    }
    std::vector<uint8_t> serialize() const override;
    bool deserialize(const uint8_t* data, uint16_t size) override;
};
#pragma pack(pop)

// Protocol Handler Class
class Protocol {
private:
    uint16_t protocol_version_;
    bool is_connected_;
    std::string client_info_;

public:
    Protocol();
    ~Protocol() = default;
    
    // Version management
    std::string version() const;
    uint16_t get_protocol_version() const { return protocol_version_; }
    
    // Connection management
    bool is_connected() const { return is_connected_; }
    void set_connected(bool connected) { is_connected_ = connected; }
    
    // Message processing
    std::unique_ptr<DTCMessage> parse_message(const uint8_t* data, uint16_t size);
    std::vector<uint8_t> create_message(const DTCMessage& message);
    
    // Protocol helpers
    std::unique_ptr<LogonResponse> create_logon_response(bool success, const std::string& message = "");
    std::unique_ptr<MarketDataUpdateTrade> create_trade_update(
        uint32_t symbol_id, double price, double volume, uint64_t timestamp);
    std::unique_ptr<MarketDataUpdateBidAsk> create_bid_ask_update(
        uint32_t symbol_id, double bid_price, double bid_qty, 
        double ask_price, double ask_qty, uint64_t timestamp);
    
    // Utility functions
    static uint64_t get_current_timestamp();
    static MessageType get_message_type(const uint8_t* data, uint16_t size);
    static bool validate_message_header(const uint8_t* data, uint16_t size);
};

} // namespace dtc
} // namespace coinbase_dtc_core
