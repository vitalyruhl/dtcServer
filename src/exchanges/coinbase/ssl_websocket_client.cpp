#include "coinbase_dtc_core/exchanges/coinbase/ssl_websocket_client.hpp"
#include "coinbase_dtc_core/core/util/log.hpp"
#include "../../../secrets/coinbase/coinbase.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <random>
#include <cstring>

// JSON and JWT libraries
#include <nlohmann/json.hpp>
#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/traits.h>

// Base64 encoding for WebSocket key
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

namespace open_dtc_server
{
    namespace feed
    {
        namespace coinbase
        {
            SSLWebSocketClient::SSLWebSocketClient()
                : connected_(false), should_stop_(false), host_("ws-feed.exchange.coinbase.com"),
                  port_(443), ssl_ctx_(nullptr), ssl_(nullptr), bio_(nullptr),
                  ssl_initialized_(false), socket_fd_(-1), messages_received_(0),
                  messages_sent_(0), last_message_time_(0), reconnect_attempts_(0),
                  credentials_loaded_(false)
            {
#ifdef _WIN32
                WSADATA wsaData;
                int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
                if (result != 0)
                {
                    open_dtc_server::util::log("[ERROR] WSAStartup failed: " + std::to_string(result));
                }
#endif

                if (!init_ssl())
                {
                    open_dtc_server::util::log("[ERROR] Failed to initialize SSL");
                }

                // Load JWT credentials
                api_key_id_ = load_api_key_id();
                private_key_ = load_private_key();
                credentials_loaded_ = !api_key_id_.empty() && !private_key_.empty();

                if (!credentials_loaded_)
                {
                    open_dtc_server::util::log("[WARNING] JWT credentials not loaded - will use public data only");
                }
                else
                {
                    open_dtc_server::util::log("[SUCCESS] JWT credentials loaded successfully");
                }
            }

            SSLWebSocketClient::~SSLWebSocketClient()
            {
                disconnect();
                cleanup_ssl();

#ifdef _WIN32
                WSACleanup();
#endif
            }

            bool SSLWebSocketClient::init_ssl()
            {
                if (ssl_initialized_)
                    return true;

                // Initialize OpenSSL
                SSL_library_init();
                SSL_load_error_strings();
                OpenSSL_add_all_algorithms();

                // Create SSL context
                ssl_ctx_ = SSL_CTX_new(TLS_client_method());
                if (!ssl_ctx_)
                {
                    open_dtc_server::util::log("[ERROR] Failed to create SSL context");
                    return false;
                }

                // Configure SSL context
                SSL_CTX_set_verify(ssl_ctx_, SSL_VERIFY_NONE, nullptr); // DEVELOPMENT: Skip certificate verification
                SSL_CTX_set_default_verify_paths(ssl_ctx_);

                // Enable SNI (Server Name Indication)
                SSL_CTX_set_tlsext_use_srtp(ssl_ctx_, "SRTP_AES128_CM_SHA1_80");

                ssl_initialized_ = true;
                open_dtc_server::util::log("[SUCCESS] SSL context initialized");
                return true;
            }

            void SSLWebSocketClient::cleanup_ssl()
            {
                if (ssl_)
                {
                    SSL_free(ssl_);
                    ssl_ = nullptr;
                }

                if (bio_)
                {
                    BIO_free_all(bio_);
                    bio_ = nullptr;
                }

                if (ssl_ctx_)
                {
                    SSL_CTX_free(ssl_ctx_);
                    ssl_ctx_ = nullptr;
                }

                ssl_initialized_ = false;
            }

