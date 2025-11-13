#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <chrono>

namespace open_dtc_server
{
    namespace core
    {
        namespace dtc
        {

            // DTC Protocol Version
            constexpr uint16_t DTC_PROTOCOL_VERSION = 8;

            // Core DTC Message Types (from DTC specification)
            enum class MessageType : uint16_t
            {
                // Logon/Connection Messages
                LOGON_REQUEST = 1,
                LOGON_RESPONSE = 2,
                HEARTBEAT = 3,
                LOGOFF = 5,
                DISCONNECT_FROM_SERVER_NO_RECONNECT = 6,

                // Market Data Messages
                MARKET_DATA_REQUEST = 101,
                MARKET_DATA_REJECT = 103,
                MARKET_DATA_SNAPSHOT = 104,
                MARKET_DATA_UPDATE_TRADE = 107,
                MARKET_DATA_UPDATE_TRADE_COMPACT = 112,
                MARKET_DATA_UPDATE_LAST_TRADE_SNAPSHOT = 134,
                MARKET_DATA_UPDATE_BID_ASK = 108,
                MARKET_DATA_UPDATE_BID_ASK_COMPACT = 117,
                MARKET_DATA_UPDATE_SESSION_OPEN = 120,
                MARKET_DATA_UPDATE_SESSION_HIGH = 114,
                MARKET_DATA_UPDATE_SESSION_LOW = 115,
                MARKET_DATA_UPDATE_SESSION_VOLUME = 113,
                MARKET_DATA_UPDATE_OPEN_INTEREST = 124,

                // Trading Messages
                SUBMIT_NEW_SINGLE_ORDER = 208,
                SUBMIT_NEW_SINGLE_ORDER_INT = 206,
                CANCEL_ORDER = 203,
                CANCEL_REPLACE_ORDER = 204,
                ORDER_UPDATE = 210,
                ORDER_CANCEL_UPDATE = 212,
                ORDER_CANCEL_REJECT = 213,

                // Account/Position Messages
                OPEN_ORDERS_REQUEST = 300,
                OPEN_ORDERS_REJECT = 302,
                ORDER_UPDATE_REPORT = 301,
                CURRENT_POSITIONS_REQUEST = 400,
                CURRENT_POSITIONS_REJECT = 402,
                POSITION_UPDATE = 401,

                // Symbol/Security Messages
                SECURITY_DEFINITION_FOR_SYMBOL_REQUEST = 501,
                SECURITY_DEFINITION_RESPONSE = 502,
                SYMBOL_SEARCH_REQUEST = 503,
                SYMBOL_SEARCH_RESPONSE = 504,

                // System Messages
                GENERAL_LOG_MESSAGE = 700,
                ALERT_MESSAGE = 701,
                JOURNAL_ENTRY_ADD = 702,
                JOURNAL_ENTRIES_REQUEST = 703,
                JOURNAL_ENTRIES_REJECT = 704,
                JOURNAL_ENTRY_RESPONSE = 705
            };

            // DTC Request Action enumeration
            enum class RequestAction : uint8_t
            {
                SUBSCRIBE = 1,
                UNSUBSCRIBE = 2,
                SNAPSHOT = 3
            };

            // Order Status enumeration
            enum class OrderStatusEnum : uint8_t
            {
                ORDER_STATUS_UNSPECIFIED = 0,
                ORDER_STATUS_ORDER_SENT = 1,
                ORDER_STATUS_PENDING_OPEN = 2,
                ORDER_STATUS_PENDING_CHILD = 3,
                ORDER_STATUS_OPEN = 4,
                ORDER_STATUS_FILLED = 5,
                ORDER_STATUS_CANCELED = 6,
                ORDER_STATUS_REJECTED = 7,
                ORDER_STATUS_PARTIALLY_FILLED = 8
            };

            // Order Type enumeration
            enum class OrderTypeEnum : uint8_t
            {
                ORDER_TYPE_UNSET = 0,
                ORDER_TYPE_MARKET = 1,
                ORDER_TYPE_LIMIT = 2,
                ORDER_TYPE_STOP = 3,
                ORDER_TYPE_STOP_LIMIT = 4,
                ORDER_TYPE_MARKET_IF_TOUCHED = 5
            };

            // Buy/Sell enumeration
            enum class BuySellEnum : uint8_t
            {
                BUY_SELL_UNSET = 0,
                BUY = 1,
                SELL = 2
            };

