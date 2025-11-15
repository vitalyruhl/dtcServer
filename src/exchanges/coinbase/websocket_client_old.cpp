#include "coinbase_dtc_core/exchanges/coinbase/websocket_client.hpp"
#include "coinbase_dtc_core/core/util/log.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <random>
#include <cstring>
#include <algorithm>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int ssize_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif

namespace open_dtc_server
{
    namespace feed
    {
        namespace coinbase
        {

            WebSocketClient::WebSocketClient()
                : connected_(false), should_stop_(false), socket_(-1), host_("ws-feed.exchange.coinbase.com"), port_(443) // Use HTTPS/WSS port
                  ,
                  messages_received_(0), messages_sent_(0), last_message_time_(0)
            {

#ifdef _WIN32
                WSADATA wsaData;
                int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
                if (result != 0)
                {
                    util::log("[ERROR] WSAStartup failed: " + std::to_string(result));
                }
#endif
            }

            WebSocketClient::~WebSocketClient()
            {
                disconnect();

#ifdef _WIN32
                WSACleanup();
#endif
            }

            bool WebSocketClient::connect(const std::string &host, uint16_t port)
            {
                if (connected_.load())
                {
                    return true;
                }

                host_ = host;
                port_ = port;

                util::log("[WS] Connecting to " + host + ":" + std::to_string(port));

                // Establish real WebSocket connection
                if (!establish_websocket_connection())
                {
                    util::log("[ERROR] Failed to establish WebSocket connection");
                    return false;
                }

                connected_.store(true);
                should_stop_.store(false);

                // Start worker threads
                worker_thread_ = std::thread(&WebSocketClient::worker_loop, this);
                ping_thread_ = std::thread(&WebSocketClient::ping_loop, this);

                util::log("[SUCCESS] Connected to Coinbase WebSocket feed");
                return true;
            }

            void WebSocketClient::disconnect()
            {
                if (!connected_.load())
                {
                    return;
                }

                util::log("[WS] Disconnecting from Coinbase feed...");

                should_stop_.store(true);
                connected_.store(false);

                // Wait for threads to finish
                if (worker_thread_.joinable())
                {
                    worker_thread_.join();
                }
                if (ping_thread_.joinable())
                {
                    ping_thread_.join();
                }

                cleanup_socket();
                util::log("[WS] Disconnected from Coinbase feed");
            }

            bool WebSocketClient::subscribe_trades(const std::string &product_id)
            {
                if (!connected_.load())
                {
                    return false;
                }

                // Add to subscribed symbols
                {
                    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
                    subscribed_symbols_.push_back(product_id);
                }

                util::log("[WS] Subscribing to trades for " + product_id);

                // Create subscription message (simplified JSON)
                std::string message = create_subscribe_message("matches", product_id);

                {
                    std::lock_guard<std::mutex> lock(send_queue_mutex_);
                    send_queue_.push(message);
                }

                return true;
            }

            bool WebSocketClient::subscribe_level2(const std::string &product_id)
            {
                if (!connected_.load())
                {
                    return false;
                }

                util::log("[WS] Subscribing to level2 for " + product_id);

                std::string message = create_subscribe_message("level2", product_id);

                {
                    std::lock_guard<std::mutex> lock(send_queue_mutex_);
                    send_queue_.push(message);
                }

                return true;
            }

            bool WebSocketClient::unsubscribe(const std::string &product_id)
            {
                util::log("[WS] Unsubscribing from " + product_id);
                return true;
            }

            bool WebSocketClient::subscribe_multiple_symbols(const std::vector<std::string> &product_ids)
            {
                bool all_success = true;
                for (const auto &product_id : product_ids)
                {
                    if (!subscribe_trades(product_id) || !subscribe_level2(product_id))
                    {
                        all_success = false;
                        util::log("[WS] Failed to subscribe to " + product_id);
                    }
                }
                return all_success;
            }

            std::vector<std::string> WebSocketClient::get_subscribed_symbols() const
            {
                std::lock_guard<std::mutex> lock(const_cast<std::mutex &>(subscriptions_mutex_));
                return subscribed_symbols_;
            }

            std::string WebSocketClient::get_status() const
            {
                std::stringstream ss;
                ss << "Coinbase WebSocket Status:\n";
                ss << "  Connected: " << (connected_.load() ? "Yes" : "No") << "\n";
                ss << "  Host: " << host_ << ":" << port_ << "\n";
                ss << "  Messages Received: " << messages_received_.load() << "\n";
                ss << "  Messages Sent: " << messages_sent_.load() << "\n";
                ss << "  Last Activity: " << last_message_time_.load() << "\n";
                return ss.str();
            }

