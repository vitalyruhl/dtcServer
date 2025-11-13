#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <mutex>

namespace open_dtc_server
{
    namespace exchanges
    {
        namespace base
        {

            // Exchange-agnostic market data structures
            struct MarketTrade
            {
                std::string symbol;   // Normalized symbol (e.g., "BTC/USD")
                std::string exchange; // Exchange name (e.g., "coinbase")
                double price;
                double volume;
                std::string side; // "buy" or "sell"
                uint64_t timestamp;
                std::string trade_id;

                MarketTrade() : price(0.0), volume(0.0), timestamp(0) {}
            };

            struct MarketLevel2
            {
                std::string symbol;   // Normalized symbol
                std::string exchange; // Exchange name
                double bid_price;
                double bid_size;
                double ask_price;
                double ask_size;
                uint64_t timestamp;

                MarketLevel2() : bid_price(0.0), bid_size(0.0), ask_price(0.0), ask_size(0.0), timestamp(0) {}
            };

            // Exchange configuration
            struct ExchangeConfig
            {
                std::string name; // "coinbase", "binance", "kraken"
                std::string websocket_url;
                std::string api_url;
                uint16_t port;
                bool requires_auth;
                std::string api_key;
                std::string secret_key;
                std::string passphrase; // For Coinbase Pro

                ExchangeConfig() : port(443), requires_auth(false) {}
            };

            // Callback types for market data
            using TradeCallback = std::function<void(const MarketTrade &)>;
            using Level2Callback = std::function<void(const MarketLevel2 &)>;
            using ConnectionCallback = std::function<void(bool connected, const std::string &exchange)>;
            using ErrorCallback = std::function<void(const std::string &error, const std::string &exchange)>;

            /**
             * Abstract base class for exchange market data feeds.
             *
             * This is the TEMPLATE that developers copy when adding new exchanges.
             * Each exchange inherits from this and implements the pure virtual methods.
             */
            class ExchangeFeedBase
            {
            public:
                ExchangeFeedBase(const ExchangeConfig &config) : config_(config) {}
                virtual ~ExchangeFeedBase() = default;

                // ========================================================================
                // PURE VIRTUAL METHODS - MUST BE IMPLEMENTED BY EACH EXCHANGE
                // ========================================================================

                /** Connect to exchange WebSocket/API */
                virtual bool connect() = 0;

                /** Disconnect from exchange */
                virtual void disconnect() = 0;

                /** Check connection status */
                virtual bool is_connected() const = 0;

                /** Subscribe to trade data for a symbol */
                virtual bool subscribe_trades(const std::string &symbol) = 0;

                /** Subscribe to level2/orderbook data for a symbol */
                virtual bool subscribe_level2(const std::string &symbol) = 0;

                /** Unsubscribe from a symbol */
                virtual bool unsubscribe(const std::string &symbol) = 0;

                /** Subscribe to multiple symbols at once */
                virtual bool subscribe_multiple_symbols(const std::vector<std::string> &symbols) = 0;

                // ========================================================================
                // SYMBOL MAPPING - EXCHANGE SPECIFIC (OVERRIDE THESE)
                // ========================================================================

                /** Convert exchange symbol to normalized format (e.g., "BTC-USD" -> "BTC/USD") */
                virtual std::string normalize_symbol(const std::string &exchange_symbol) = 0;

                /** Convert normalized symbol to exchange format (e.g., "BTC/USD" -> "BTC-USD") */
                virtual std::string exchange_symbol(const std::string &normalized_symbol) = 0;

                /** Get list of available symbols on this exchange */
                virtual std::vector<std::string> get_available_symbols() = 0;

                // ========================================================================
                // STATUS AND MONITORING
                // ========================================================================

                /** Get detailed status information */
                virtual std::string get_status() const = 0;

                /** Get currently subscribed symbols */
                virtual std::vector<std::string> get_subscribed_symbols() const = 0;

                // ========================================================================
                // COMMON FUNCTIONALITY (SHARED BY ALL EXCHANGES)
                // ========================================================================

                // Callback management
                void set_trade_callback(TradeCallback callback) { trade_callback_ = callback; }
                void set_level2_callback(Level2Callback callback) { level2_callback_ = callback; }
                void set_connection_callback(ConnectionCallback callback) { connection_callback_ = callback; }
                void set_error_callback(ErrorCallback callback) { error_callback_ = callback; }

                // Configuration access
                const ExchangeConfig &get_config() const { return config_; }
                std::string get_exchange_name() const { return config_.name; }

            protected:
                // ========================================================================
                // HELPER METHODS FOR DERIVED CLASSES
                // ========================================================================

                /** Notify all listeners of new trade data */
                void notify_trade(const MarketTrade &trade)
                {
                    if (trade_callback_)
                        trade_callback_(trade);
                }

                /** Notify all listeners of new level2 data */
                void notify_level2(const MarketLevel2 &level2)
                {
                    if (level2_callback_)
                        level2_callback_(level2);
                }

                /** Notify connection status change */
                void notify_connection(bool connected)
                {
                    if (connection_callback_)
                        connection_callback_(connected, config_.name);
                }

                /** Notify error occurred */
                void notify_error(const std::string &error)
                {
                    if (error_callback_)
                        error_callback_(error, config_.name);
                }

                /** Exchange configuration */
                ExchangeConfig config_;

            private:
                // Callbacks
                TradeCallback trade_callback_;
                Level2Callback level2_callback_;
                ConnectionCallback connection_callback_;
                ErrorCallback error_callback_;
            };

            // Multi-exchange aggregator
            class MultiExchangeFeed
            {
            public:
                MultiExchangeFeed();
                ~MultiExchangeFeed();

                // Exchange management
                bool add_exchange(const ExchangeConfig &config);
                bool remove_exchange(const std::string &exchange_name);
                std::vector<std::string> get_active_exchanges() const;

                // Symbol subscription across exchanges
                bool subscribe_symbol(const std::string &symbol, const std::string &exchange = ""); // Empty = all exchanges
                bool unsubscribe_symbol(const std::string &symbol, const std::string &exchange = "");

                // Callbacks - aggregates from all exchanges
                void set_trade_callback(TradeCallback callback) { trade_callback_ = callback; }
                void set_level2_callback(Level2Callback callback) { level2_callback_ = callback; }

                // Status
                std::string get_status() const;
                size_t get_total_subscriptions() const;

            private:
                std::unordered_map<std::string, std::unique_ptr<ExchangeFeedBase>> exchanges_;
                mutable std::mutex exchanges_mutex_;

                TradeCallback trade_callback_;
                Level2Callback level2_callback_;

                void on_trade_data(const MarketTrade &trade);
                void on_level2_data(const MarketLevel2 &level2);
            };

        } // namespace base
    } // namespace exchanges
} // namespace open_dtc_server