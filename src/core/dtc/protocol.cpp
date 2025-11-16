#include "coinbase_dtc_core/core/dtc/protocol.hpp"
#include <cstring>
#include <cstdio>
#include <chrono>

namespace open_dtc_server
{
    namespace core
    {
        namespace dtc
        {

            // Stub implementations that compile - we'll just make simple versions

            uint16_t LogonRequest::get_size() const
            {
                return sizeof(MessageHeader) + 100; // Simple approximation
            }

            std::vector<uint8_t> LogonRequest::serialize() const
            {
                std::vector<uint8_t> buffer(get_size());
                MessageHeader header(get_size(), get_type());
                std::memcpy(buffer.data(), &header, sizeof(MessageHeader));
                return buffer;
            }

            bool LogonRequest::deserialize(const uint8_t *data, uint16_t size)
            {
                return size >= sizeof(MessageHeader);
            }

            uint16_t LogonResponse::get_size() const
            {
                return sizeof(MessageHeader) + 1 + server_name.length() + 1; // +1 for result, +1 for null terminator
            }

            std::vector<uint8_t> LogonResponse::serialize() const
            {
                // Simple serialization: header + result byte + server_name
                size_t total_size = sizeof(MessageHeader) + 1 + server_name.length() + 1; // +1 for null terminator
                std::vector<uint8_t> buffer(total_size);

                MessageHeader header(total_size, get_type());
                std::memcpy(buffer.data(), &header, sizeof(MessageHeader));

                // Write result
                buffer[sizeof(MessageHeader)] = result;

                // Write server name (null-terminated string)
                if (!server_name.empty())
                {
                    std::memcpy(buffer.data() + sizeof(MessageHeader) + 1, server_name.c_str(), server_name.length());
                }
                buffer[total_size - 1] = 0; // null terminator

                return buffer;
            }

            bool LogonResponse::deserialize(const uint8_t *data, uint16_t size)
            {
                if (size < sizeof(MessageHeader) + 1)
                    return false;

                // Read result
                result = data[sizeof(MessageHeader)];

                // Read server name if present
                if (size > sizeof(MessageHeader) + 1)
                {
                    const char *name_ptr = reinterpret_cast<const char *>(data + sizeof(MessageHeader) + 1);
                    server_name = std::string(name_ptr, size - sizeof(MessageHeader) - 2); // -2 for result byte and null terminator
                }

                return true;
            }

            uint16_t MarketDataRequest::get_size() const
            {
                return sizeof(MessageHeader) + sizeof(uint16_t) + // request_action
                       sizeof(uint16_t) +                         // symbol_id
                       symbol.length() + 1 +                      // symbol + null terminator
                       exchange.length() + 1;                     // exchange + null terminator
            }

            std::vector<uint8_t> MarketDataRequest::serialize() const
            {
                std::vector<uint8_t> buffer(get_size());
                MessageHeader header(get_size(), get_type());
                std::memcpy(buffer.data(), &header, sizeof(MessageHeader));

                uint8_t *ptr = buffer.data() + sizeof(MessageHeader);

                // Write request_action
                uint16_t action = static_cast<uint16_t>(request_action);
                std::memcpy(ptr, &action, sizeof(uint16_t));
                ptr += sizeof(uint16_t);

                // Write symbol_id
                std::memcpy(ptr, &symbol_id, sizeof(uint16_t));
                ptr += sizeof(uint16_t);

                // Write symbol (null-terminated)
                std::memcpy(ptr, symbol.c_str(), symbol.length() + 1);
                ptr += symbol.length() + 1;

                // Write exchange (null-terminated)
                std::memcpy(ptr, exchange.c_str(), exchange.length() + 1);

                return buffer;
            }