            // Time In Force enumeration
            enum class TimeInForceEnum : uint8_t
            {
                TIF_UNSET = 0,
                TIF_DAY = 1,
                TIF_GOOD_TILL_CANCELED = 2,
                TIF_GOOD_TILL_DATE_TIME = 3,
                TIF_IMMEDIATE_OR_CANCEL = 4,
                TIF_FILL_OR_KILL = 5,
                TIF_GOOD_TILL_CROSSING = 6
            };

// DTC Message Header (all messages start with this)
#pragma pack(push, 1)
            struct MessageHeader
            {
                uint16_t size; // Total message size including header
                uint16_t type; // MessageType enum value

                MessageHeader() : size(0), type(0) {}
                MessageHeader(uint16_t msg_size, MessageType msg_type)
                    : size(msg_size), type(static_cast<uint16_t>(msg_type)) {}
            };
#pragma pack(pop)

            // Base class for all DTC messages
            class DTCMessage
            {
            public:
                virtual ~DTCMessage() = default;
                virtual MessageType get_type() const = 0;
                virtual uint16_t get_size() const = 0;
                virtual std::vector<uint8_t> serialize() const = 0;
                virtual bool deserialize(const uint8_t *data, uint16_t size) = 0;

                // Helper to get message header
                MessageHeader get_header() const
                {
                    return MessageHeader(get_size(), get_type());
                }
            };

            // Logon Request Message
            class LogonRequest : public DTCMessage
            {
            public:
                uint16_t protocol_version = DTC_PROTOCOL_VERSION;
                std::string username;
                std::string password;
                std::string general_text_data;
                std::string integer_1;
                std::string integer_2;
                uint8_t heartbeat_interval_in_seconds = 0;
                uint8_t unused_1 = 0;
                std::string trade_account;
                std::string hardware_identifier;
                std::string client_name;

                MessageType get_type() const override { return MessageType::LOGON_REQUEST; }
                uint16_t get_size() const override;
                std::vector<uint8_t> serialize() const override;
                bool deserialize(const uint8_t *data, uint16_t size) override;
            };

            // Logon Response Message
            class LogonResponse : public DTCMessage
            {
            public:
                uint16_t protocol_version = DTC_PROTOCOL_VERSION;
                uint8_t result = 0; // 1 = success, 0 = failure
                std::string result_text;
                std::string reconnect_address;
                uint16_t integer_1 = 0;
                std::string server_name;
                uint8_t market_depth_updates_best_bid_and_ask = 1;
                uint8_t trading_is_supported = 1;
                uint8_t order_cancel_replace_supported = 1;
                std::string symbol_exchange_delimiter;
                uint8_t security_definitions_supported = 1;
                uint8_t historical_price_data_supported = 0;
                uint8_t resubscribe_when_market_data_feed_available = 1;
                uint8_t market_depth_is_supported = 1;
                uint8_t one_historical_price_data_request_per_connection = 0;
                uint8_t use_integer_price_order_messages = 0;
                uint8_t bracket_order_supported = 0;
                uint8_t use_lookup_table_for_order_id = 0;

                MessageType get_type() const override { return MessageType::LOGON_RESPONSE; }
                uint16_t get_size() const override;
                std::vector<uint8_t> serialize() const override;
                bool deserialize(const uint8_t *data, uint16_t size) override;
            };

            // Heartbeat Message
            class Heartbeat : public DTCMessage
            {
            public:
                uint32_t num_drops = 0;
                uint64_t current_date_time = 0;

                MessageType get_type() const override { return MessageType::HEARTBEAT; }
                uint16_t get_size() const override;
                std::vector<uint8_t> serialize() const override;
                bool deserialize(const uint8_t *data, uint16_t size) override;
            };

            // Logoff Message
            class Logoff : public DTCMessage
            {
            public:
                std::string reason;
                uint8_t do_not_reconnect = 0;

                MessageType get_type() const override { return MessageType::LOGOFF; }
                uint16_t get_size() const override;
                std::vector<uint8_t> serialize() const override;
                bool deserialize(const uint8_t *data, uint16_t size) override;
            };

            // Market Data Request Message
            class MarketDataRequest : public DTCMessage
            {
            public:
                RequestAction request_action = RequestAction::SUBSCRIBE;
                uint16_t symbol_id = 0;
                std::string symbol;
                std::string exchange;