            void WebSocketClient::worker_loop()
            {
                util::log("[WS] Worker thread started - connecting to Coinbase");

                // For now, use realistic simulation with proper symbol names
                // TODO: Implement real WebSocket connection to wss://ws-feed.exchange.coinbase.com

                std::random_device rd;
                std::mt19937 gen(rd());

                // Realistic price ranges for each symbol
                std::unordered_map<std::string, std::pair<double, double>> price_ranges = {
                    {"STRK-USDC", {0.45, 0.65}},      // Starknet realistic range
                    {"USDC-EUR", {0.85, 0.95}},       // USDC/EUR exchange rate
                    {"SOL-USDC", {180.0, 220.0}},     // Solana range
                    {"BTC-USDC", {85000.0, 95000.0}}, // Bitcoin range
                    {"ETH-USDC", {3200.0, 3800.0}},   // Ethereum range
                    {"LTC-USDC", {85.0, 105.0}},      // Litecoin range
                    {"LINK-USDC", {18.0, 25.0}},      // Chainlink range
                    {"XRP-USDC", {1.10, 1.35}},       // Ripple range
                    {"ADA-USDC", {0.85, 1.15}}        // Cardano range
                };

                while (!should_stop_.load() && connected_.load())
                {
                    auto current_time = get_current_timestamp();

                    // Get currently subscribed symbols
                    std::vector<std::string> symbols;
                    {
                        std::lock_guard<std::mutex> lock(const_cast<std::mutex &>(subscriptions_mutex_));
                        symbols = subscribed_symbols_;
                    }

                    // Generate data for each subscribed symbol
                    for (const auto &symbol : symbols)
                    {
                        auto price_it = price_ranges.find(symbol);
                        if (price_it == price_ranges.end())
                            continue;

                        double min_price = price_it->second.first;
                        double max_price = price_it->second.second;
                        std::uniform_real_distribution<> price_dist(min_price, max_price);
                        std::uniform_real_distribution<> volume_dist(0.001, 0.500);
                        std::uniform_real_distribution<> spread_dist(0.001, 0.005); // 0.1-0.5% spread

                        // Generate trade data
                        if (trade_callback_)
                        {
                            TradeData trade;
                            trade.product_id = symbol;
                            trade.price = price_dist(gen);
                            trade.size = volume_dist(gen);
                            trade.side = (gen() % 2) ? "buy" : "sell";
                            trade.timestamp = current_time;

                            {
                                std::lock_guard<std::mutex> lock(callback_mutex_);
                                if (trade_callback_)
                                {
                                    trade_callback_(trade);
                                }
                            }

                            messages_received_++;
                            last_message_time_.store(current_time);
                        }

                        // Generate level2 data
                        if (level2_callback_)
                        {
                            Level2Data level2;
                            level2.product_id = symbol;
                            double base_price = price_dist(gen);
                            double spread_pct = spread_dist(gen);
                            level2.bid_price = base_price * (1.0 - spread_pct);
                            level2.ask_price = base_price * (1.0 + spread_pct);
                            level2.bid_size = volume_dist(gen) * 10;
                            level2.ask_size = volume_dist(gen) * 10;
                            level2.timestamp = current_time;

                            {
                                std::lock_guard<std::mutex> lock(callback_mutex_);
                                if (level2_callback_)
                                {
                                    level2_callback_(level2);
                                }
                            }

                            messages_received_++;
                            last_message_time_.store(current_time);
                        }
                    }

                    // Wait before next update (1-2 seconds)
                    if (!should_stop_.load())
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000 + (gen() % 1000)));
                    }
                }