            bool MarketDataRequest::deserialize(const uint8_t *data, uint16_t size)
            {
                if (size < sizeof(MessageHeader) + sizeof(uint16_t) * 2)
                {
                    return false;
                }

                // Skip message header
                const uint8_t *ptr = data + sizeof(MessageHeader);
                size_t remaining = size - sizeof(MessageHeader);

                // Read request_action
                if (remaining < sizeof(uint16_t))
                    return false;
                uint16_t action;
                std::memcpy(&action, ptr, sizeof(uint16_t));
                request_action = static_cast<RequestAction>(action);
                ptr += sizeof(uint16_t);
                remaining -= sizeof(uint16_t);

                // Read symbol_id
                if (remaining < sizeof(uint16_t))
                    return false;
                std::memcpy(&symbol_id, ptr, sizeof(uint16_t));
                ptr += sizeof(uint16_t);
                remaining -= sizeof(uint16_t);

                // Read symbol (null-terminated string)
                const char *symbol_start = reinterpret_cast<const char *>(ptr);
                size_t symbol_len = strnlen(symbol_start, remaining);
                if (symbol_len == remaining)
                    return false; // No null terminator found
                symbol.assign(symbol_start, symbol_len);
                ptr += symbol_len + 1;
                remaining -= symbol_len + 1;

                // Read exchange (null-terminated string)
                if (remaining == 0)
                    return false;
                const char *exchange_start = reinterpret_cast<const char *>(ptr);
                size_t exchange_len = strnlen(exchange_start, remaining);
                if (exchange_len == remaining)
                    return false;
                exchange.assign(exchange_start, exchange_len);

                return true;
            }

            // MarketDataResponse implementation
            uint16_t MarketDataResponse::get_size() const
            {
                return sizeof(MessageHeader) + sizeof(uint16_t) + // symbol_id
                       symbol.length() + 1 +                      // symbol + null terminator
                       exchange.length() + 1 +                    // exchange + null terminator
                       sizeof(uint8_t);                           // result
            }

            std::vector<uint8_t> MarketDataResponse::serialize() const
            {
                std::vector<uint8_t> buffer(get_size());
                MessageHeader header(get_size(), get_type());
                std::memcpy(buffer.data(), &header, sizeof(MessageHeader));

                uint8_t *ptr = buffer.data() + sizeof(MessageHeader);

                // Write symbol_id
                std::memcpy(ptr, &symbol_id, sizeof(uint16_t));
                ptr += sizeof(uint16_t);

                // Write symbol (null-terminated)
                std::memcpy(ptr, symbol.c_str(), symbol.length() + 1);
                ptr += symbol.length() + 1;

                // Write exchange (null-terminated)
                std::memcpy(ptr, exchange.c_str(), exchange.length() + 1);
                ptr += exchange.length() + 1;

                // Write result
                std::memcpy(ptr, &result, sizeof(uint8_t));

                return buffer;
            }

            bool MarketDataResponse::deserialize(const uint8_t *data, uint16_t size)
            {
                if (size < sizeof(MessageHeader) + sizeof(uint16_t) + sizeof(uint8_t))
                {
                    return false;
                }

                // Skip message header
                const uint8_t *ptr = data + sizeof(MessageHeader);
                size_t remaining = size - sizeof(MessageHeader);

                // Read symbol_id
                if (remaining < sizeof(uint16_t))
                    return false;
                std::memcpy(&symbol_id, ptr, sizeof(uint16_t));
                ptr += sizeof(uint16_t);
                remaining -= sizeof(uint16_t);

                // Read symbol (null-terminated string)
                const char *symbol_start = reinterpret_cast<const char *>(ptr);
                size_t symbol_len = strnlen(symbol_start, remaining);
                if (symbol_len == remaining)
                    return false; // No null terminator found
                symbol.assign(symbol_start, symbol_len);
                ptr += symbol_len + 1;
                remaining -= symbol_len + 1;

                // Read exchange (null-terminated string)
                if (remaining == 0)
                    return false;
                const char *exchange_start = reinterpret_cast<const char *>(ptr);
                size_t exchange_len = strnlen(exchange_start, remaining);
                if (exchange_len == remaining)
                    return false;
                exchange.assign(exchange_start, exchange_len);
                ptr += exchange_len + 1;
                remaining -= exchange_len + 1;

                // Read result
                if (remaining < sizeof(uint8_t))
                    return false;
                std::memcpy(&result, ptr, sizeof(uint8_t));

                return true;
            }

            uint16_t MarketDataUpdateTrade::get_size() const
            {
                return sizeof(MessageHeader) + 40;
            }

