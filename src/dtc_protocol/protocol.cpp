#include "coinbase_dtc_core/dtc/protocol.hpp"
#include <cstring>
#include <chrono>
#include <iostream>

namespace coinbase_dtc_core {
namespace dtc {

// Protocol Constructor
Protocol::Protocol() 
    : protocol_version_(DTC_PROTOCOL_VERSION), is_connected_(false), client_info_("CoinbaseDTC") {
}

std::string Protocol::version() const { 
    return std::to_string(protocol_version_) + ".0.1"; 
}

// Message Serialization Implementations

std::vector<uint8_t> LogonRequest::serialize() const {
    // Create a fresh header
    MessageHeader fresh_header(sizeof(LogonRequest), MessageType::LOGON_REQUEST);
    
    std::vector<uint8_t> buffer(sizeof(LogonRequest));
    
    // Copy header
    memcpy(buffer.data(), &fresh_header, sizeof(MessageHeader));
    
    // Copy protocol version
    memcpy(buffer.data() + sizeof(MessageHeader), &protocol_version, sizeof(protocol_version));
    
    // Copy strings
    size_t offset = sizeof(MessageHeader) + sizeof(protocol_version);
    memcpy(buffer.data() + offset, username, sizeof(username));
    offset += sizeof(username);
    memcpy(buffer.data() + offset, password, sizeof(password));
    offset += sizeof(password);
    memcpy(buffer.data() + offset, general_text_data, sizeof(general_text_data));
    
    return buffer;
}

bool LogonRequest::deserialize(const uint8_t* data, uint16_t size) {
    uint16_t expected_size = sizeof(MessageHeader) + sizeof(protocol_version) + 
                            sizeof(username) + sizeof(password) + sizeof(general_text_data);
    if (size < expected_size) return false;
    
    // Safely copy field by field to avoid vtable corruption
    size_t offset = 0;
    memcpy(&header, data + offset, sizeof(header));
    offset += sizeof(header);
    
    memcpy(&protocol_version, data + offset, sizeof(protocol_version));
    offset += sizeof(protocol_version);
    
    memcpy(username, data + offset, sizeof(username));
    offset += sizeof(username);
    
    memcpy(password, data + offset, sizeof(password));
    offset += sizeof(password);
    
    memcpy(general_text_data, data + offset, sizeof(general_text_data));
    
    return true;
}

std::vector<uint8_t> LogonResponse::serialize() const {
    // Create a fresh header with correct size
    uint16_t data_size = sizeof(MessageHeader) + sizeof(protocol_version) + sizeof(result) + 
                        sizeof(result_text) + sizeof(reconnect_address) + sizeof(integer_to_float_price_divisor);
    MessageHeader fresh_header(data_size, MessageType::LOGON_RESPONSE);
    
    std::vector<uint8_t> buffer(data_size);
    
    // Copy header
    memcpy(buffer.data(), &fresh_header, sizeof(MessageHeader));
    
    // Copy fields
    size_t offset = sizeof(MessageHeader);
    memcpy(buffer.data() + offset, &protocol_version, sizeof(protocol_version));
    offset += sizeof(protocol_version);
    memcpy(buffer.data() + offset, &result, sizeof(result));
    offset += sizeof(result);
    memcpy(buffer.data() + offset, result_text, sizeof(result_text));
    offset += sizeof(result_text);
    memcpy(buffer.data() + offset, reconnect_address, sizeof(reconnect_address));
    offset += sizeof(reconnect_address);
    memcpy(buffer.data() + offset, &integer_to_float_price_divisor, sizeof(integer_to_float_price_divisor));
    
    return buffer;
}

bool LogonResponse::deserialize(const uint8_t* data, uint16_t size) {
    uint16_t expected_size = sizeof(MessageHeader) + sizeof(protocol_version) + sizeof(result) + 
                            sizeof(result_text) + sizeof(reconnect_address) + sizeof(integer_to_float_price_divisor);
    if (size < expected_size) return false;
    
    // Safely copy field by field
    size_t offset = 0;
    memcpy(&header, data + offset, sizeof(header));
    offset += sizeof(header);
    memcpy(&protocol_version, data + offset, sizeof(protocol_version));
    offset += sizeof(protocol_version);
    memcpy(&result, data + offset, sizeof(result));
    offset += sizeof(result);
    memcpy(result_text, data + offset, sizeof(result_text));
    offset += sizeof(result_text);
    memcpy(reconnect_address, data + offset, sizeof(reconnect_address));
    offset += sizeof(reconnect_address);
    memcpy(&integer_to_float_price_divisor, data + offset, sizeof(integer_to_float_price_divisor));
    
    return true;
}

std::vector<uint8_t> MarketDataRequest::serialize() const {
    // Create a fresh header
    MessageHeader fresh_header(sizeof(MarketDataRequest), MessageType::MARKET_DATA_REQUEST);
    
    std::vector<uint8_t> buffer(sizeof(MarketDataRequest));
    
    // Copy header
    memcpy(buffer.data(), &fresh_header, sizeof(MessageHeader));
    
    // Copy fields
    size_t offset = sizeof(MessageHeader);
    memcpy(buffer.data() + offset, &request_action, sizeof(request_action));
    offset += sizeof(request_action);
    
    memcpy(buffer.data() + offset, &symbol_id, sizeof(symbol_id));
    offset += sizeof(symbol_id);
    
    memcpy(buffer.data() + offset, symbol, sizeof(symbol));
    
    return buffer;
}

bool MarketDataRequest::deserialize(const uint8_t* data, uint16_t size) {
    uint16_t expected_size = sizeof(MessageHeader) + sizeof(request_action) + 
                            sizeof(symbol_id) + sizeof(symbol);
    if (size < expected_size) return false;
    
    // Safely copy field by field to avoid vtable corruption
    size_t offset = 0;
    memcpy(&header, data + offset, sizeof(header));
    offset += sizeof(header);
    
    memcpy(&request_action, data + offset, sizeof(request_action));
    offset += sizeof(request_action);
    
    memcpy(&symbol_id, data + offset, sizeof(symbol_id));
    offset += sizeof(symbol_id);
    
    memcpy(symbol, data + offset, sizeof(symbol));
    
    return true;
}

std::vector<uint8_t> MarketDataUpdateTrade::serialize() const {
    // Create a fresh header with correct size
    uint16_t data_size = sizeof(MessageHeader) + sizeof(symbol_id) + 
                        sizeof(price) + sizeof(volume) + sizeof(date_time);
    MessageHeader fresh_header(data_size, MessageType::MARKET_DATA_UPDATE_TRADE);
    
    std::vector<uint8_t> buffer(data_size);
    
    // Copy header
    memcpy(buffer.data(), &fresh_header, sizeof(MessageHeader));
    
    // Copy fields
    size_t offset = sizeof(MessageHeader);
    memcpy(buffer.data() + offset, &symbol_id, sizeof(symbol_id));
    offset += sizeof(symbol_id);
    memcpy(buffer.data() + offset, &price, sizeof(price));
    offset += sizeof(price);
    memcpy(buffer.data() + offset, &volume, sizeof(volume));
    offset += sizeof(volume);
    memcpy(buffer.data() + offset, &date_time, sizeof(date_time));
    
    return buffer;
}

bool MarketDataUpdateTrade::deserialize(const uint8_t* data, uint16_t size) {
    uint16_t expected_size = sizeof(MessageHeader) + sizeof(symbol_id) + 
                            sizeof(price) + sizeof(volume) + sizeof(date_time);
    if (size < expected_size) return false;
    
    // Safely copy field by field
    size_t offset = 0;
    memcpy(&header, data + offset, sizeof(header));
    offset += sizeof(header);
    memcpy(&symbol_id, data + offset, sizeof(symbol_id));
    offset += sizeof(symbol_id);
    memcpy(&price, data + offset, sizeof(price));
    offset += sizeof(price);
    memcpy(&volume, data + offset, sizeof(volume));
    offset += sizeof(volume);
    memcpy(&date_time, data + offset, sizeof(date_time));
    
    return true;
}

std::vector<uint8_t> MarketDataUpdateBidAsk::serialize() const {
    // Create a fresh header with correct size
    uint16_t data_size = sizeof(MessageHeader) + sizeof(symbol_id) + 
                        sizeof(bid_price) + sizeof(bid_quantity) + 
                        sizeof(ask_price) + sizeof(ask_quantity) + sizeof(date_time);
    MessageHeader fresh_header(data_size, MessageType::MARKET_DATA_UPDATE_BID_ASK);
    
    std::vector<uint8_t> buffer(data_size);
    
    // Copy header
    memcpy(buffer.data(), &fresh_header, sizeof(MessageHeader));
    
    // Copy fields
    size_t offset = sizeof(MessageHeader);
    memcpy(buffer.data() + offset, &symbol_id, sizeof(symbol_id));
    offset += sizeof(symbol_id);
    memcpy(buffer.data() + offset, &bid_price, sizeof(bid_price));
    offset += sizeof(bid_price);
    memcpy(buffer.data() + offset, &bid_quantity, sizeof(bid_quantity));
    offset += sizeof(bid_quantity);
    memcpy(buffer.data() + offset, &ask_price, sizeof(ask_price));
    offset += sizeof(ask_price);
    memcpy(buffer.data() + offset, &ask_quantity, sizeof(ask_quantity));
    offset += sizeof(ask_quantity);
    memcpy(buffer.data() + offset, &date_time, sizeof(date_time));
    
    return buffer;
}

bool MarketDataUpdateBidAsk::deserialize(const uint8_t* data, uint16_t size) {
    uint16_t expected_size = sizeof(MessageHeader) + sizeof(symbol_id) + 
                            sizeof(bid_price) + sizeof(bid_quantity) + 
                            sizeof(ask_price) + sizeof(ask_quantity) + sizeof(date_time);
    if (size < expected_size) return false;
    
    // Safely copy field by field
    size_t offset = 0;
    memcpy(&header, data + offset, sizeof(header));
    offset += sizeof(header);
    memcpy(&symbol_id, data + offset, sizeof(symbol_id));
    offset += sizeof(symbol_id);
    memcpy(&bid_price, data + offset, sizeof(bid_price));
    offset += sizeof(bid_price);
    memcpy(&bid_quantity, data + offset, sizeof(bid_quantity));
    offset += sizeof(bid_quantity);
    memcpy(&ask_price, data + offset, sizeof(ask_price));
    offset += sizeof(ask_price);
    memcpy(&ask_quantity, data + offset, sizeof(ask_quantity));
    offset += sizeof(ask_quantity);
    memcpy(&date_time, data + offset, sizeof(date_time));
    
    return true;
}

// Protocol Message Processing Methods

std::unique_ptr<DTCMessage> Protocol::parse_message(const uint8_t* data, uint16_t size) {
    if (!validate_message_header(data, size)) {
        return nullptr;
    }
    
    MessageType type = get_message_type(data, size);
    
    switch (type) {
        case MessageType::LOGON_REQUEST: {
            auto msg = std::make_unique<LogonRequest>();
            if (msg->deserialize(data, size)) {
                return std::move(msg);
            }
            break;
        }
        
        case MessageType::MARKET_DATA_REQUEST: {
            auto msg = std::make_unique<MarketDataRequest>();
            if (msg->deserialize(data, size)) {
                return std::move(msg);
            }
            break;
        }
        
        case MessageType::MARKET_DATA_UPDATE_TRADE: {
            auto msg = std::make_unique<MarketDataUpdateTrade>();
            if (msg->deserialize(data, size)) {
                return std::move(msg);
            }
            break;
        }
        
        case MessageType::MARKET_DATA_UPDATE_BID_ASK: {
            auto msg = std::make_unique<MarketDataUpdateBidAsk>();
            if (msg->deserialize(data, size)) {
                return std::move(msg);
            }
            break;
        }
        
        default:
            std::cout << "⚠️ Unknown message type: " << static_cast<uint16_t>(type) << std::endl;
            break;
    }
    
    return nullptr;
}

std::vector<uint8_t> Protocol::create_message(const DTCMessage& message) {
    return message.serialize();
}

// Protocol Helper Methods

std::unique_ptr<LogonResponse> Protocol::create_logon_response(bool success, const std::string& message) {
    auto response = std::make_unique<LogonResponse>();
    response->result = success ? 1 : 0;
    
    if (!message.empty()) {
        strncpy(response->result_text, message.c_str(), sizeof(response->result_text) - 1);
        response->result_text[sizeof(response->result_text) - 1] = '\0';
    }
    
    return response;
}

std::unique_ptr<MarketDataUpdateTrade> Protocol::create_trade_update(
    uint32_t symbol_id, double price, double volume, uint64_t timestamp) {
    
    auto update = std::make_unique<MarketDataUpdateTrade>();
    update->symbol_id = symbol_id;
    update->price = price;
    update->volume = volume;
    update->date_time = timestamp;
    
    return update;
}

std::unique_ptr<MarketDataUpdateBidAsk> Protocol::create_bid_ask_update(
    uint32_t symbol_id, double bid_price, double bid_qty, 
    double ask_price, double ask_qty, uint64_t timestamp) {
    
    auto update = std::make_unique<MarketDataUpdateBidAsk>();
    update->symbol_id = symbol_id;
    update->bid_price = bid_price;
    update->bid_quantity = bid_qty;
    update->ask_price = ask_price;
    update->ask_quantity = ask_qty;
    update->date_time = timestamp;
    
    return update;
}

// Utility Functions

uint64_t Protocol::get_current_timestamp() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

MessageType Protocol::get_message_type(const uint8_t* data, uint16_t size) {
    if (size < sizeof(MessageHeader)) {
        return static_cast<MessageType>(0); // Invalid
    }
    
    const MessageHeader* header = reinterpret_cast<const MessageHeader*>(data);
    return static_cast<MessageType>(header->type);
}

bool Protocol::validate_message_header(const uint8_t* data, uint16_t size) {
    if (size < sizeof(MessageHeader)) {
        std::cout << "❌ Message too small for header: " << size << " bytes" << std::endl;
        return false;
    }
    
    const MessageHeader* header = reinterpret_cast<const MessageHeader*>(data);
    
    // Check if message size matches header
    if (header->size != size) {
        std::cout << "❌ Message size mismatch: header=" << header->size 
                  << ", actual=" << size << std::endl;
        return false;
    }
    
    // Check if message type is valid
    uint16_t type = header->type;
    if (type == 0 || type > 1000) { // Basic sanity check
        std::cout << "❌ Invalid message type: " << type << std::endl;
        return false;
    }
    
    return true;
}

} // namespace dtc
} // namespace coinbase_dtc_core