            std::string SSLWebSocketClient::load_api_key_id()
            {
                try
                {
                    // Try to load from JSON file first
                    std::string json_path = coinbase_dtc_core::exchanges::coinbase::secrets::CDP_JSON_FILE_PATH;
                    std::ifstream file(json_path);

                    if (file.is_open())
                    {
                        nlohmann::json json_data;
                        file >> json_data;

                        if (json_data.contains("name"))
                        {
                            std::string name = json_data["name"];
                            // Extract API key ID from the full name
                            // Format: "organizations/{org_id}/apiKeys/{api_key_id}"
                            size_t last_slash = name.find_last_of('/');
                            if (last_slash != std::string::npos)
                            {
                                return name.substr(last_slash + 1);
                            }
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    open_dtc_server::util::log("[WARNING] Failed to load API key from JSON: " + std::string(e.what()));
                }

                // Fallback to hardcoded value
                return coinbase_dtc_core::exchanges::coinbase::secrets::CDP_API_KEY_ID;
            }

            std::string SSLWebSocketClient::load_private_key()
            {
                try
                {
                    // Try to load from JSON file first
                    std::string json_path = coinbase_dtc_core::exchanges::coinbase::secrets::CDP_JSON_FILE_PATH;
                    std::ifstream file(json_path);

                    if (file.is_open())
                    {
                        nlohmann::json json_data;
                        file >> json_data;

                        if (json_data.contains("privateKey"))
                        {
                            return json_data["privateKey"];
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    open_dtc_server::util::log("[WARNING] Failed to load private key from JSON: " + std::string(e.what()));
                }

                // Fallback to hardcoded value
                return coinbase_dtc_core::exchanges::coinbase::secrets::CDP_PRIVATE_KEY;
            }

            std::string SSLWebSocketClient::generate_jwt_token()
            {
                if (!credentials_loaded_)
                {
                    open_dtc_server::util::log("[ERROR] Cannot generate JWT - credentials not loaded");
                    return "";
                }

                try
                {
                    // Create JWT token for Coinbase Advanced Trade API
                    auto token = jwt::create<jwt::traits::nlohmann_json>()
                                     .set_issuer("cdp")
                                     .set_subject(api_key_id_)
                                     .set_audience("retail_rest_api_proxy")
                                     .set_issued_at(std::chrono::system_clock::now())
                                     .set_expires_at(std::chrono::system_clock::now() + std::chrono::minutes(1))
                                     .set_not_before(std::chrono::system_clock::now() - std::chrono::seconds(10))
                                     .sign(jwt::algorithm::es256("", private_key_, "", ""));

                    open_dtc_server::util::log("[SUCCESS] JWT token generated");
                    return token;
                }
                catch (const std::exception &e)
                {
                    open_dtc_server::util::log("[ERROR] Failed to generate JWT token: " + std::string(e.what()));
                    return "";
                }
            }

            bool SSLWebSocketClient::connect(const std::string &host, uint16_t port)
            {
                if (connected_.load())
                {
                    return true;
                }

                host_ = host;
                port_ = port;

                open_dtc_server::util::log("[WS] Connecting to " + host + ":" + std::to_string(port) + " with SSL");

                // Create TCP socket
                if (!connect_tcp_socket())
                {
                    open_dtc_server::util::log("[ERROR] Failed to create TCP socket");
                    return false;
                }

                // Create SSL connection
                if (!create_ssl_socket())
                {
                    open_dtc_server::util::log("[ERROR] Failed to create SSL socket");
                    return false;
                }

                // Perform SSL handshake
                if (!perform_ssl_handshake())
                {
                    open_dtc_server::util::log("[ERROR] SSL handshake failed");
                    return false;
                }

                // Perform WebSocket handshake
                if (!perform_websocket_handshake())
                {
                    open_dtc_server::util::log("[ERROR] WebSocket handshake failed");
                    return false;
                }

                connected_.store(true);
                should_stop_.store(false);

                // Start worker threads
                worker_thread_ = std::thread(&SSLWebSocketClient::worker_loop, this);
                ping_thread_ = std::thread(&SSLWebSocketClient::ping_loop, this);

                open_dtc_server::util::log("[SUCCESS] SSL WebSocket connected to " + host);

                if (connection_callback_)
                {
                    connection_callback_(true);
                }

                return true;
            }

            bool SSLWebSocketClient::connect_tcp_socket()
            {
                // Create socket
                socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
                if (socket_fd_ == INVALID_SOCKET)
#else
                if (socket_fd_ < 0)
#endif
                {
                    open_dtc_server::util::log("[ERROR] Failed to create socket");
                    return false;
                }

                // Resolve hostname
                struct addrinfo hints = {};
                hints.ai_family = AF_INET;
                hints.ai_socktype = SOCK_STREAM;

                struct addrinfo *result;
                int status = getaddrinfo(host_.c_str(), std::to_string(port_).c_str(), &hints, &result);
                if (status != 0)
                {
                    open_dtc_server::util::log("[ERROR] Failed to resolve hostname: " + host_);
#ifdef _WIN32
                    closesocket(socket_fd_);
#else
                    close(socket_fd_);
#endif
                    return false;
                }

                // Connect to server
                bool connected = false;
                for (struct addrinfo *addr = result; addr != nullptr; addr = addr->ai_next)
                {
                    if (::connect(socket_fd_, addr->ai_addr, addr->ai_addrlen) == 0)
                    {
                        connected = true;
                        break;
                    }
                }

                freeaddrinfo(result);

                if (!connected)
                {
                    open_dtc_server::util::log("[ERROR] Failed to connect to " + host_ + ":" + std::to_string(port_));
#ifdef _WIN32
                    closesocket(socket_fd_);
#else
                    close(socket_fd_);
#endif
                    return false;
                }

                open_dtc_server::util::log("[SUCCESS] TCP connection established");
                return true;
            }

            bool SSLWebSocketClient::create_ssl_socket()
            {
                // Create SSL object
                ssl_ = SSL_new(ssl_ctx_);
                if (!ssl_)
                {
                    open_dtc_server::util::log("[ERROR] Failed to create SSL object");
                    return false;
                }

                // Set socket file descriptor
                if (SSL_set_fd(ssl_, socket_fd_) != 1)
                {
                    open_dtc_server::util::log("[ERROR] Failed to set SSL file descriptor");
                    return false;
                }

                // Set hostname for SNI
                if (SSL_set_tlsext_host_name(ssl_, host_.c_str()) != 1)
                {
                    open_dtc_server::util::log("[WARNING] Failed to set SNI hostname");
                }

                return true;
            }

            bool SSLWebSocketClient::perform_ssl_handshake()
            {
                int result = SSL_connect(ssl_);
                if (result != 1)
                {
                    int error = SSL_get_error(ssl_, result);
                    char error_buf[256];
                    ERR_error_string_n(ERR_get_error(), error_buf, sizeof(error_buf));
                    open_dtc_server::util::log("[ERROR] SSL connect failed: " + std::string(error_buf));
                    return false;
                }

                // Verify certificate (DEVELOPMENT: Skip verification)
                if (!verify_certificate())
                {
                    open_dtc_server::util::log("[WARNING] Certificate verification failed - continuing anyway (DEVELOPMENT MODE)");
                    // Continue anyway for development
                }

                open_dtc_server::util::log("[SUCCESS] SSL handshake completed");
                return true;
            }

            bool SSLWebSocketClient::verify_certificate()
            {
                // DEVELOPMENT MODE: Skip certificate verification for Coinbase testing
                open_dtc_server::util::log("[DEVELOPMENT] Skipping certificate verification");
                return true;

                // PRODUCTION CODE (commented out):
                /*
                X509 *cert = SSL_get_peer_certificate(ssl_);
                if (!cert)
                {
                    open_dtc_server::util::log("[ERROR] No peer certificate");
                    return false;
                }

                int verify_result = SSL_get_verify_result(ssl_);
                if (verify_result != X509_V_OK)
                {
                    open_dtc_server::util::log("[WARNING] Certificate verification failed: " +
                                               std::string(X509_verify_cert_error_string(verify_result)));
                    X509_free(cert);
                    return false;
                }

                X509_free(cert);
                open_dtc_server::util::log("[SUCCESS] Certificate verified");
                return true;
                */
            }

            std::string SSLWebSocketClient::generate_websocket_key()
            {
                // Generate 16 random bytes for WebSocket key
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(0, 255);

                std::vector<uint8_t> key_bytes(16);
                for (int i = 0; i < 16; ++i)
                {
                    key_bytes[i] = dis(gen);
                }

                // Base64 encode
                BIO *b64 = BIO_new(BIO_f_base64());
                BIO *mem = BIO_new(BIO_s_mem());
                BIO_push(b64, mem);
                BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

                BIO_write(b64, key_bytes.data(), key_bytes.size());
                BIO_flush(b64);

                BUF_MEM *buffer_ptr;
                BIO_get_mem_ptr(b64, &buffer_ptr);
                std::string result(buffer_ptr->data, buffer_ptr->length);

                BIO_free_all(b64);
                return result;
            }

            bool SSLWebSocketClient::perform_websocket_handshake()
            {
                std::string websocket_key = generate_websocket_key();

                // Build WebSocket handshake request
                std::ostringstream request;
                request << "GET / HTTP/1.1\r\n";
                request << "Host: " << host_ << "\r\n";
                request << "Upgrade: websocket\r\n";
                request << "Connection: Upgrade\r\n";
                request << "Sec-WebSocket-Key: " << websocket_key << "\r\n";
                request << "Sec-WebSocket-Version: 13\r\n";
                request << "User-Agent: CoinbaseDTC/1.0\r\n";
                request << "\r\n";

                std::string request_str = request.str();

                // Send handshake request
                if (!send_ssl_data(request_str.c_str(), request_str.length()))
                {
                    open_dtc_server::util::log("[ERROR] Failed to send WebSocket handshake");
                    return false;
                }

                // Read handshake response
                char buffer[4096];
                int bytes_received = receive_ssl_data(buffer, sizeof(buffer) - 1);
                if (bytes_received <= 0)
                {
                    open_dtc_server::util::log("[ERROR] Failed to receive WebSocket handshake response");
                    return false;
                }

                buffer[bytes_received] = '\\0';
                std::string response(buffer);

                open_dtc_server::util::log("[DEBUG] WebSocket handshake response:\\n" + response);

                // Validate response
                if (!validate_websocket_response(response))
                {
                    open_dtc_server::util::log("[ERROR] Invalid WebSocket handshake response");
                    return false;
                }

                open_dtc_server::util::log("[SUCCESS] WebSocket handshake completed");
                return true;
            }

            bool SSLWebSocketClient::validate_websocket_response(const std::string &response)
            {
                return response.find("HTTP/1.1 101") != std::string::npos &&
                       response.find("Upgrade: websocket") != std::string::npos &&
                       response.find("Connection: upgrade") != std::string::npos;
            }

            bool SSLWebSocketClient::send_ssl_data(const char *data, size_t len)
            {
                if (!ssl_)
                    return false;

                std::lock_guard<std::mutex> lock(send_mutex_);

                size_t total_sent = 0;
                while (total_sent < len)
                {
                    int sent = SSL_write(ssl_, data + total_sent, len - total_sent);
                    if (sent <= 0)
                    {
                        int error = SSL_get_error(ssl_, sent);
                        if (error == SSL_ERROR_WANT_WRITE)
                        {
                            continue; // Retry
                        }

                        char error_buf[256];
                        ERR_error_string_n(ERR_get_error(), error_buf, sizeof(error_buf));
                        open_dtc_server::util::log("[ERROR] SSL_write failed: " + std::string(error_buf));
                        return false;
                    }
                    total_sent += sent;
                }

                return true;
            }

            int SSLWebSocketClient::receive_ssl_data(char *buffer, size_t buffer_size)
            {
                if (!ssl_)
                    return -1;

                std::lock_guard<std::mutex> lock(receive_mutex_);

                int received = SSL_read(ssl_, buffer, buffer_size);
                if (received <= 0)
                {
                    int error = SSL_get_error(ssl_, received);
                    if (error == SSL_ERROR_WANT_READ)
                    {
                        return 0; // No data available
                    }

                    char error_buf[256];
                    ERR_error_string_n(ERR_get_error(), error_buf, sizeof(error_buf));
                    open_dtc_server::util::log("[WARNING] SSL_read failed: " + std::string(error_buf));
                    return -1;
                }

                return received;
            }

            void SSLWebSocketClient::disconnect()
            {
                if (!connected_.load())
                    return;

                open_dtc_server::util::log("[WS] Disconnecting SSL WebSocket");

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

                // Clean up SSL
                if (ssl_)
                {
                    SSL_shutdown(ssl_);
                    SSL_free(ssl_);
                    ssl_ = nullptr;
                }

                // Close socket
                if (socket_fd_ >= 0)
                {
#ifdef _WIN32
                    closesocket(socket_fd_);
#else
                    close(socket_fd_);
#endif
                    socket_fd_ = -1;
                }

                if (connection_callback_)
                {
                    connection_callback_(false);
                }

                open_dtc_server::util::log("[SUCCESS] SSL WebSocket disconnected");
            }

            void SSLWebSocketClient::worker_loop()
            {
                open_dtc_server::util::log("[WORKER] SSL WebSocket worker thread started");

                while (!should_stop_.load() && connected_.load())
                {
                    process_incoming_messages();
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }

                open_dtc_server::util::log("[WORKER] SSL WebSocket worker thread ended");
            }

            void SSLWebSocketClient::ping_loop()
            {
                open_dtc_server::util::log("[PING] WebSocket ping thread started");

                while (!should_stop_.load() && connected_.load())
                {
                    std::this_thread::sleep_for(std::chrono::seconds(30));

                    if (connected_.load())
                    {
                        // Send ping frame (opcode 0x9)
                        std::vector<uint8_t> ping_frame = create_websocket_frame("ping", 0x9);
                        send_ssl_data((char *)ping_frame.data(), ping_frame.size());
                        open_dtc_server::util::log("[PING] Sent WebSocket ping");
                    }
                }

                open_dtc_server::util::log("[PING] WebSocket ping thread ended");
            }

            void SSLWebSocketClient::process_incoming_messages()
            {
                char buffer[4096];
                int bytes_received = receive_ssl_data(buffer, sizeof(buffer));

                if (bytes_received > 0)
                {
                    incoming_buffer_.insert(incoming_buffer_.end(), buffer, buffer + bytes_received);

                    // Process complete WebSocket frames
                    // This is a simplified version - full implementation would handle frame fragmentation
                    if (incoming_buffer_.size() >= 2)
                    {
                        // Basic WebSocket frame parsing
                        std::string message(incoming_buffer_.begin(), incoming_buffer_.end());
                        incoming_buffer_.clear();

                        if (message_callback_)
                        {
                            message_callback_(message);
                        }

                        messages_received_.fetch_add(1);
                        last_message_time_.store(std::chrono::steady_clock::now().time_since_epoch().count());
                    }
                }
            }

            std::vector<uint8_t> SSLWebSocketClient::create_websocket_frame(const std::string &payload, uint8_t opcode)
            {
                std::vector<uint8_t> frame;

                // WebSocket frame format (simplified)
                frame.push_back(0x80 | opcode); // FIN + opcode

                size_t payload_len = payload.length();
                if (payload_len < 126)
                {
                    frame.push_back(0x80 | payload_len); // MASK + length
                }
                else if (payload_len < 65536)
                {
                    frame.push_back(0x80 | 126);
                    frame.push_back((payload_len >> 8) & 0xFF);
                    frame.push_back(payload_len & 0xFF);
                }
                else
                {
                    frame.push_back(0x80 | 127);
                    for (int i = 7; i >= 0; --i)
                    {
                        frame.push_back((payload_len >> (i * 8)) & 0xFF);
                    }
                }

                // Add masking key (required for client)
                uint32_t mask = 0x12345678; // Simple mask for demo
                frame.push_back((mask >> 24) & 0xFF);
                frame.push_back((mask >> 16) & 0xFF);
                frame.push_back((mask >> 8) & 0xFF);
                frame.push_back(mask & 0xFF);

                // Add masked payload
                for (size_t i = 0; i < payload_len; ++i)
                {
                    frame.push_back(payload[i] ^ ((mask >> ((3 - (i % 4)) * 8)) & 0xFF));
                }

                return frame;
            }

            bool SSLWebSocketClient::send_message(const std::string &message)
            {
                if (!connected_.load())
                {
                    open_dtc_server::util::log("[ERROR] Cannot send message - not connected");
                    return false;
                }

                std::vector<uint8_t> frame = create_websocket_frame(message);
                bool success = send_ssl_data((char *)frame.data(), frame.size());

                if (success)
                {
                    messages_sent_.fetch_add(1);
                    open_dtc_server::util::log("[SEND] Message sent: " + message.substr(0, 100) + "...");
                }

                return success;
            }

            bool SSLWebSocketClient::authenticate_with_jwt()
            {
                if (!credentials_loaded_)
                {
                    open_dtc_server::util::log("[WARNING] Cannot authenticate - JWT credentials not loaded");
                    return false;
                }

                std::string jwt_token = generate_jwt_token();
                if (jwt_token.empty())
                {
                    return false;
                }

                // Create authentication message
                nlohmann::json auth_message = {
                    {"type", "subscribe"},
                    {"channels", nlohmann::json::array()},
                    {"signature", jwt_token},
                    {"key", api_key_id_},
                    {"timestamp", std::chrono::duration_cast<std::chrono::seconds>(
                                      std::chrono::system_clock::now().time_since_epoch())
                                      .count()}};

                return send_message(auth_message.dump());
            }

            bool SSLWebSocketClient::subscribe_to_ticker(const std::vector<std::string> &symbols)
            {
                nlohmann::json subscribe_message = {
                    {"type", "subscribe"},
                    {"channels", nlohmann::json::array({{{"name", "ticker"},
                                                         {"product_ids", symbols}}})}};

                return send_message(subscribe_message.dump());
            }

            bool SSLWebSocketClient::subscribe_to_level2(const std::vector<std::string> &symbols)
            {
                nlohmann::json subscribe_message = {
                    {"type", "subscribe"},
                    {"channels", nlohmann::json::array({{{"name", "level2"},
                                                         {"product_ids", symbols}}})}};

                return send_message(subscribe_message.dump());
            }

            void SSLWebSocketClient::set_message_callback(std::function<void(const std::string &)> callback)
            {
                message_callback_ = callback;
            }

            void SSLWebSocketClient::set_connection_callback(std::function<void(bool)> callback)
            {
                connection_callback_ = callback;
            }

            void SSLWebSocketClient::set_error_callback(std::function<void(const std::string &)> callback)
            {
                error_callback_ = callback;
            }

            std::chrono::steady_clock::time_point SSLWebSocketClient::get_last_message_time() const
            {
                return std::chrono::steady_clock::time_point(
                    std::chrono::steady_clock::duration(last_message_time_.load()));
            }

        } // namespace coinbase
    } // namespace feed
} // namespace open_dtc_server