            std::vector<uint8_t> MarketDataUpdateTrade::serialize() const
            {
                std::vector<uint8_t> buffer(get_size());
                MessageHeader header(get_size(), get_type());
                std::memcpy(buffer.data(), &header, sizeof(MessageHeader));
                return buffer;
            }

            bool MarketDataUpdateTrade::deserialize(const uint8_t *data, uint16_t size)
            {
                return size >= sizeof(MessageHeader);
            }

            uint16_t MarketDataUpdateBidAsk::get_size() const
            {
                return sizeof(MessageHeader) + 50;
            }

            std::vector<uint8_t> MarketDataUpdateBidAsk::serialize() const
            {
                std::vector<uint8_t> buffer(get_size());
                MessageHeader header(get_size(), get_type());
                std::memcpy(buffer.data(), &header, sizeof(MessageHeader));
                return buffer;
            }

            bool MarketDataUpdateBidAsk::deserialize(const uint8_t *data, uint16_t size)
            {
                return size >= sizeof(MessageHeader);
            }

            uint16_t Heartbeat::get_size() const
            {
                return sizeof(MessageHeader) + 20;
            }

            std::vector<uint8_t> Heartbeat::serialize() const
            {
                std::vector<uint8_t> buffer(get_size());
                MessageHeader header(get_size(), get_type());
                std::memcpy(buffer.data(), &header, sizeof(MessageHeader));
                return buffer;
            }

            bool Heartbeat::deserialize(const uint8_t *data, uint16_t size)
            {
                return size >= sizeof(MessageHeader);
            }

            // SecurityDefinitionForSymbolRequest implementation
            uint16_t SecurityDefinitionForSymbolRequest::get_size() const
            {
                return sizeof(MessageHeader) + sizeof(uint32_t) + symbol.length() + 1 + exchange.length() + 1 + product_type.length() + 1;
            }

            std::vector<uint8_t> SecurityDefinitionForSymbolRequest::serialize() const
            {
                std::vector<uint8_t> buffer(get_size());
                MessageHeader header(get_size(), get_type());
                std::memcpy(buffer.data(), &header, sizeof(MessageHeader));

                uint8_t *ptr = buffer.data() + sizeof(MessageHeader);

                // Write request_id
                std::memcpy(ptr, &request_id, sizeof(uint32_t));
                ptr += sizeof(uint32_t);

                // Write symbol (null-terminated)
                std::memcpy(ptr, symbol.c_str(), symbol.length() + 1);
                ptr += symbol.length() + 1;

                // Write exchange (null-terminated)
                std::memcpy(ptr, exchange.c_str(), exchange.length() + 1);
                ptr += exchange.length() + 1;

                // Write product_type (null-terminated)
                std::memcpy(ptr, product_type.c_str(), product_type.length() + 1);

                return buffer;
            }

            bool SecurityDefinitionForSymbolRequest::deserialize(const uint8_t *data, uint16_t size)
            {
                if (size < sizeof(MessageHeader) + sizeof(uint32_t))
                {
                    return false;
                }

                // Skip message header
                const uint8_t *ptr = data + sizeof(MessageHeader);
                size_t remaining = size - sizeof(MessageHeader);

                // Read request_id
                if (remaining < sizeof(uint32_t))
                    return false;
                std::memcpy(&request_id, ptr, sizeof(uint32_t));
                ptr += sizeof(uint32_t);
                remaining -= sizeof(uint32_t);

                // Read symbol (null-terminated string)
                const char *symbol_start = reinterpret_cast<const char *>(ptr);
                size_t symbol_len = strnlen(symbol_start, remaining);
                if (symbol_len == remaining)
                    return false; // No null terminator found
                symbol.assign(symbol_start, symbol_len);
                ptr += symbol_len + 1; // +1 for null terminator
                remaining -= symbol_len + 1;

                // Read exchange (null-terminated string)
                if (remaining == 0)
                    return false;
                const char *exchange_start = reinterpret_cast<const char *>(ptr);
                size_t exchange_len = strnlen(exchange_start, remaining);
                if (exchange_len == remaining)
                    return false; // No null terminator found
                exchange.assign(exchange_start, exchange_len);
                ptr += exchange_len + 1;
                remaining -= exchange_len + 1;

                // Read product_type (null-terminated string)
                if (remaining == 0)
                    return false;
                const char *product_type_start = reinterpret_cast<const char *>(ptr);
                size_t product_type_len = strnlen(product_type_start, remaining);
                if (product_type_len == remaining)
                    return false; // No null terminator found
                product_type.assign(product_type_start, product_type_len);

                return true;
            }

