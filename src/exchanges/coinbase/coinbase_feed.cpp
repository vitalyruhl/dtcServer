#include "coinbase_dtc_core/exchanges/coinbase/coinbase_feed.hpp"
#include "coinbase_dtc_core/exchanges/coinbase/websocket_client.hpp" // Re-enabled
#include "coinbase_dtc_core/core/util/log.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace open_dtc_server
{
    namespace exchanges
    {
        namespace coinbase
        {

            CoinbaseFeed::CoinbaseFeed(const base::ExchangeConfig &config)
                : base::ExchangeFeedBase(config)
            {
                util::log("[COINBASE] Coinbase feed initialized with config: " + config.name);
            }

            CoinbaseFeed::~CoinbaseFeed()
            {
                disconnect();
                util::log("[COINBASE] Coinbase feed destroyed");
            }

            bool CoinbaseFeed::connect()
            {
                if (is_connected())
                {
                    util::log("[COINBASE] Already connected");
                    return true;
                }

                try
                {
                    util::log("[COINBASE] Connecting to WebSocket: " + config_.websocket_url);

                    // Use real WebSocket connection with our WebSocketClient
                    websocket_client_ = std::make_unique<feed::coinbase::WebSocketClient>();

                    // Set up callbacks
                    websocket_client_->set_trade_callback([this](const exchanges::base::MarketTrade &trade)
                                                          { this->on_trade_received(trade); });

                    websocket_client_->set_level2_callback([this](const exchanges::base::MarketLevel2 &level2)
                                                           { this->on_level2_received(level2); });

                    // Connect to WebSocket
                    bool ws_connected = websocket_client_->connect("ws-feed.exchange.coinbase.com", 443);
                    if (!ws_connected)
                    {
                        util::log("[ERROR] Failed to establish WebSocket connection to Coinbase");
                        return false;
                    }

                    connected_.store(true);
                    util::log("[SUCCESS] Connected to Coinbase WebSocket feed at ws-feed.exchange.coinbase.com");
                    notify_connection(true);
                    return true;
                }
                catch (const std::exception &e)
                {
                    util::log("[COINBASE] Connection failed: " + std::string(e.what()));
                    notify_error("Connection failed: " + std::string(e.what()));
                    return false;
                }
            }

            void CoinbaseFeed::disconnect()
            {
                if (!is_connected())
                {
                    return;
                }

                util::log("[COINBASE] Disconnecting...");

                // Disconnect WebSocket client
                if (websocket_client_)
                {
                    websocket_client_->disconnect();
                    websocket_client_.reset();
                }

                connected_.store(false);

                std::lock_guard<std::mutex> lock(subscriptions_mutex_);
                subscriptions_.clear();

                util::log("[COINBASE] Disconnected");
                notify_connection(false);
            }

            bool CoinbaseFeed::is_connected() const
            {
                return connected_.load();
            }

            bool CoinbaseFeed::subscribe_trades(const std::string &symbol)
            {
                if (!is_connected())
                {
                    util::log("[COINBASE] Cannot subscribe - not connected");
                    return false;
                }

                std::string coinbase_symbol = exchange_symbol(symbol);

                std::lock_guard<std::mutex> lock(subscriptions_mutex_);
                subscriptions_[symbol] = SubscriptionInfo(SubscriptionType::TRADES, coinbase_symbol);
                subscriptions_[symbol].active = true;

                util::log("[COINBASE] Subscribed to trades for " + symbol + " (Coinbase: " + coinbase_symbol + ")");

                // Send subscription to WebSocket client
                if (websocket_client_)
                {
                    websocket_client_->subscribe_trades(coinbase_symbol);
                }

                return true;
            }

            bool CoinbaseFeed::subscribe_level2(const std::string &symbol)
            {
                if (!is_connected())
                {
                    util::log("[COINBASE] Cannot subscribe - not connected");
                    return false;
                }

                std::string coinbase_symbol = exchange_symbol(symbol);

                std::lock_guard<std::mutex> lock(subscriptions_mutex_);
                subscriptions_[symbol + "_level2"] = SubscriptionInfo(SubscriptionType::LEVEL2, coinbase_symbol);
                subscriptions_[symbol + "_level2"].active = true;

                util::log("[COINBASE] Subscribed to level2 for " + symbol + " (Coinbase: " + coinbase_symbol + ")");

                // Send subscription to WebSocket client
                if (websocket_client_)
                {
                    websocket_client_->subscribe_level2(coinbase_symbol);
                }

                return true;
            }

            bool CoinbaseFeed::unsubscribe(const std::string &symbol)
            {
                std::lock_guard<std::mutex> lock(subscriptions_mutex_);

                auto it = subscriptions_.find(symbol);
                if (it != subscriptions_.end())
                {
                    std::string coinbase_symbol = it->second.product_id;
                    subscriptions_.erase(it);

                    util::log("[COINBASE] Unsubscribed from " + symbol + " (Coinbase: " + coinbase_symbol + ")");

                    // TODO: Send actual unsubscribe message to Coinbase WebSocket
                    return true;
                }

                return false;
            }

            bool CoinbaseFeed::subscribe_multiple_symbols(const std::vector<std::string> &symbols)
            {
                bool all_success = true;
                for (const auto &symbol : symbols)
                {
                    if (!subscribe_trades(symbol) || !subscribe_level2(symbol))
                    {
                        all_success = false;
                    }
                }
                return all_success;
            }

            std::string CoinbaseFeed::normalize_symbol(const std::string &exchange_symbol)
            {
                // Convert Coinbase format (BTC-USD) to normalized format (BTC/USD)
                std::string result = exchange_symbol;
                std::replace(result.begin(), result.end(), '-', '/');
                return result;
            }

            std::string CoinbaseFeed::exchange_symbol(const std::string &normalized_symbol)
            {
                // Convert normalized format (BTC/USD) to Coinbase format (BTC-USD)
                std::string result = normalized_symbol;
                std::replace(result.begin(), result.end(), '/', '-');

                // Ensure uppercase
                std::transform(result.begin(), result.end(), result.begin(), ::toupper);

                return result;
            }

            std::vector<std::string> CoinbaseFeed::get_available_symbols()
            {
                // TODO: Fetch from Coinbase API
                // For now, return some common symbols
                return {"BTC/USD", "ETH/USD", "LTC/USD", "BCH/USD"};
            }

            std::string CoinbaseFeed::get_status() const
            {
                std::ostringstream ss;
                ss << "Coinbase Feed Status:\n";
                ss << "  Connected: " << (is_connected() ? "Yes" : "No") << "\n";

                std::lock_guard<std::mutex> lock(subscriptions_mutex_);
                ss << "  Subscriptions: " << subscriptions_.size() << "\n";

                for (const auto &[key, sub] : subscriptions_)
                {
                    std::string type_str = (sub.type == SubscriptionType::TRADES) ? "trades" : "level2";
                    ss << "    " << sub.product_id << " (" << type_str << ")\n";
                }

                return ss.str();
            }

            std::vector<std::string> CoinbaseFeed::get_subscribed_symbols() const
            {
                std::vector<std::string> symbols;
                std::lock_guard<std::mutex> lock(subscriptions_mutex_);

                for (const auto &[key, sub] : subscriptions_)
                {
                    if (sub.type == SubscriptionType::TRADES)
                    {
                        // Convert Coinbase format to normalized format inline
                        std::string result = sub.product_id;
                        std::replace(result.begin(), result.end(), '-', '/');
                        symbols.push_back(result);
                    }
                }

                return symbols;
            }

            void CoinbaseFeed::on_trade_received(const exchanges::base::MarketTrade &trade)
            {
                // Process received trade data
                util::log("[COINBASE] Trade received: " + trade.symbol + " - " + std::to_string(trade.price) + " @ " + std::to_string(trade.volume));

                // Forward to base class for distribution to clients
                notify_trade(trade);
            }
            void CoinbaseFeed::on_level2_received(const exchanges::base::MarketLevel2 &level2)
            {
                // Process received level2 data
                util::log("[COINBASE] Level2 received: " + level2.symbol + " - Bid: " + std::to_string(level2.bid_price) + " Ask: " + std::to_string(level2.ask_price));

                // Forward to base class for distribution to clients
                notify_level2(level2);
            }

        } // namespace coinbase
    } // namespace exchanges
} // namespace open_dtc_server