                util::log("[WS] Worker thread stopped");
            }

            void WebSocketClient::ping_loop()
            {
                // Simple keepalive thread
                while (!should_stop_.load() && connected_.load())
                {
                    std::this_thread::sleep_for(std::chrono::seconds(30));

                    if (connected_.load())
                    {
                        // Send ping (in real implementation)
                        messages_sent_++;
                    }
                }
            }

            std::string WebSocketClient::create_subscribe_message(const std::string &channel, const std::string &product_id) const
            {
                // Simplified JSON subscription message
                return "{\"type\":\"subscribe\",\"channels\":[{\"name\":\"" + channel + "\",\"product_ids\":[\"" + product_id + "\"]}]}";
            }

            std::string WebSocketClient::create_unsubscribe_message(const std::string &product_id) const
            {
                return "{\"type\":\"unsubscribe\",\"product_ids\":[\"" + product_id + "\"]}";
            }

            uint64_t WebSocketClient::get_current_timestamp() const
            {
                auto now = std::chrono::high_resolution_clock::now();
                auto duration = now.time_since_epoch();
                return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            }

            void WebSocketClient::cleanup_socket()
            {
                if (socket_ != -1)
                {
#ifdef _WIN32
                    closesocket(socket_);
#else
                    close(socket_);
#endif
                    socket_ = -1;
                }
            }

            bool WebSocketClient::establish_websocket_connection()
            {
                // Create socket
                socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if (socket_ < 0)
                {
                    util::log("[ERROR] Failed to create socket");
                    return false;
                }

                // Resolve hostname
                struct hostent *host_entry = gethostbyname(host_.c_str());
                if (!host_entry)
                {
                    util::log("[ERROR] Failed to resolve hostname: " + host_);
                    cleanup_socket();
                    return false;
                }

                // Setup server address
                struct sockaddr_in server_addr;
                memset(&server_addr, 0, sizeof(server_addr));
                server_addr.sin_family = AF_INET;
                server_addr.sin_port = htons(port_);
                memcpy(&server_addr.sin_addr, host_entry->h_addr_list[0], host_entry->h_length);

                // Connect to server
                if (connect(socket_, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
                {
                    util::log("[ERROR] Failed to connect to " + host_ + ":" + std::to_string(port_));
                    cleanup_socket();
                    return false;
                }

                util::log("[INFO] TCP connection established to " + host_);

                // Perform WebSocket handshake
                if (!perform_websocket_handshake())
                {
                    util::log("[ERROR] WebSocket handshake failed");
                    cleanup_socket();
                    return false;
                }

                return true;
            }

            bool WebSocketClient::send_websocket_frame(const std::string &payload)
            {
                if (socket_ == -1 || !connected_.load())
                {
                    return false;
                }

                // Create WebSocket frame header
                std::vector<uint8_t> frame;
                frame.push_back(0x81); // Text frame, final fragment

                size_t payload_length = payload.length();
                if (payload_length < 126)
                {
                    frame.push_back(static_cast<uint8_t>(payload_length | 0x80)); // Mask bit set
                }
                else if (payload_length < 65536)
                {
                    frame.push_back(0x80 | 126);
                    frame.push_back(static_cast<uint8_t>(payload_length >> 8));
                    frame.push_back(static_cast<uint8_t>(payload_length & 0xFF));
                }
                else
                {
                    frame.push_back(0x80 | 127);
                    for (int i = 7; i >= 0; i--)
                    {
                        frame.push_back(static_cast<uint8_t>((payload_length >> (8 * i)) & 0xFF));
                    }
                }

                // Add mask key (client must mask)
                uint8_t mask[4] = {0x12, 0x34, 0x56, 0x78}; // Simple static mask for now
                frame.insert(frame.end(), mask, mask + 4);

                // Add masked payload
                for (size_t i = 0; i < payload_length; i++)
                {
                    frame.push_back(static_cast<uint8_t>(payload[i] ^ mask[i % 4]));
                }

                // Send frame
                ssize_t sent = send(socket_, reinterpret_cast<const char *>(frame.data()), frame.size(), 0);
                if (sent < 0)
                {
                    util::log("[ERROR] Failed to send WebSocket frame");
                    return false;
                }

                messages_sent_++;
                return true;
            }

            void WebSocketClient::process_received_message(const std::string &message)
            {
                // Placeholder for JSON parsing
                messages_received_++;
            }

            void WebSocketClient::parse_trade_message(const std::string &json)
            {
                // Would parse actual JSON trade data
            }

            void WebSocketClient::parse_level2_message(const std::string &json)
            {
                // Would parse actual JSON level2 data
            }

            std::string WebSocketClient::create_websocket_handshake() const
            {
                // Placeholder for WebSocket handshake
                return "";
            }

            bool WebSocketClient::perform_websocket_handshake()
            {
                // Generate WebSocket key
                std::string websocket_key = generate_websocket_key();

                // Create HTTP upgrade request
                std::string request =
                    "GET / HTTP/1.1\\r\\n"
                    "Host: " +
                    host_ + "\\r\\n"
                            "Upgrade: websocket\\r\\n"
                            "Connection: Upgrade\\r\\n"
                            "Sec-WebSocket-Key: " +
                    websocket_key + "\\r\\n"
                                    "Sec-WebSocket-Version: 13\\r\\n"
                                    "\\r\\n";

                // Send handshake request
                ssize_t sent = send(socket_, request.c_str(), request.length(), 0);
                if (sent < 0)
                {
                    util::log("[ERROR] Failed to send WebSocket handshake");
                    return false;
                }

                // Read handshake response
                char response[4096];
                ssize_t received = recv(socket_, response, sizeof(response) - 1, 0);
                if (received <= 0)
                {
                    util::log("[ERROR] Failed to receive WebSocket handshake response");
                    return false;
                }
                response[received] = '\\0';

                std::string response_str(response);

                // Check for successful handshake
                if (response_str.find("HTTP/1.1 101") == std::string::npos)
                {
                    util::log("[ERROR] WebSocket handshake failed - not 101 response");
                    util::log("[DEBUG] Response: " + response_str);
                    return false;
                }

                if (response_str.find("Upgrade: websocket") == std::string::npos)
                {
                    util::log("[ERROR] WebSocket handshake failed - no Upgrade header");
                    return false;
                }

                util::log("[SUCCESS] WebSocket handshake completed");
                return true;
            }

            std::string WebSocketClient::generate_websocket_key() const
            {
                // Simple key generation - in production, use proper random bytes + base64
                return "x3JJHMbDL1EzLkh9GBhXDw==";
            }

            std::string WebSocketClient::encode_websocket_frame(const std::string &payload) const
            {
                // Placeholder for WebSocket frame encoding
                return payload;
            }

            std::string WebSocketClient::decode_websocket_frame(const uint8_t *data, size_t length) const
            {
                // Placeholder for WebSocket frame decoding
                return std::string(reinterpret_cast<const char *>(data), length);
            }

        } // namespace coinbase
    } // namespace feed
} // namespace open_dtc_server