            // SecurityDefinitionResponse implementation
            uint16_t SecurityDefinitionResponse::get_size() const
            {
                return sizeof(MessageHeader) + sizeof(uint32_t) + // request_id
                       symbol.length() + 1 +                      // symbol + null terminator
                       exchange.length() + 1 +                    // exchange + null terminator
                       description.length() + 1 +                 // description + null terminator
                       currency.length() + 1 +                    // currency + null terminator
                       sizeof(uint32_t) +                         // security_type
                       sizeof(float) * 4 +                        // price increments, contract_size
                       sizeof(uint8_t);                           // has_market_depth_data
            }

            std::vector<uint8_t> SecurityDefinitionResponse::serialize() const
            {
                std::vector<uint8_t> buffer(get_size());
                MessageHeader header(get_size(), get_type());
                std::memcpy(buffer.data(), &header, sizeof(MessageHeader));

                uint8_t *ptr = buffer.data() + sizeof(MessageHeader);

                // Write request_id
                std::memcpy(ptr, &request_id, sizeof(uint32_t));
                ptr += sizeof(uint32_t);

                // Write symbol (null-terminated)
                std::memcpy(ptr, symbol.c_str(), symbol.length() + 1);
                ptr += symbol.length() + 1;

                // Write exchange (null-terminated)
                std::memcpy(ptr, exchange.c_str(), exchange.length() + 1);
                ptr += exchange.length() + 1;

                // Write description (null-terminated)
                std::memcpy(ptr, description.c_str(), description.length() + 1);
                ptr += description.length() + 1;

                // Write currency (null-terminated)
                std::memcpy(ptr, currency.c_str(), currency.length() + 1);
                ptr += currency.length() + 1;

                // Write security_type
                std::memcpy(ptr, &security_type, sizeof(uint32_t));
                ptr += sizeof(uint32_t);

                // Write price increments
                std::memcpy(ptr, &min_price_increment, sizeof(float));
                ptr += sizeof(float);
                std::memcpy(ptr, &currency_value_per_increment, sizeof(float));
                ptr += sizeof(float);

                // Write contract_size
                std::memcpy(ptr, &contract_size, sizeof(float));
                ptr += sizeof(float);

                // Write has_market_depth_data
                std::memcpy(ptr, &has_market_depth_data, sizeof(uint8_t));

                return buffer;
            }

            bool SecurityDefinitionResponse::deserialize(const uint8_t *data, uint16_t size)
            {
                if (size < sizeof(MessageHeader) + sizeof(uint32_t))
                {
                    return false;
                }

                // Skip message header
                const uint8_t *ptr = data + sizeof(MessageHeader);
                size_t remaining = size - sizeof(MessageHeader);

                // Read request_id
                if (remaining < sizeof(uint32_t))
                    return false;
                std::memcpy(&request_id, ptr, sizeof(uint32_t));
                ptr += sizeof(uint32_t);
                remaining -= sizeof(uint32_t);

                // Read symbol (null-terminated string)
                const char *symbol_start = reinterpret_cast<const char *>(ptr);
                size_t symbol_len = strnlen(symbol_start, remaining);
                if (symbol_len == remaining)
                    return false; // No null terminator found
                symbol.assign(symbol_start, symbol_len);
                ptr += symbol_len + 1;
                remaining -= symbol_len + 1;

                // Read exchange (null-terminated string)
                if (remaining == 0)
                    return false;
                const char *exchange_start = reinterpret_cast<const char *>(ptr);
                size_t exchange_len = strnlen(exchange_start, remaining);
                if (exchange_len == remaining)
                    return false;
                exchange.assign(exchange_start, exchange_len);
                ptr += exchange_len + 1;
                remaining -= exchange_len + 1;

                // Read description (null-terminated string)
                if (remaining == 0)
                    return false;
                const char *description_start = reinterpret_cast<const char *>(ptr);
                size_t description_len = strnlen(description_start, remaining);
                if (description_len == remaining)
                    return false;
                description.assign(description_start, description_len);
                ptr += description_len + 1;
                remaining -= description_len + 1;

                // Read currency (null-terminated string)
                if (remaining == 0)
                    return false;
                const char *currency_start = reinterpret_cast<const char *>(ptr);
                size_t currency_len = strnlen(currency_start, remaining);
                if (currency_len == remaining)
                    return false;
                currency.assign(currency_start, currency_len);
                ptr += currency_len + 1;
                remaining -= currency_len + 1;

                // Read remaining numeric fields
                if (remaining < sizeof(uint32_t) + sizeof(float) * 3 + sizeof(uint8_t))
                    return false;

                std::memcpy(&security_type, ptr, sizeof(uint32_t));
                ptr += sizeof(uint32_t);

                std::memcpy(&min_price_increment, ptr, sizeof(float));
                ptr += sizeof(float);

                std::memcpy(&currency_value_per_increment, ptr, sizeof(float));
                ptr += sizeof(float);

                std::memcpy(&contract_size, ptr, sizeof(float));
                ptr += sizeof(float);

                std::memcpy(&has_market_depth_data, ptr, sizeof(uint8_t));

                return true;
            }

