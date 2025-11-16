#include "coinbase_dtc_core/exchanges/coinbase/coinbase_feed.hpp"
#include "coinbase_dtc_core/exchanges/coinbase/websocket_client.hpp"     // Re-enabled
#include "coinbase_dtc_core/exchanges/coinbase/ssl_websocket_client.hpp" // NEW: SSL WebSocket client
#include "coinbase_dtc_core/core/util/log.hpp"
#include <nlohmann/json.hpp> // For JSON parsing
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
                util::simple_log("[COINBASE] Coinbase feed initialized with config: " + config.name);
            }

            CoinbaseFeed::~CoinbaseFeed()
            {
                disconnect();
                util::simple_log("[COINBASE] Coinbase feed destroyed");
            }

            bool CoinbaseFeed::connect()
            {
                if (is_connected())
                {
                    util::simple_log("[COINBASE] Already connected");
                    return true;
                }

                try
                {
                    util::simple_log("[COINBASE] Connecting to Secure WebSocket: " + config_.websocket_url);

                    // Choose between SSL WebSocket (for authenticated feeds) or plain WebSocket
                    bool use_ssl = config_.websocket_url.find("wss://") == 0 || config_.websocket_url.find("443") != std::string::npos;

                    if (use_ssl)
                    {
                        util::simple_log("[COINBASE] Using SSL WebSocket client with JWT authentication");
                        ssl_websocket_client_ = std::make_unique<feed::coinbase::SSLWebSocketClient>();

                        // Set up callbacks for SSL client
                        ssl_websocket_client_->set_message_callback([this](const std::string &message)
                                                                    { this->on_websocket_message_received(message); });

                        ssl_websocket_client_->set_connection_callback([this](bool connected)
                                                                       {
                            if (connected) {
                                this->notify_connection(true);
                                // Authenticate and subscribe after connection
                                ssl_websocket_client_->authenticate_with_jwt();
                                
                                // Start with basic subscriptions for common symbols
                                ssl_websocket_client_->subscribe_to_ticker({"BTC-USD"});
                                util::simple_log("[COINBASE] SSL WebSocket: Auto-subscribed to BTC-USD ticker for testing");
                            } else {
                                this->notify_connection(false);
                            } });

                        // Connect to SSL WebSocket (Coinbase Advanced Trade)
                        bool ws_connected = ssl_websocket_client_->connect("ws-feed.exchange.coinbase.com", 443);
                        if (!ws_connected)
                        {
                            util::simple_log("[ERROR] Failed to establish SSL WebSocket connection to Coinbase");
                            return false;
                        }
                    }
                    else
                    {
                        util::simple_log("[COINBASE] Using plain WebSocket client (public data only)");
                        // Use plain WebSocket connection with our WebSocketClient
                        websocket_client_ = std::make_unique<feed::coinbase::WebSocketClient>();

                        // Set up callbacks
                        websocket_client_->set_trade_callback([this](const exchanges::base::MarketTrade &trade)
                                                              { this->on_trade_received(trade); });

                        websocket_client_->set_level2_callback([this](const exchanges::base::MarketLevel2 &level2)
                                                               { this->on_level2_received(level2); });

                        // Connect to plain WebSocket
                        bool ws_connected = websocket_client_->connect("ws-feed.exchange.coinbase.com", 80);
                        if (!ws_connected)
                        {
                            util::simple_log("[ERROR] Failed to establish WebSocket connection to Coinbase");
                            return false;
                        }
                    }

                    connected_.store(true);
                    util::simple_log("[SUCCESS] Connected to Coinbase WebSocket feed at ws-feed.exchange.coinbase.com");
                    notify_connection(true);
                    return true;
                }
                catch (const std::exception &e)
                {
                    util::simple_log("[COINBASE] Connection failed: " + std::string(e.what()));
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

                util::simple_log("[COINBASE] Disconnecting...");

                // Disconnect SSL WebSocket client
                if (ssl_websocket_client_)
                {
                    ssl_websocket_client_->disconnect();
                    ssl_websocket_client_.reset();
                }

                // Disconnect plain WebSocket client
                if (websocket_client_)
                {
                    websocket_client_->disconnect();
                    websocket_client_.reset();
                }

                connected_.store(false);

                std::lock_guard<std::mutex> lock(subscriptions_mutex_);
                subscriptions_.clear();

                util::simple_log("[COINBASE] Disconnected");
                notify_connection(false);
            }

            void CoinbaseFeed::set_credentials(const std::string &api_key_id, const std::string &private_key)
            {
                if (ssl_websocket_client_)
                {
                    ssl_websocket_client_->set_credentials(api_key_id, private_key);
                    util::simple_log("[COINBASE] Credentials passed to SSL WebSocket client");
                }
                else
                {
                    util::simple_log("[WARNING] Cannot set credentials - SSL WebSocket client not initialized");
                }
            }

            bool CoinbaseFeed::is_connected() const
            {
                return connected_.load();
            }

            bool CoinbaseFeed::subscribe_trades(const std::string &symbol)
            {
                if (!is_connected())
                {
                    util::simple_log("[COINBASE] Cannot subscribe - not connected");
                    return false;
                }

                std::string coinbase_symbol = exchange_symbol(symbol);

                std::lock_guard<std::mutex> lock(subscriptions_mutex_);
                subscriptions_[symbol] = SubscriptionInfo(SubscriptionType::TRADES, coinbase_symbol);
                subscriptions_[symbol].active = true;

                util::simple_log("[COINBASE] Subscribed to trades for " + symbol + " (Coinbase: " + coinbase_symbol + ")");

                // Send subscription to WebSocket client
                if (websocket_client_)
                {
                    websocket_client_->subscribe_trades(coinbase_symbol);
                }
                else if (ssl_websocket_client_)
                {
                    // Use SSL WebSocket for ticker data (includes trade info)
                    ssl_websocket_client_->subscribe_to_ticker({coinbase_symbol});
                }

                return true;
            }

            bool CoinbaseFeed::subscribe_level2(const std::string &symbol)
            {
                if (!is_connected())
                {
                    util::simple_log("[COINBASE] Cannot subscribe - not connected");
                    return false;
                }

                std::string coinbase_symbol = exchange_symbol(symbol);

                std::lock_guard<std::mutex> lock(subscriptions_mutex_);
                subscriptions_[symbol + "_level2"] = SubscriptionInfo(SubscriptionType::LEVEL2, coinbase_symbol);
                subscriptions_[symbol + "_level2"].active = true;

                util::simple_log("[COINBASE] Subscribed to level2 for " + symbol + " (Coinbase: " + coinbase_symbol + ")");

                // Send subscription to WebSocket client
                if (websocket_client_)
                {
                    websocket_client_->subscribe_level2(coinbase_symbol);
                }
                else if (ssl_websocket_client_)
                {
                    // Use SSL WebSocket for level2 data
                    ssl_websocket_client_->subscribe_to_level2({coinbase_symbol});
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

                    util::simple_log("[COINBASE] Unsubscribed from " + symbol + " (Coinbase: " + coinbase_symbol + ")");

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
                util::log_debug("[COINBASE] Trade received: " + trade.symbol + " - " + std::to_string(trade.price) + " @ " + std::to_string(trade.volume));

                // Forward to base class for distribution to clients
                notify_trade(trade);
            }
            void CoinbaseFeed::on_level2_received(const exchanges::base::MarketLevel2 &level2)
            {
                // Process received level2 data
                util::log_debug("[COINBASE] Level2 received: " + level2.symbol + " - Bid: " + std::to_string(level2.bid_price) + " Ask: " + std::to_string(level2.ask_price));

                // Forward to base class for distribution to clients
                notify_level2(level2);
            }

            void CoinbaseFeed::on_websocket_message_received(const std::string &message)
            {
                // Process raw WebSocket message from SSL client
                util::log_debug("[COINBASE] SSL WebSocket message received: " + message.substr(0, 100) + "...");

                try
                {
                    // Parse JSON message
                    nlohmann::json json_message = nlohmann::json::parse(message);

                    if (json_message.contains("type"))
                    {
                        std::string message_type = json_message["type"];

                        if (message_type == "ticker")
                        {
                            // Handle ticker message
                            handle_ticker_message(message);
                        }
                        else if (message_type == "match")
                        {
                            // Handle trade message
                            handle_trade_message(message);
                        }
                        else if (message_type == "l2update")
                        {
                            // Handle level2 update
                            handle_level2_message(message);
                        }
                        else if (message_type == "heartbeat")
                        {
                            // Handle heartbeat
                            handle_heartbeat_message(message);
                        }
                        else if (message_type == "subscriptions")
                        {
                            // Handle subscription confirmation from Coinbase
                            handle_subscriptions_message(message);
                        }
                        else if (message_type == "error")
                        {
                            // Handle error
                            handle_error_message(message);
                        }
                        else
                        {
                            util::simple_log("[COINBASE] Unknown message type: " + message_type);
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    util::simple_log("[ERROR] Failed to parse SSL WebSocket message: " + std::string(e.what()));
                }
            }

            void CoinbaseFeed::handle_trade_message(const std::string &message)
            {
                try
                {
                    nlohmann::json json = nlohmann::json::parse(message);

                    if (json.contains("product_id") && json.contains("price") && json.contains("size"))
                    {
                        std::string product_id = json["product_id"];
                        double price = std::stod(json["price"].get<std::string>());
                        double size = std::stod(json["size"].get<std::string>());

                        // Convert to DTC format
                        exchanges::base::MarketTrade trade;
                        trade.symbol = product_id;
                        trade.price = price;
                        trade.volume = size;
                        trade.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                              std::chrono::system_clock::now().time_since_epoch())
                                              .count();

                        // Forward to base class
                        on_trade_received(trade);
                    }
                }
                catch (const std::exception &e)
                {
                    util::simple_log("[ERROR] Failed to parse trade message: " + std::string(e.what()));
                }
            }

            void CoinbaseFeed::handle_level2_message(const std::string &message)
            {
                try
                {
                    nlohmann::json json = nlohmann::json::parse(message);

                    if (json.contains("product_id") && json.contains("changes"))
                    {
                        std::string product_id = json["product_id"];
                        auto changes = json["changes"];

                        for (const auto &change : changes)
                        {
                            if (change.size() >= 3)
                            {
                                std::string side = change[0]; // "buy" or "sell"
                                double price = std::stod(change[1].get<std::string>());
                                double size = std::stod(change[2].get<std::string>());

                                // Convert to DTC format
                                exchanges::base::MarketLevel2 level2;
                                level2.symbol = product_id;

                                if (side == "buy")
                                {
                                    level2.bid_price = price;
                                    level2.bid_size = size;
                                    level2.ask_price = 0.0;
                                    level2.ask_size = 0.0;
                                }
                                else
                                {
                                    level2.bid_price = 0.0;
                                    level2.bid_size = 0.0;
                                    level2.ask_price = price;
                                    level2.ask_size = size;
                                }

                                level2.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                       std::chrono::system_clock::now().time_since_epoch())
                                                       .count();

                                // Forward to base class
                                on_level2_received(level2);
                            }
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    util::simple_log("[ERROR] Failed to parse level2 message: " + std::string(e.what()));
                }
            }

            void CoinbaseFeed::process_websocket_message(const std::string &message)
            {
                try
                {
                    // Skip empty messages
                    if (message.empty())
                    {
                        return;
                    }

                    // Check if message looks like binary data (contains non-printable characters early on)
                    if (message.length() > 0 && (static_cast<unsigned char>(message[0]) > 127 ||
                                                 (message.length() > 1 && static_cast<unsigned char>(message[1]) > 127)))
                    {
                        util::simple_log("[COINBASE] Skipping binary/compressed WebSocket frame");
                        return;
                    }

                    // Skip if it doesn't start with JSON-like characters
                    if (message[0] != '{' && message[0] != '[')
                    {
                        util::simple_log("[COINBASE] Skipping non-JSON WebSocket message");
                        return;
                    }

                    nlohmann::json json = nlohmann::json::parse(message);

                    if (json.contains("product_id") && json.contains("price"))
                    {
                        std::string product_id = json["product_id"];
                        double price = std::stod(json["price"].get<std::string>());

                        util::log_debug("[COINBASE] Ticker update: " + product_id + " = $" + std::to_string(price));

                        // Convert ticker to trade format for DTC
                        exchanges::base::MarketTrade trade;
                        trade.symbol = product_id;
                        trade.price = price;
                        trade.volume = json.contains("last_size") ? std::stod(json["last_size"].get<std::string>()) : 1.0;
                        trade.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                              std::chrono::system_clock::now().time_since_epoch())
                                              .count();

                        // Forward to DTC clients
                        on_trade_received(trade);

                        // Also create level2 data from ticker if bid/ask available
                        if (json.contains("best_bid") && json.contains("best_ask"))
                        {
                            exchanges::base::MarketLevel2 level2;
                            level2.symbol = product_id;
                            level2.bid_price = std::stod(json["best_bid"].get<std::string>());
                            level2.ask_price = std::stod(json["best_ask"].get<std::string>());
                            level2.bid_size = json.contains("best_bid_size") ? std::stod(json["best_bid_size"].get<std::string>()) : 1.0;
                            level2.ask_size = json.contains("best_ask_size") ? std::stod(json["best_ask_size"].get<std::string>()) : 1.0;
                            level2.timestamp = trade.timestamp;

                            // Forward to DTC clients
                            on_level2_received(level2);
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    util::simple_log("[ERROR] Failed to parse ticker message: " + std::string(e.what()));
                }
            }

            void CoinbaseFeed::handle_ticker_message(const std::string &message)
            {
                try
                {
                    nlohmann::json json = nlohmann::json::parse(message);

                    if (json.contains("product_id") && json.contains("price"))
                    {
                        std::string product_id = json["product_id"];
                        double price = std::stod(json["price"].get<std::string>());

                        // Log ticker updates occasionally
                        static int ticker_count = 0;
                        ticker_count++;
                        if (ticker_count % 100 == 0) // Log every 100th ticker
                        {
                            util::simple_log("[COINBASE] Ticker " + product_id + ": $" + std::to_string(price));
                        }

                        // Could forward to DTC clients for price updates
                        // For now, just acknowledge we received it
                    }
                }
                catch (const std::exception &e)
                {
                    util::simple_log("[ERROR] Failed to parse ticker message: " + std::string(e.what()));
                }
            }

            void CoinbaseFeed::handle_heartbeat_message(const std::string &message)
            {
                util::simple_log("[COINBASE] Heartbeat received - connection alive");

                // Update heartbeat timestamp for connection monitoring
                // Could be used for reconnection logic
            }

            void CoinbaseFeed::handle_error_message(const std::string &message)
            {
                try
                {
                    nlohmann::json json = nlohmann::json::parse(message);

                    std::string error_msg = "Coinbase WebSocket Error";
                    if (json.contains("message"))
                    {
                        error_msg = json["message"];
                    }

                    util::simple_log("[ERROR] Coinbase WebSocket: " + error_msg);

                    // Forward error to base class
                    notify_error(error_msg);
                }
                catch (const std::exception &e)
                {
                    util::simple_log("[ERROR] Failed to parse error message: " + std::string(e.what()));
                }
            }

            void CoinbaseFeed::handle_subscriptions_message(const std::string &message)
            {
                try
                {
                    nlohmann::json json = nlohmann::json::parse(message);

                    // This is a confirmation message from Coinbase about active subscriptions
                    util::simple_log("[COINBASE] Subscription confirmation received");

                    // Log the channels we're subscribed to
                    if (json.contains("channels"))
                    {
                        for (const auto &channel : json["channels"])
                        {
                            if (channel.contains("name"))
                            {
                                std::string channel_name = channel["name"];
                                util::simple_log("[COINBASE] Subscribed to channel: " + channel_name);

                                if (channel.contains("product_ids"))
                                {
                                    for (const auto &product : channel["product_ids"])
                                    {
                                        util::simple_log("[COINBASE] - Product: " + product.get<std::string>());
                                    }
                                }
                            }
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    util::simple_log("[ERROR] Failed to parse subscriptions message: " + std::string(e.what()));
                }
            }

        } // namespace coinbase
    } // namespace exchanges
} // namespace open_dtc_server
