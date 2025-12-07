#include "coinbase_dtc_core/exchanges/coinbase/coinbase_feed.hpp"
#include "coinbase_dtc_core/exchanges/coinbase/websocket_client.hpp"     // Re-enabled
#include "coinbase_dtc_core/exchanges/coinbase/ssl_websocket_client.hpp" // NEW: SSL WebSocket client
#include "coinbase_dtc_core/core/util/advanced_log.hpp"
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
                : base::ExchangeFeedBase(config),
                  connected_(false),
                  should_stop_(false),
                  websocket_connected_(false),
                  socket_fd_(-1),
                  websocket_host_(WEBSOCKET_HOST),
                  websocket_path_(WEBSOCKET_PATH),
                  websocket_port_(WEBSOCKET_PORT),
                  should_reconnect_(false),
                  last_successful_connection_(0),
                  reconnect_attempts_(0),
                  next_reconnect_time_(0),
                  messages_received_(0),
                  messages_sent_(0),
                  last_message_time_(0),
                  last_heartbeat_time_(0),
                  total_trades_received_(0),
                  total_level2_updates_(0),
                  connection_uptime_start_(0),
                  last_request_time_(0)
            {
                LOG_INFO("[COINBASE] Coinbase feed initialized with config: " + config.name);
            }

            CoinbaseFeed::~CoinbaseFeed()
            {
                disconnect();
                LOG_INFO("[COINBASE] Coinbase feed destroyed");
            }

            bool CoinbaseFeed::connect()
            {
                if (is_connected())
                {
                    LOG_INFO("[COINBASE] Already connected");
                    return true;
                }

                initialize_credentials();

                try
                {
                    LOG_INFO("[COINBASE] Connecting to Secure WebSocket: " + config_.websocket_url);

                    // Choose between SSL WebSocket (for authenticated feeds) or plain WebSocket
                    bool use_ssl = config_.websocket_url.find("wss://") == 0 || config_.websocket_url.find(":443") != std::string::npos;

                    // Parse host and port from config_.websocket_url
                    std::string host = "ws-feed.exchange.coinbase.com";
                    int port = use_ssl ? 443 : 80;
                    try
                    {
                        std::string url = config_.websocket_url;
                        // Strip scheme
                        if (url.rfind("wss://", 0) == 0)
                            url = url.substr(6);
                        else if (url.rfind("ws://", 0) == 0)
                            url = url.substr(5);
                        // Extract host[:port]
                        size_t slashPos = url.find('/');
                        std::string hostport = (slashPos == std::string::npos) ? url : url.substr(0, slashPos);
                        size_t colonPos = hostport.find(':');
                        if (colonPos != std::string::npos)
                        {
                            host = hostport.substr(0, colonPos);
                            std::string portStr = hostport.substr(colonPos + 1);
                            try
                            {
                                port = std::stoi(portStr);
                            }
                            catch (...)
                            {
                                port = use_ssl ? 443 : 80;
                            }
                        }
                        else
                        {
                            host = hostport;
                        }
                    }
                    catch (...)
                    {
                        // Keep defaults
                    }

                    // Force Advanced Trade WS host when legacy host is detected
                    if (host == std::string("ws-feed.exchange.coinbase.com"))
                    {
                        LOG_INFO("[COINBASE] Overriding legacy WS host to Advanced Trade: advanced-trade-ws.coinbase.com");
                        host = "advanced-trade-ws.coinbase.com";
                    }

                    if (use_ssl)
                    {
                        LOG_INFO("[COINBASE] Using SSL WebSocket client with JWT authentication");
                        ssl_websocket_client_ = std::make_unique<feed::coinbase::SSLWebSocketClient>();

                        configure_ssl_credentials();

                        // Set up callbacks for SSL client
                        ssl_websocket_client_->set_message_callback([this](const std::string &message)
                                                                    { this->on_websocket_message_received(message); });

                        ssl_websocket_client_->set_connection_callback([this](bool connected)
                                                                       {
                            if (connected) {
                                this->notify_connection(true);
                                // Authenticate and subscribe after connection
                                if (has_credentials()) {
                                    if (!ssl_websocket_client_->authenticate_with_jwt()) {
                                        LOG_INFO("[ERROR] SSL WebSocket authentication failed");
                                        this->notify_error("Coinbase SSL authentication failed");
                                    }
                                } else {
                                    LOG_INFO("[WARNING] No credentials available for SSL WebSocket authentication");
                                }
                                
                                // No auto-subscriptions on startup; await explicit server requests
                            } else {
                                this->notify_connection(false);
                            } });

                        // Connect to SSL WebSocket (Coinbase Advanced Trade)
                        bool ws_connected = ssl_websocket_client_->connect(host, static_cast<uint16_t>(port));
                        if (!ws_connected)
                        {
                            LOG_INFO("[ERROR] Failed to establish SSL WebSocket connection to Coinbase");
                            return false;
                        }
                    }
                    else
                    {
                        LOG_INFO("[COINBASE] Using plain WebSocket client (public data only)");
                        // Use plain WebSocket connection with our WebSocketClient
                        websocket_client_ = std::make_unique<feed::coinbase::WebSocketClient>();

                        // Set up callbacks
                        websocket_client_->set_trade_callback([this](const exchanges::base::MarketTrade &trade)
                                                              { this->on_trade_received(trade); });

                        websocket_client_->set_level2_callback([this](const exchanges::base::MarketLevel2 &level2)
                                                               { this->on_level2_received(level2); });

                        // Connect to plain WebSocket
                        bool ws_connected = websocket_client_->connect(host, static_cast<uint16_t>(port));
                        if (!ws_connected)
                        {
                            LOG_INFO("[ERROR] Failed to establish WebSocket connection to Coinbase");
                            return false;
                        }
                    }

                    connected_.store(true);
                    LOG_INFO("[SUCCESS] Connected to Coinbase WebSocket feed at ws-feed.exchange.coinbase.com");
                    notify_connection(true);
                    return true;
                }
                catch (const std::exception &e)
                {
                    LOG_INFO("[COINBASE] Connection failed: " + std::string(e.what()));
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

                LOG_INFO("[COINBASE] Disconnecting...");

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

                LOG_INFO("[COINBASE] Disconnected");
                notify_connection(false);
            }

            void CoinbaseFeed::set_credentials(const std::string &api_key_id, const std::string &private_key)
            {
                if (api_key_id.empty() || private_key.empty())
                {
                    LOG_INFO("[WARNING] Ignoring empty Coinbase credentials");
                    return;
                }

                {
                    std::lock_guard<std::mutex> lock(credentials_mutex_);
                    config_.api_key = api_key_id;
                    config_.secret_key = private_key;
                    credentials_.key_id = api_key_id;
                    credentials_.private_key = private_key;
                    credentials_.passphrase.clear();
                    authenticator_ = std::make_unique<auth::JWTAuthenticator>(credentials_);
                }

                configure_ssl_credentials();
                LOG_INFO("[COINBASE] Credentials stored for Coinbase feed");
            }

            void CoinbaseFeed::initialize_credentials()
            {
                std::lock_guard<std::mutex> lock(credentials_mutex_);

                if (!config_.api_key.empty() && !config_.secret_key.empty())
                {
                    credentials_.key_id = config_.api_key;
                    credentials_.private_key = config_.secret_key;
                    credentials_.passphrase.clear();
                }

                if (credentials_.is_valid())
                {
                    authenticator_ = std::make_unique<auth::JWTAuthenticator>(credentials_);
                    LOG_INFO("[COINBASE] Authentication configured using provided API key");
                }
                else
                {
                    LOG_INFO("[WARNING] Coinbase credentials not provided - authenticated streams disabled");
                }
            }

            void CoinbaseFeed::configure_ssl_credentials()
            {
                std::lock_guard<std::mutex> lock(credentials_mutex_);

                if (!ssl_websocket_client_)
                {
                    return;
                }

                if (!credentials_.is_valid())
                {
                    LOG_INFO("[WARNING] SSL WebSocket client cannot load credentials - none available");
                    return;
                }

                ssl_websocket_client_->set_credentials(credentials_.key_id, credentials_.private_key);
                LOG_INFO("[COINBASE] SSL WebSocket client configured with credentials");
            }

            bool CoinbaseFeed::has_credentials() const
            {
                std::lock_guard<std::mutex> lock(credentials_mutex_);
                return credentials_.is_valid();
            }

            bool CoinbaseFeed::is_connected() const
            {
                return connected_.load();
            }

            bool CoinbaseFeed::subscribe_trades(const std::string &symbol)
            {
                if (!is_connected())
                {
                    LOG_INFO("[COINBASE] Cannot subscribe - not connected");
                    return false;
                }

                std::string coinbase_symbol = exchange_symbol(symbol);

                // Track pending subscription
                {
                    std::lock_guard<std::mutex> lock(pending_subscriptions_mutex_);
                    pending_subscriptions_[coinbase_symbol] = std::chrono::steady_clock::now();
                }

                LOG_INFO("[COINBASE] Requesting trades subscription for " + symbol + " (Coinbase: " + coinbase_symbol + ")");

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

                // Wait for subscription result or timeout
                bool success = true;
                {
                    std::unique_lock<std::mutex> lock(pending_subscriptions_mutex_);
                    auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);

                    subscription_cv_.wait_until(lock, timeout, [this, &coinbase_symbol, &success]()
                                                {
                                                    auto it = subscription_results_.find(coinbase_symbol);
                                                    if (it != subscription_results_.end())
                                                    {
                                                        success = it->second;
                                                        return true; // Result is available
                                                    }
                                                    return false; // Still waiting
                                                });

                    // Clean up pending tracking
                    pending_subscriptions_.erase(coinbase_symbol);
                    subscription_results_.erase(coinbase_symbol);
                }

                if (success)
                {
                    // Add to active subscriptions
                    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
                    subscriptions_[symbol] = SubscriptionInfo(SubscriptionType::TRADES, coinbase_symbol);
                    subscriptions_[symbol].active = true;
                    ticker_products_.insert(coinbase_symbol);
                    // Resubscribe with full ticker product list to ensure proper server state
                    if (ssl_websocket_client_)
                    {
                        std::vector<std::string> all(ticker_products_.begin(), ticker_products_.end());
                        ssl_websocket_client_->subscribe_to_ticker(all);
                    }
                }

                return success;
            }

            bool CoinbaseFeed::subscribe_level2(const std::string &symbol)
            {
                if (!is_connected())
                {
                    LOG_INFO("[COINBASE] Cannot subscribe - not connected");
                    return false;
                }

                std::string coinbase_symbol = exchange_symbol(symbol);

                // Track pending subscription
                {
                    std::lock_guard<std::mutex> lock(pending_subscriptions_mutex_);
                    pending_subscriptions_[coinbase_symbol] = std::chrono::steady_clock::now();
                }

                LOG_INFO("[COINBASE] Requesting level2 subscription for " + symbol + " (Coinbase: " + coinbase_symbol + ")");

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

                // Wait for subscription result or timeout
                bool success = true;
                {
                    std::unique_lock<std::mutex> lock(pending_subscriptions_mutex_);
                    auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);

                    subscription_cv_.wait_until(lock, timeout, [this, &coinbase_symbol, &success]()
                                                {
                                                    auto it = subscription_results_.find(coinbase_symbol);
                                                    if (it != subscription_results_.end())
                                                    {
                                                        success = it->second;
                                                        return true; // Result is available
                                                    }
                                                    return false; // Still waiting
                                                });

                    // Clean up pending tracking
                    pending_subscriptions_.erase(coinbase_symbol);
                    subscription_results_.erase(coinbase_symbol);
                }

                if (success)
                {
                    // Add to active subscriptions
                    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
                    subscriptions_[symbol + "_level2"] = SubscriptionInfo(SubscriptionType::LEVEL2, coinbase_symbol);
                    subscriptions_[symbol + "_level2"].active = true;
                }

                return success;
            }

            bool CoinbaseFeed::unsubscribe(const std::string &symbol)
            {
                std::lock_guard<std::mutex> lock(subscriptions_mutex_);

                auto it = subscriptions_.find(symbol);
                if (it != subscriptions_.end())
                {
                    std::string coinbase_symbol = it->second.product_id;
                    subscriptions_.erase(it);

                    LOG_INFO("[COINBASE] Unsubscribed from " + symbol + " (Coinbase: " + coinbase_symbol + ")");
                    // Send actual unsubscribe message to Coinbase WebSocket
                    if (ssl_websocket_client_)
                    {
                        ticker_products_.erase(coinbase_symbol);
                        // If products remain, send a fresh subscribe to remaining list; if none, send unsubscribe
                        if (!ticker_products_.empty())
                        {
                            std::vector<std::string> remaining(ticker_products_.begin(), ticker_products_.end());
                            ssl_websocket_client_->subscribe_to_ticker(remaining);
                        }
                        else
                        {
                            ssl_websocket_client_->unsubscribe_from_ticker({coinbase_symbol});
                        }
                        ssl_websocket_client_->unsubscribe_from_level2({coinbase_symbol});
                    }
                    else if (websocket_client_)
                    {
                        // Plain client unsubscribe TODO if implemented
                    }
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
                util::log_debug("[COINBASE] SSL WebSocket message received (len=" + std::to_string(message.size()) + ")");

                // Some servers send multiple JSON objects concatenated or NDJSON; normalize by
                // inserting newlines between adjacent objects and splitting on '\n'.
                std::string normalized = message;
                // Insert newline between adjacent JSON objects if needed
                for (size_t pos = 0; (pos = normalized.find("}{", pos)) != std::string::npos;)
                {
                    normalized.replace(pos, 2, "}\n{");
                    pos += 3;
                }

                std::stringstream ss(normalized);
                std::string line;
                while (std::getline(ss, line))
                {
                    if (line.empty())
                        continue;
                    // Trim whitespace
                    line.erase(0, line.find_first_not_of(" \t\r\n"));
                    line.erase(line.find_last_not_of(" \t\r\n") + 1);
                    if (line.empty())
                        continue;

                    util::log_debug("[COINBASE] WS chunk: " + (line.size() > 200 ? line.substr(0, 200) + "..." : line));

                    try
                    {
                        nlohmann::json json_message = nlohmann::json::parse(line);

                        // Advanced Trade WS uses 'channel' for routing; fall back to 'type' for Exchange.
                        std::string channel = json_message.value("channel", "");
                        std::string type = json_message.value("type", "");
                        std::string product_id = json_message.value("product_id", "");
                        if (json_message.contains("events") && json_message["events"].is_array() && !json_message["events"].empty())
                        {
                            const auto &ev0 = json_message["events"][0];
                            if (product_id.empty() && ev0.contains("product_id") && ev0["product_id"].is_string())
                            {
                                product_id = ev0["product_id"].get<std::string>();
                            }
                        }
                        if (!channel.empty() || !type.empty())
                        {
                            LOG_INFO(std::string("[WS-IN] Channel=") + (channel.empty() ? type : channel) + (product_id.empty() ? std::string("") : std::string(" Product=") + product_id));
                            // Raw payload logging for diagnostics (throttled per channel)
                            static int raw_log_counter_trades = 0;
                            static int raw_log_counter_level2 = 0;
                            if (channel == "market_trades")
                            {
                                if (++raw_log_counter_trades % 50 == 0)
                                {
                                    LOG_INFO(std::string("[RAW] market_trades payload: ") + (line.size() > 1024 ? line.substr(0, 1024) + "..." : line));
                                }
                            }
                            else if (channel == "level2" || channel == "l2_data" || type == "l2update")
                            {
                                if (++raw_log_counter_level2 % 20 == 0)
                                {
                                    LOG_INFO(std::string("[RAW] level2 payload: ") + (line.size() > 2048 ? line.substr(0, 2048) + "..." : line));
                                }
                            }
                        }

                        if (!channel.empty())
                        {
                            // Alias Coinbase Advanced Trade Level2 variations
                            if (channel == "l2_data")
                                channel = "level2";
                            if (channel == "market_trades")
                            {
                                handle_trade_message(line);
                            }
                            else if (channel == "level2")
                            {
                                handle_level2_message(line);
                            }
                            else if (channel == "heartbeat")
                            {
                                handle_heartbeat_message(line);
                            }
                            else if (channel == "subscriptions")
                            {
                                handle_subscriptions_message(line);
                            }
                            else if (channel == "errors")
                            {
                                handle_error_message(line);
                            }
                            else
                            {
                                LOG_INFO("[COINBASE] Unknown channel: " + channel);
                            }
                        }
                        else if (!type.empty())
                        {
                            if (type == "ticker")
                            {
                                handle_ticker_message(line);
                            }
                            else if (type == "match")
                            {
                                handle_trade_message(line);
                            }
                            else if (type == "l2update")
                            {
                                handle_level2_message(line);
                            }
                            else if (type == "heartbeat")
                            {
                                handle_heartbeat_message(line);
                            }
                            else if (type == "subscriptions")
                            {
                                handle_subscriptions_message(line);
                            }
                            else if (type == "error")
                            {
                                handle_error_message(line);
                            }
                            else
                            {
                                LOG_INFO("[COINBASE] Unknown message type: " + type);
                            }
                        }
                    }
                    catch (const std::exception &e)
                    {
                        LOG_INFO("[ERROR] Failed to parse SSL WebSocket message chunk: " + std::string(e.what()));
                    }
                }
            }

            void CoinbaseFeed::handle_trade_message(const std::string &message)
            {
                try
                {
                    static int trade_update_count = 0; // throttle logging
                    nlohmann::json json = nlohmann::json::parse(message);
                    auto get_double = [](const nlohmann::json &v) -> double
                    {
                        if (v.is_number_float())
                            return v.get<double>();
                        if (v.is_number_integer())
                            return static_cast<double>(v.get<long long>());
                        if (v.is_string())
                            return std::stod(v.get<std::string>());
                        return 0.0;
                    };

                    // Advanced Trade WS format with events
                    if (json.contains("events"))
                    {
                        auto events = json["events"];
                        for (const auto &ev : events)
                        {
                            std::string product_id = ev.value("product_id", "");
                            if (ev.contains("trades"))
                            {
                                for (const auto &t : ev["trades"])
                                {
                                    double price = 0.0;
                                    double size = 0.0;
                                    if (t.contains("price"))
                                        price = get_double(t["price"]);
                                    if (t.contains("size"))
                                        size = get_double(t["size"]);

                                    exchanges::base::MarketTrade trade;
                                    trade.symbol = product_id;
                                    trade.price = price;
                                    trade.volume = size;
                                    trade.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                          std::chrono::system_clock::now().time_since_epoch())
                                                          .count();
                                    if (++trade_update_count % 50 == 0)
                                    {
                                        LOG_INFO(std::string("[INFO] Trade update ") + std::to_string(trade_update_count) +
                                                 " (" + product_id + ") price=" + std::to_string(trade.price) +
                                                 " size=" + std::to_string(trade.volume));
                                    }
                                    on_trade_received(trade);
                                }
                            }
                        }
                        return;
                    }

                    // Fallback: Exchange feed format
                    if (json.contains("product_id") && json.contains("price") && json.contains("size"))
                    {
                        std::string product_id = json["product_id"];
                        double price = get_double(json["price"]);
                        double size = get_double(json["size"]);

                        exchanges::base::MarketTrade trade;
                        trade.symbol = product_id;
                        trade.price = price;
                        trade.volume = size;
                        trade.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                              std::chrono::system_clock::now().time_since_epoch())
                                              .count();
                        if (++trade_update_count % 50 == 0)
                        {
                            LOG_INFO(std::string("[INFO] Trade update ") + std::to_string(trade_update_count) +
                                     " (" + product_id + ") price=" + std::to_string(trade.price) +
                                     " size=" + std::to_string(trade.volume));
                        }
                        on_trade_received(trade);
                    }
                }
                catch (const std::exception &e)
                {
                    LOG_INFO("[ERROR] Failed to parse trade message: " + std::string(e.what()));
                }
            }

            void CoinbaseFeed::handle_level2_message(const std::string &message)
            {
                try
                {
                    static int level2_update_count = 0; // throttle logging
                    nlohmann::json json = nlohmann::json::parse(message);
                    auto get_double = [](const nlohmann::json &v) -> double
                    {
                        if (v.is_number_float())
                            return v.get<double>();
                        if (v.is_number_integer())
                            return static_cast<double>(v.get<long long>());
                        if (v.is_string())
                            return std::stod(v.get<std::string>());
                        return 0.0;
                    };

                    // Advanced Trade WS format: events -> updates or snapshot
                    if (json.contains("events"))
                    {
                        auto events = json["events"];
                        for (const auto &ev : events)
                        {
                            std::string product_id = ev.value("product_id", "");
                            if (ev.contains("updates"))
                            {
                                for (const auto &up : ev["updates"])
                                {
                                    std::string side = up.value("side", "");
                                    double price = 0.0;
                                    double size = 0.0;
                                    if (up.contains("price"))
                                        price = get_double(up["price"]);
                                    if (up.contains("size"))
                                        size = get_double(up["size"]);

                                    exchanges::base::MarketLevel2 level2;
                                    level2.symbol = product_id;
                                    if (side == "bid" || side == "buy")
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
                                    // Throttled Level2 update log
                                    if (++level2_update_count % 100 == 0)
                                    {
                                        LOG_INFO(std::string("[INFO] Level2 update ") + std::to_string(level2_update_count) +
                                                 " (" + product_id + ") bid=" + std::to_string(level2.bid_price) +
                                                 "x" + std::to_string(level2.bid_size) + " ask=" + std::to_string(level2.ask_price) +
                                                 "x" + std::to_string(level2.ask_size));
                                    }
                                    on_level2_received(level2);
                                }
                            }
                            // If snapshots are present, convert rows to MarketLevel2 entries
                            if (ev.contains("bids"))
                            {
                                for (const auto &b : ev["bids"])
                                {
                                    exchanges::base::MarketLevel2 level2;
                                    level2.symbol = product_id;
                                    level2.bid_price = get_double(b[0]);
                                    level2.bid_size = get_double(b[1]);
                                    level2.ask_price = 0.0;
                                    level2.ask_size = 0.0;
                                    level2.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                           std::chrono::system_clock::now().time_since_epoch())
                                                           .count();
                                    if (++level2_update_count % 100 == 0)
                                    {
                                        LOG_INFO(std::string("[INFO] Level2 update ") + std::to_string(level2_update_count) +
                                                 " (" + product_id + ") bid=" + std::to_string(level2.bid_price) +
                                                 "x" + std::to_string(level2.bid_size));
                                    }
                                    on_level2_received(level2);
                                }
                            }
                            if (ev.contains("asks"))
                            {
                                for (const auto &a : ev["asks"])
                                {
                                    exchanges::base::MarketLevel2 level2;
                                    level2.symbol = product_id;
                                    level2.bid_price = 0.0;
                                    level2.bid_size = 0.0;
                                    level2.ask_price = get_double(a[0]);
                                    level2.ask_size = get_double(a[1]);
                                    level2.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                           std::chrono::system_clock::now().time_since_epoch())
                                                           .count();
                                    if (++level2_update_count % 100 == 0)
                                    {
                                        LOG_INFO(std::string("[INFO] Level2 update ") + std::to_string(level2_update_count) +
                                                 " (" + product_id + ") ask=" + std::to_string(level2.ask_price) +
                                                 "x" + std::to_string(level2.ask_size));
                                    }
                                    on_level2_received(level2);
                                }
                            }
                        }
                        return;
                    }

                    // Fallback: Exchange feed format
                    if (json.contains("product_id") && json.contains("changes"))
                    {
                        std::string product_id = json["product_id"];
                        auto changes = json["changes"];
                        for (const auto &change : changes)
                        {
                            if (change.size() >= 3)
                            {
                                std::string side = change[0];
                                double price = get_double(change[1]);
                                double size = get_double(change[2]);

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
                                if (++level2_update_count % 100 == 0)
                                {
                                    LOG_INFO(std::string("[INFO] Level2 update ") + std::to_string(level2_update_count) +
                                             " (" + product_id + ") " + (side == "buy" ? "bid=" : "ask=") +
                                             std::to_string(side == "buy" ? level2.bid_price : level2.ask_price) +
                                             "x" + std::to_string(side == "buy" ? level2.bid_size : level2.ask_size));
                                }
                                on_level2_received(level2);
                            }
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    LOG_INFO("[ERROR] Failed to parse level2 message: " + std::string(e.what()));
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
                        LOG_INFO("[COINBASE] Skipping binary/compressed WebSocket frame");
                        return;
                    }

                    // Skip if it doesn't start with JSON-like characters
                    if (message[0] != '{' && message[0] != '[')
                    {
                        LOG_INFO("[COINBASE] Skipping non-JSON WebSocket message");
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
                    LOG_INFO("[ERROR] Failed to parse ticker message: " + std::string(e.what()));
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
                            LOG_INFO("[COINBASE] Ticker " + product_id + ": $" + std::to_string(price));
                        }

                        // Forward as trade update to DTC clients
                        exchanges::base::MarketTrade trade;
                        trade.symbol = product_id;
                        trade.price = price;
                        trade.volume = json.contains("last_size") ? std::stod(json["last_size"].get<std::string>()) : 1.0;
                        trade.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                              std::chrono::system_clock::now().time_since_epoch())
                                              .count();
                        on_trade_received(trade);

                        // Also forward best bid/ask as level2 if available
                        if (json.contains("best_bid") && json.contains("best_ask"))
                        {
                            exchanges::base::MarketLevel2 level2;
                            level2.symbol = product_id;
                            level2.bid_price = std::stod(json["best_bid"].get<std::string>());
                            level2.ask_price = std::stod(json["best_ask"].get<std::string>());
                            level2.bid_size = json.contains("best_bid_size") ? std::stod(json["best_bid_size"].get<std::string>()) : 1.0;
                            level2.ask_size = json.contains("best_ask_size") ? std::stod(json["best_ask_size"].get<std::string>()) : 1.0;
                            level2.timestamp = trade.timestamp;
                            on_level2_received(level2);
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    LOG_INFO("[ERROR] Failed to parse ticker message: " + std::string(e.what()));
                }
            }

            void CoinbaseFeed::handle_heartbeat_message(const std::string &message)
            {
                LOG_INFO("[COINBASE] Heartbeat received - connection alive");

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
                    else if (json.contains("reason"))
                    {
                        error_msg = json["reason"];
                    }

                    LOG_INFO("[ERROR] Coinbase WebSocket: " + error_msg);
                    LOG_INFO("[ERROR] Full error message: " + message);

                    // Check if this error relates to a pending subscription
                    std::vector<std::string> failed_products;

                    if (json.contains("product_id"))
                    {
                        failed_products.push_back(json["product_id"]);
                    }
                    else if (json.contains("product_ids"))
                    {
                        for (const auto &product : json["product_ids"])
                        {
                            failed_products.push_back(product);
                        }
                    }
                    else if (json.contains("reason"))
                    {
                        // Extract product ID from reason string (e.g., "ETH-USDC is delisted")
                        std::string reason = json["reason"];
                        std::size_t pos = reason.find(" is delisted");
                        if (pos != std::string::npos)
                        {
                            std::string product_id = reason.substr(0, pos);
                            failed_products.push_back(product_id);
                        }
                    }

                    // Mark these subscriptions as failed
                    {
                        std::lock_guard<std::mutex> lock(pending_subscriptions_mutex_);
                        for (const auto &product : failed_products)
                        {
                            // Unconditionally set failure result so waiters can observe it
                            subscription_results_[product] = false;
                            LOG_INFO("[COINBASE] Subscription failed for product: " + product + " - " + error_msg);
                        }
                        // Notify waiting threads
                        subscription_cv_.notify_all();
                    } // Forward error to base class
                    notify_error(error_msg);
                }
                catch (const std::exception &e)
                {
                    LOG_INFO("[ERROR] Failed to parse error message: " + std::string(e.what()));
                    LOG_INFO("[ERROR] Raw error message: " + message);
                }
            }

            void CoinbaseFeed::handle_subscriptions_message(const std::string &message)
            {
                try
                {
                    nlohmann::json json = nlohmann::json::parse(message);

                    // This is a confirmation message from Coinbase about active subscriptions
                    LOG_INFO("[COINBASE] Subscription confirmation received");

                    // Log the channels we're subscribed to
                    if (json.contains("channels"))
                    {
                        for (const auto &channel : json["channels"])
                        {
                            if (channel.contains("name"))
                            {
                                std::string channel_name = channel["name"];
                                LOG_INFO("[COINBASE] Subscribed to channel: " + channel_name);

                                if (channel.contains("product_ids"))
                                {
                                    for (const auto &product : channel["product_ids"])
                                    {
                                        const std::string product_id = product.get<std::string>();
                                        LOG_INFO("[COINBASE] - Product: " + product_id);
                                        // Mark subscription success for waiting requests
                                        {
                                            std::lock_guard<std::mutex> lock(pending_subscriptions_mutex_);
                                            subscription_results_[product_id] = true;
                                            subscription_cv_.notify_all();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    LOG_INFO("[ERROR] Failed to parse subscriptions message: " + std::string(e.what()));
                }
            }

        } // namespace coinbase
    } // namespace exchanges
} // namespace open_dtc_server