            // Protocol class implementation
            Protocol::Protocol() {}

            std::string Protocol::version() const
            {
                return "8.0.1";
            }

            std::unique_ptr<DTCMessage> Protocol::parse_message(const uint8_t *data, uint16_t size)
            {
                if (!data || size < sizeof(MessageHeader))
                {
                    return nullptr;
                }

                const MessageHeader *header = reinterpret_cast<const MessageHeader *>(data);
                MessageType type = static_cast<MessageType>(header->type);

                switch (type)
                {
                case MessageType::LOGON_REQUEST:
                {
                    auto msg = std::make_unique<LogonRequest>();
                    if (msg->deserialize(data, header->size))
                    {
                        return std::move(msg);
                    }
                    break;
                }
                case MessageType::LOGON_RESPONSE:
                {
                    auto msg = std::make_unique<LogonResponse>();
                    if (msg->deserialize(data, header->size))
                    {
                        return std::move(msg);
                    }
                    break;
                }
                case MessageType::MARKET_DATA_REQUEST:
                {
                    auto msg = std::make_unique<MarketDataRequest>();
                    if (msg->deserialize(data, header->size))
                    {
                        return std::move(msg);
                    }
                    break;
                }
                case MessageType::MARKET_DATA_RESPONSE:
                {
                    auto msg = std::make_unique<MarketDataResponse>();
                    if (msg->deserialize(data, header->size))
                    {
                        return std::move(msg);
                    }
                    break;
                }
                case MessageType::MARKET_DATA_UPDATE_TRADE:
                {
                    auto msg = std::make_unique<MarketDataUpdateTrade>();
                    if (msg->deserialize(data, header->size))
                    {
                        return std::move(msg);
                    }
                    break;
                }
                case MessageType::MARKET_DATA_UPDATE_BID_ASK:
                {
                    auto msg = std::make_unique<MarketDataUpdateBidAsk>();
                    if (msg->deserialize(data, header->size))
                    {
                        return std::move(msg);
                    }
                    break;
                }
                case MessageType::POSITION_UPDATE:
                {
                    auto msg = std::make_unique<PositionUpdate>();
                    if (msg->deserialize(data, header->size))
                    {
                        return std::move(msg);
                    }
                    break;
                }
                case MessageType::SECURITY_DEFINITION_FOR_SYMBOL_REQUEST:
                {
                    auto msg = std::make_unique<SecurityDefinitionForSymbolRequest>();
                    if (msg->deserialize(data, header->size))
                    {
                        return std::move(msg);
                    }
                    break;
                }
                case MessageType::SECURITY_DEFINITION_RESPONSE:
                {
                    auto msg = std::make_unique<SecurityDefinitionResponse>();
                    if (msg->deserialize(data, header->size))
                    {
                        return std::move(msg);
                    }
                    break;
                }
                default:
                    return nullptr;
                }

                return nullptr;
            }