                MessageType get_type() const override { return MessageType::MARKET_DATA_REQUEST; }
                uint16_t get_size() const override;
                std::vector<uint8_t> serialize() const override;
                bool deserialize(const uint8_t *data, uint16_t size) override;
            };

            // Market Data Reject Message
            class MarketDataReject : public DTCMessage
            {
            public:
                uint16_t symbol_id = 0;
                std::string reject_text;

                MessageType get_type() const override { return MessageType::MARKET_DATA_REJECT; }
                uint16_t get_size() const override;
                std::vector<uint8_t> serialize() const override;
                bool deserialize(const uint8_t *data, uint16_t size) override;
            };

            // Market Data Update Trade Message
            class MarketDataUpdateTrade : public DTCMessage
            {
            public:
                uint16_t symbol_id = 0;
                double at_bid_or_ask = 0;
                double price = 0.0;
                double volume = 0.0;
                uint64_t date_time = 0;

                MessageType get_type() const override { return MessageType::MARKET_DATA_UPDATE_TRADE; }
                uint16_t get_size() const override;
                std::vector<uint8_t> serialize() const override;
                bool deserialize(const uint8_t *data, uint16_t size) override;
            };

            // Market Data Update Bid Ask Message
            class MarketDataUpdateBidAsk : public DTCMessage
            {
            public:
                uint16_t symbol_id = 0;
                double bid_price = 0.0;
                float bid_quantity = 0.0f;
                double ask_price = 0.0;
                float ask_quantity = 0.0f;
                uint64_t date_time = 0;
                uint8_t is_bid_change = 0;
                uint8_t is_ask_change = 0;

                MessageType get_type() const override { return MessageType::MARKET_DATA_UPDATE_BID_ASK; }
                uint16_t get_size() const override;
                std::vector<uint8_t> serialize() const override;
                bool deserialize(const uint8_t *data, uint16_t size) override;
            };

            // Submit New Single Order Message
            class SubmitNewSingleOrder : public DTCMessage
            {
            public:
                std::string symbol;
                std::string exchange;
                std::string trade_account;
                std::string client_order_id;
                OrderTypeEnum order_type = OrderTypeEnum::ORDER_TYPE_UNSET;
                BuySellEnum buy_sell = BuySellEnum::BUY_SELL_UNSET;
                double price1 = 0.0;
                double price2 = 0.0; // For stop-limit orders
                double quantity = 0.0;
                TimeInForceEnum time_in_force = TimeInForceEnum::TIF_UNSET;
                uint64_t good_till_date_time = 0;
                uint8_t is_automated_order = 0;
                uint8_t is_parent_order = 0;
                std::string free_form_text;

                MessageType get_type() const override { return MessageType::SUBMIT_NEW_SINGLE_ORDER; }
                uint16_t get_size() const override;
                std::vector<uint8_t> serialize() const override;
                bool deserialize(const uint8_t *data, uint16_t size) override;
            };

            // Order Update Message
            class OrderUpdate : public DTCMessage
            {
            public:
                uint32_t request_id = 0;
                int32_t total_num_messages = 0;
                int32_t message_number = 0;
                std::string symbol;
                std::string exchange;
                std::string previous_server_order_id;
                std::string server_order_id;
                std::string client_order_id;
                std::string exchange_order_id;
                OrderStatusEnum order_status = OrderStatusEnum::ORDER_STATUS_UNSPECIFIED;
                OrderTypeEnum order_type = OrderTypeEnum::ORDER_TYPE_UNSET;
                BuySellEnum buy_sell = BuySellEnum::BUY_SELL_UNSET;
                double price1 = 0.0;
                double price2 = 0.0;
                double order_quantity = 0.0;
                double filled_quantity = 0.0;
                double remaining_quantity = 0.0;
                double average_fill_price = 0.0;
                double last_fill_price = 0.0;
                double last_fill_quantity = 0.0;
                uint64_t last_fill_date_time = 0;
                uint64_t order_received_date_time = 0;
                TimeInForceEnum time_in_force = TimeInForceEnum::TIF_UNSET;
                uint64_t good_till_date_time = 0;
                uint32_t order_update_sequence_number = 0;
                std::string free_form_text;
                std::string order_id;
                std::string trade_account;
                std::string info_text;
                uint8_t no_orders = 0;
                std::string parent_server_order_id;
                std::string oco_linked_order_server_order_id;

