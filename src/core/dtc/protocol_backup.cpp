#include "coinbase_dtc_core/core/dtc/protocol.hpp"
#include <cstring>
#include <cstdio>

namespace coinbase_dtc_core {
namespace core {
namespace dtc {

// Stub implementations that compile - we'll just make simple versions

uint16_t LogonRequest::get_size() const {
    return sizeof(MessageHeader) + 100; // Simple approximation
}

std::vector<uint8_t> LogonRequest::serialize() const {
    std::vector<uint8_t> buffer(get_size());
    MessageHeader header(get_size(), get_type());
    std::memcpy(buffer.data(), &header, sizeof(MessageHeader));
    return buffer;
}

bool LogonRequest::deserialize(const uint8_t* data, uint16_t size) {
    return size >= sizeof(MessageHeader);
}

uint16_t LogonResponse::get_size() const {
    return sizeof(MessageHeader) + 100;
}

std::vector<uint8_t> LogonResponse::serialize() const {
    std::vector<uint8_t> buffer(get_size());
    MessageHeader header(get_size(), get_type());
    std::memcpy(buffer.data(), &header, sizeof(MessageHeader));
    return buffer;
}

bool LogonResponse::deserialize(const uint8_t* data, uint16_t size) {
    return size >= sizeof(MessageHeader);
}

uint16_t MarketDataRequest::get_size() const {
    return sizeof(MessageHeader) + 50;
}

std::vector<uint8_t> MarketDataRequest::serialize() const {
    std::vector<uint8_t> buffer(get_size());
    MessageHeader header(get_size(), get_type());
    std::memcpy(buffer.data(), &header, sizeof(MessageHeader));
    return buffer;
}

bool MarketDataRequest::deserialize(const uint8_t* data, uint16_t size) {
    return size >= sizeof(MessageHeader);
}

uint16_t MarketDataUpdateTrade::get_size() const {
    return sizeof(MessageHeader) + 40;
}

std::vector<uint8_t> MarketDataUpdateTrade::serialize() const {
    std::vector<uint8_t> buffer(get_size());
    MessageHeader header(get_size(), get_type());
    std::memcpy(buffer.data(), &header, sizeof(MessageHeader));
    return buffer;
}

bool MarketDataUpdateTrade::deserialize(const uint8_t* data, uint16_t size) {
    return size >= sizeof(MessageHeader);
}

uint16_t MarketDataUpdateBidAsk::get_size() const {
    return sizeof(MessageHeader) + 50;
}

std::vector<uint8_t> MarketDataUpdateBidAsk::serialize() const {
    std::vector<uint8_t> buffer(get_size());
    MessageHeader header(get_size(), get_type());
    std::memcpy(buffer.data(), &header, sizeof(MessageHeader));
    return buffer;
}

bool MarketDataUpdateBidAsk::deserialize(const uint8_t* data, uint16_t size) {
    return size >= sizeof(MessageHeader);
}

uint16_t Heartbeat::get_size() const {
    return sizeof(MessageHeader) + 20;
}

std::vector<uint8_t> Heartbeat::serialize() const {
    std::vector<uint8_t> buffer(get_size());
    MessageHeader header(get_size(), get_type());
    std::memcpy(buffer.data(), &header, sizeof(MessageHeader));
    return buffer;
}

bool Heartbeat::deserialize(const uint8_t* data, uint16_t size) {
    return size >= sizeof(MessageHeader);
}

// Protocol class implementation
Protocol::Protocol() {}

std::string Protocol::version() const {
    return "8.0.1";
}

std::unique_ptr<DTCMessage> Protocol::parse_message(const uint8_t* data, size_t size) {
    if (!data || size < sizeof(MessageHeader)) {
        return nullptr;
    }

    const MessageHeader* header = reinterpret_cast<const MessageHeader*>(data);
    MessageType type = static_cast<MessageType>(header->type);

    switch (type) {
        case MessageType::LOGON_REQUEST: {
            auto msg = std::make_unique<LogonRequest>();
            if (msg->deserialize(data, header->size)) {
                return std::move(msg);
            }
            break;
        }
        case MessageType::LOGON_RESPONSE: {
            auto msg = std::make_unique<LogonResponse>();
            if (msg->deserialize(data, header->size)) {
                return std::move(msg);
            }
            break;
        }
        case MessageType::MARKET_DATA_REQUEST: {
            auto msg = std::make_unique<MarketDataRequest>();
            if (msg->deserialize(data, header->size)) {
                return std::move(msg);
            }
            break;
        }
        case MessageType::MARKET_DATA_UPDATE_TRADE: {
            auto msg = std::make_unique<MarketDataUpdateTrade>();
            if (msg->deserialize(data, header->size)) {
                return std::move(msg);
            }
            break;
        }
        case MessageType::MARKET_DATA_UPDATE_BID_ASK: {
            auto msg = std::make_unique<MarketDataUpdateBidAsk>();
            if (msg->deserialize(data, header->size)) {
                return std::move(msg);
            }
            break;
        }
        default:
            return nullptr;
    }

    return nullptr;
}

} // namespace dtc
} // namespace core
} // namespace coinbase_dtc_core