            // Factory methods for creating messages
            std::unique_ptr<LogonResponse> Protocol::create_logon_response(bool success, const std::string &message)
            {
                auto response = std::make_unique<LogonResponse>();
                response->result = success ? 1 : 0;
                response->result_text = message;
                return response;
            }

            std::unique_ptr<MarketDataRequest> Protocol::create_market_data_request(
                RequestAction action, uint16_t symbol_id, const std::string &symbol, const std::string &exchange)
            {
                auto request = std::make_unique<MarketDataRequest>();
                request->request_action = action;
                request->symbol_id = symbol_id;
                request->symbol = symbol;
                request->exchange = exchange;
                return request;
            }

            std::unique_ptr<MarketDataResponse> Protocol::create_market_data_response(
                uint16_t symbol_id, const std::string &symbol, const std::string &exchange, bool success)
            {
                auto response = std::make_unique<MarketDataResponse>();
                response->symbol_id = symbol_id;
                response->symbol = symbol;
                response->exchange = exchange;
                response->result = success ? 1 : 0;
                return response;
            }

            std::unique_ptr<MarketDataUpdateTrade> Protocol::create_trade_update(
                uint16_t symbol_id, double price, double volume, uint64_t timestamp)
            {
                auto update = std::make_unique<MarketDataUpdateTrade>();
                update->symbol_id = symbol_id;
                update->price = price;
                update->volume = volume;
                update->date_time = timestamp;
                return update;
            }

            std::unique_ptr<MarketDataUpdateBidAsk> Protocol::create_bid_ask_update(
                uint16_t symbol_id, double bid_price, float bid_qty,
                double ask_price, float ask_qty, uint64_t timestamp)
            {
                auto update = std::make_unique<MarketDataUpdateBidAsk>();
                update->symbol_id = symbol_id;
                update->bid_price = bid_price;
                update->bid_quantity = bid_qty;
                update->ask_price = ask_price;
                update->ask_quantity = ask_qty;
                update->date_time = timestamp;
                return update;
            }

            // Static utility methods
            uint64_t Protocol::get_current_timestamp()
            {
                auto now = std::chrono::system_clock::now();
                auto time_t = std::chrono::system_clock::to_time_t(now);
                return static_cast<uint64_t>(time_t);
            }

            MessageType Protocol::get_message_type(const uint8_t *data, uint16_t size)
            {
                if (!data || size < sizeof(MessageHeader))
                {
                    return MessageType::LOGOFF; // Use LOGOFF as default/error value
                }
                const MessageHeader *header = reinterpret_cast<const MessageHeader *>(data);
                return static_cast<MessageType>(header->type);
            }

            bool Protocol::validate_message_header(const uint8_t *data, uint16_t size)
            {
                if (!data || size < sizeof(MessageHeader))
                {
                    return false;
                }
                const MessageHeader *header = reinterpret_cast<const MessageHeader *>(data);
                return header->size <= size && header->size >= sizeof(MessageHeader);
            }

            // Missing Protocol methods needed by server

            std::vector<uint8_t> Protocol::create_message(const DTCMessage &message)
            {
                return message.serialize();
            }

            std::unique_ptr<Heartbeat> Protocol::create_heartbeat(uint32_t num_drops)
            {
                auto heartbeat = std::make_unique<Heartbeat>();
                heartbeat->num_drops = num_drops;
                heartbeat->current_date_time = get_current_timestamp();
                return heartbeat;
            }

            std::unique_ptr<SecurityDefinitionResponse> Protocol::create_security_definition_response(
                uint32_t request_id, const std::string &symbol, const std::string &exchange)
            {
                auto response = std::make_unique<SecurityDefinitionResponse>();
                response->request_id = request_id;
                response->symbol = symbol;
                response->exchange = exchange;
                response->security_type = 2; // 2 = Stock (crypto treated as stock for DTC)
                response->description = symbol + " on " + exchange;
                response->min_price_increment = 0.01f;
                response->currency_value_per_increment = 0.01f;
                response->has_market_depth_data = 1;
                response->contract_size = 1.0f;
                response->currency = "USD";
                return response;
            }