                MessageType get_type() const override { return MessageType::ORDER_UPDATE; }
                uint16_t get_size() const override;
                std::vector<uint8_t> serialize() const override;
                bool deserialize(const uint8_t *data, uint16_t size) override;
            };

            // Open Orders Request Message
            class OpenOrdersRequest : public DTCMessage
            {
            public:
                uint32_t request_id = 0;
                int32_t request_all_orders = 1;
                std::string server_order_id;
                std::string trade_account;

                MessageType get_type() const override { return MessageType::OPEN_ORDERS_REQUEST; }
                uint16_t get_size() const override;
                std::vector<uint8_t> serialize() const override;
                bool deserialize(const uint8_t *data, uint16_t size) override;
            };

            // Current Positions Request Message
            class CurrentPositionsRequest : public DTCMessage
            {
            public:
                uint32_t request_id = 0;
                std::string trade_account;

                MessageType get_type() const override { return MessageType::CURRENT_POSITIONS_REQUEST; }
                uint16_t get_size() const override;
                std::vector<uint8_t> serialize() const override;
                bool deserialize(const uint8_t *data, uint16_t size) override;
            };

            // Security Definition Request Message
            class SecurityDefinitionForSymbolRequest : public DTCMessage
            {
            public:
                uint32_t request_id = 0;
                std::string symbol;
                std::string exchange;

                MessageType get_type() const override { return MessageType::SECURITY_DEFINITION_FOR_SYMBOL_REQUEST; }
                uint16_t get_size() const override;
                std::vector<uint8_t> serialize() const override;
                bool deserialize(const uint8_t *data, uint16_t size) override;
            };

            // Security Definition Response Message
            class SecurityDefinitionResponse : public DTCMessage
            {
            public:
                uint32_t request_id = 0;
                std::string symbol;
                std::string exchange;
                uint8_t security_type = 0; // 0 = Unknown, 1 = Future, 2 = Stock, 3 = Forex, 4 = Index, 5 = Option
                std::string description;
                float min_price_increment = 0.0f;
                uint8_t price_display_format = 0;
                float currency_value_per_increment = 0.0f;
                uint8_t has_market_depth_data = 1;
                float display_price_multiplier = 1.0f;
                std::string exchange_symbol;
                float initial_margin_requirement = 0.0f;
                float maintenance_margin_requirement = 0.0f;
                std::string currency;
                float contract_size = 1.0f;
                uint32_t open_interest = 0;
                uint64_t roll_over_date = 0;
                uint8_t is_delayed = 0;

                MessageType get_type() const override { return MessageType::SECURITY_DEFINITION_RESPONSE; }
                uint16_t get_size() const override;
                std::vector<uint8_t> serialize() const override;
                bool deserialize(const uint8_t *data, uint16_t size) override;
            };

            // Protocol Handler Class
            class Protocol
            {
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
                std::unique_ptr<DTCMessage> parse_message(const uint8_t *data, uint16_t size);
                std::vector<uint8_t> create_message(const DTCMessage &message);

                // Protocol helpers - factory methods for common messages
                std::unique_ptr<LogonResponse> create_logon_response(bool success, const std::string &message = "");
                std::unique_ptr<MarketDataUpdateTrade> create_trade_update(
                    uint16_t symbol_id, double price, double volume, uint64_t timestamp);
                std::unique_ptr<MarketDataUpdateBidAsk> create_bid_ask_update(
                    uint16_t symbol_id, double bid_price, float bid_qty,
                    double ask_price, float ask_qty, uint64_t timestamp);
                std::unique_ptr<Heartbeat> create_heartbeat(uint32_t num_drops = 0);
                std::unique_ptr<SecurityDefinitionResponse> create_security_definition_response(
                    uint32_t request_id, const std::string &symbol, const std::string &exchange);

                // Utility functions
                static uint64_t get_current_timestamp();
                static MessageType get_message_type(const uint8_t *data, uint16_t size);
                static bool validate_message_header(const uint8_t *data, uint16_t size);
                static std::string message_type_to_string(MessageType type);

                // String handling helpers for DTC variable-length string fields
                static std::string read_dtc_string(const uint8_t *data, uint16_t &offset, uint16_t max_size);
                static void write_dtc_string(std::vector<uint8_t> &buffer, const std::string &str);
            };

        } // namespace dtc
    } // namespace core
} // namespace coinbase_dtc_core"