            std::string Protocol::message_type_to_string(MessageType type)
            {
                switch (type)
                {
                case MessageType::LOGON_REQUEST:
                    return "LOGON_REQUEST";
                case MessageType::LOGON_RESPONSE:
                    return "LOGON_RESPONSE";
                case MessageType::HEARTBEAT:
                    return "HEARTBEAT";
                case MessageType::LOGOFF:
                    return "LOGOFF";
                case MessageType::MARKET_DATA_REQUEST:
                    return "MARKET_DATA_REQUEST";
                case MessageType::MARKET_DATA_RESPONSE:
                    return "MARKET_DATA_RESPONSE";
                case MessageType::MARKET_DATA_REJECT:
                    return "MARKET_DATA_REJECT";
                case MessageType::MARKET_DATA_UPDATE_TRADE:
                    return "MARKET_DATA_UPDATE_TRADE";
                case MessageType::MARKET_DATA_UPDATE_BID_ASK:
                    return "MARKET_DATA_UPDATE_BID_ASK";
                case MessageType::SECURITY_DEFINITION_FOR_SYMBOL_REQUEST:
                    return "SECURITY_DEFINITION_FOR_SYMBOL_REQUEST";
                case MessageType::SECURITY_DEFINITION_RESPONSE:
                    return "SECURITY_DEFINITION_RESPONSE";
                case MessageType::POSITION_UPDATE:
                    return "POSITION_UPDATE";
                default:
                    return "UNKNOWN_" + std::to_string(static_cast<uint16_t>(type));
                }
            }

            // PositionUpdate implementation
            uint16_t PositionUpdate::get_size() const
            {
                return sizeof(MessageHeader) + trade_account.length() + 1 +
                       symbol.length() + 1 + sizeof(double) * 2 +
                       position_identifier.length() + 1;
            }

            std::vector<uint8_t> PositionUpdate::serialize() const
            {
                size_t total_size = get_size();
                std::vector<uint8_t> buffer(total_size);
                size_t offset = 0;

                MessageHeader header(total_size, get_type());
                std::memcpy(buffer.data() + offset, &header, sizeof(MessageHeader));
                offset += sizeof(MessageHeader);

                // Write trade_account (null-terminated string)
                std::strcpy(reinterpret_cast<char *>(buffer.data() + offset), trade_account.c_str());
                offset += trade_account.length() + 1;

                // Write symbol (null-terminated string)
                std::strcpy(reinterpret_cast<char *>(buffer.data() + offset), symbol.c_str());
                offset += symbol.length() + 1;

                // Write quantity
                std::memcpy(buffer.data() + offset, &quantity, sizeof(double));
                offset += sizeof(double);

                // Write average_price
                std::memcpy(buffer.data() + offset, &average_price, sizeof(double));
                offset += sizeof(double);

                // Write position_identifier (null-terminated string)
                std::strcpy(reinterpret_cast<char *>(buffer.data() + offset), position_identifier.c_str());
                offset += position_identifier.length() + 1;

                return buffer;
            }

            bool PositionUpdate::deserialize(const uint8_t *data, uint16_t size)
            {
                if (size < sizeof(MessageHeader))
                    return false;

                size_t offset = sizeof(MessageHeader);

                // Read trade_account
                if (offset >= size)
                    return false;
                trade_account = std::string(reinterpret_cast<const char *>(data + offset));
                offset += trade_account.length() + 1;

                // Read symbol
                if (offset >= size)
                    return false;
                symbol = std::string(reinterpret_cast<const char *>(data + offset));
                offset += symbol.length() + 1;

                // Read quantity
                if (offset + sizeof(double) > size)
                    return false;
                std::memcpy(&quantity, data + offset, sizeof(double));
                offset += sizeof(double);

                // Read average_price
                if (offset + sizeof(double) > size)
                    return false;
                std::memcpy(&average_price, data + offset, sizeof(double));
                offset += sizeof(double);

                // Read position_identifier
                if (offset >= size)
                    return false;
                position_identifier = std::string(reinterpret_cast<const char *>(data + offset));

                return true;
            }

        } // namespace dtc
    } // namespace core
} // namespace open_dtc_server
