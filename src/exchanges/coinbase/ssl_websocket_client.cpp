#include "coinbase_dtc_core/exchanges/coinbase/ssl_websocket_client.hpp"
#include "coinbase_dtc_core/core/util/advanced_log.hpp"
#include "../../../secrets/coinbase/coinbase.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <random>
#include <cstring>
#include <filesystem>
#include <optional>
#include <cstdlib>
#include <unordered_set>

// JSON and JWT libraries
#include <nlohmann/json.hpp>
#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/traits.h>

// Base64 encoding for WebSocket key
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <zlib.h>

namespace open_dtc_server
{
    namespace feed
    {
        namespace coinbase
        {
            // Helper function to decompress deflate data
            std::string decompress_deflate(const std::vector<uint8_t> &compressed_data)
            {
                if (compressed_data.empty())
                    return "";

                z_stream stream = {};
                stream.next_in = const_cast<Bytef *>(compressed_data.data());
                stream.avail_in = static_cast<uInt>(compressed_data.size());

                // Initialize deflate decompression
                if (inflateInit2(&stream, -MAX_WBITS) != Z_OK)
                {
                    return "";
                }

                std::string result;
                char buffer[4096];

                do
                {
                    stream.next_out = reinterpret_cast<Bytef *>(buffer);
                    stream.avail_out = sizeof(buffer);

                    int ret = inflate(&stream, Z_NO_FLUSH);
                    if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR)
                    {
                        inflateEnd(&stream);
                        return "";
                    }

                    size_t bytes_produced = sizeof(buffer) - stream.avail_out;
                    result.append(buffer, bytes_produced);

                } while (stream.avail_out == 0);

                inflateEnd(&stream);
                return result;
            }

            SSLWebSocketClient::SSLWebSocketClient()
                : connected_(false), should_stop_(false), host_("advanced-trade-ws.coinbase.com"),
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
                    LOG_ERROR("[ERROR] WSAStartup failed: " + std::to_string(result));
                }
#endif

                if (!init_ssl())
                {
                    LOG_ERROR("[ERROR] Failed to initialize SSL");
                }

                // Load JWT credentials
                api_key_id_ = load_api_key_id();
                private_key_ = load_private_key();
                credentials_loaded_ = !api_key_id_.empty() && !private_key_.empty();

                if (!credentials_loaded_)
                {
                    LOG_WARN("[WARNING] JWT credentials not loaded - will use public data only");
                    LOG_WARN("[WARNING] JWT load summary -> api_key_id length: " + std::to_string(api_key_id_.size()) + ", private key length: " + std::to_string(private_key_.size()));
                }
                else
                {
                    LOG_INFO("[SUCCESS] JWT credentials loaded successfully (key id length: " + std::to_string(api_key_id_.size()) + ")");
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

            // Duplicate authenticate_with_jwt implementation removed; canonical version is defined later
#if 0
            bool SSLWebSocketClient::authenticate_with_jwt()
            {
                // Deprecated duplicate implementation. See canonical version later in file.
                return false;
            }
#endif
            // Removed duplicate early authenticate_with_jwt implementation; see canonical implementation later in file
            // authenticate_with_jwt implemented later in file with Coinbase-required fields
            // (duplicate removed) authenticate_with_jwt is defined earlier in this file

            // Duplicate subscribe_to_ticker implementation removed; canonical version is defined later
#if 0
            bool SSLWebSocketClient::subscribe_to_ticker(const std::vector<std::string> &symbols)
            {
                // Deprecated duplicate implementation. See canonical version later in file.
                return false;
            }
#endif
            // Removed duplicate early subscribe_to_ticker; see canonical implementation later in file
            // subscribe_to_ticker implemented later in file
            // (duplicate removed) subscribe_to_ticker is defined earlier in this file

            // Duplicate subscribe_to_level2 implementation removed; canonical version is defined later
#if 0
            bool SSLWebSocketClient::subscribe_to_level2(const std::vector<std::string> &symbols)
            {
                // Deprecated duplicate implementation. See canonical version later in file.
                return false;
            }
#endif
            // Removed duplicate early subscribe_to_level2; see canonical implementation later in file
            // subscribe_to_level2 implemented later in file
            // (duplicate removed) subscribe_to_level2 is defined earlier in this file

            // Duplicate unsubscribe_from_ticker implementation removed; canonical version is defined later
#if 0
            bool SSLWebSocketClient::unsubscribe_from_ticker(const std::vector<std::string> &symbols)
            {
                // Deprecated duplicate implementation. See canonical version later in file.
                return false;
            }
#endif
            // Removed duplicate early unsubscribe_from_ticker; see canonical implementation later in file
            // unsubscribe_from_ticker implemented later in file
            // (duplicate removed) unsubscribe_from_ticker is defined earlier in this file

            // Duplicate unsubscribe_from_level2 implementation removed; canonical version is defined later
#if 0
            bool SSLWebSocketClient::unsubscribe_from_level2(const std::vector<std::string> &symbols)
            {
                // Deprecated duplicate implementation. See canonical version later in file.
                return false;
            }
#endif
            // Removed duplicate early unsubscribe_from_level2; see canonical implementation later in file
            // unsubscribe_from_level2 implemented later in file
            // (duplicate removed) unsubscribe_from_level2 is defined earlier in this file

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
                    LOG_ERROR("[ERROR] Failed to create SSL context");
                    return false;
                }

                // Configure SSL context
                SSL_CTX_set_verify(ssl_ctx_, SSL_VERIFY_NONE, nullptr); // DEVELOPMENT: Skip certificate verification
                SSL_CTX_set_default_verify_paths(ssl_ctx_);

                // Enable SNI (Server Name Indication)
                SSL_CTX_set_tlsext_use_srtp(ssl_ctx_, "SRTP_AES128_CM_SHA1_80");

                ssl_initialized_ = true;
                LOG_INFO("[SUCCESS] SSL context initialized");
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

            void SSLWebSocketClient::set_credentials(const std::string &api_key_id, const std::string &private_key)
            {
                LOG_INFO("[JWT] Applying credentials from server (key id length: " + std::to_string(api_key_id.size()) + ", private key length: " + std::to_string(private_key.size()) + ")");
                api_key_id_ = api_key_id;
                private_key_ = private_key;
                credentials_loaded_ = !api_key_id_.empty() && !private_key_.empty();

                if (credentials_loaded_)
                {
                    LOG_INFO("[SUCCESS] JWT credentials set via parameter");
                }
                else
                {
                    LOG_WARN("[WARNING] Invalid credentials provided via parameter - missing key id or private key");
                }
            }

            std::string SSLWebSocketClient::resolve_credentials_path() const
            {
                namespace fs = std::filesystem;

                const fs::path default_relative(coinbase_dtc_core::exchanges::coinbase::secrets::CDP_JSON_FILE_PATH);
                const fs::path config_relative("config/cdp_api_key_ECDSA.json");

                std::vector<std::string> candidates;
                std::unordered_set<std::string> seen;

                auto add_candidate = [&](const fs::path &candidate)
                {
                    if (candidate.empty())
                    {
                        return;
                    }

                    std::string normalized = candidate.lexically_normal().string();
                    if (normalized.empty())
                    {
                        return;
                    }

                    if (seen.emplace(normalized).second)
                    {
                        candidates.emplace_back(normalized);
                    }
                };

                if (const char *env_path = std::getenv("CDP_CREDENTIALS_PATH"); env_path && *env_path)
                {
                    add_candidate(fs::path(env_path));
                }

                add_candidate(config_relative);
                add_candidate(default_relative);

                fs::path current = fs::current_path();
                for (int i = 0; i < 4 && !current.empty(); ++i)
                {
                    add_candidate(current / config_relative);
                    add_candidate(current / default_relative);
                    current = current.parent_path();
                }

#ifdef _WIN32
                char module_path[MAX_PATH];
                if (GetModuleFileNameA(nullptr, module_path, MAX_PATH))
                {
                    fs::path exe_dir = fs::path(module_path).parent_path();
                    add_candidate(exe_dir / config_relative);
                }
#endif

                for (const auto &candidate : candidates)
                {
                    std::error_code ec;
                    const fs::path candidate_path(candidate);
                    if (fs::exists(candidate_path, ec))
                    {
                        return candidate_path.string();
                    }
                }

                return default_relative.string();
            }

            std::string SSLWebSocketClient::load_api_key_id()
            {
                const std::string json_path = resolve_credentials_path();
                LOG_INFO("[AUTH] Loading API key id from credentials file: " + json_path);

                try
                {
                    // Try to load from JSON file first
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
                                std::string api_key_id = name.substr(last_slash + 1);
                                if (!api_key_id.empty())
                                {
                                    LOG_INFO("[AUTH] API key id loaded from JSON (length " + std::to_string(api_key_id.size()) + ")");
                                    return api_key_id;
                                }
                                LOG_WARN("[WARNING] Extracted API key id from JSON is empty");
                            }
                            else
                            {
                                LOG_WARN("[WARNING] Unable to parse API key id from JSON name field: " + name);
                            }
                        }
                        else
                        {
                            LOG_WARN("[WARNING] Credentials JSON missing 'name' field");
                        }
                    }
                    else
                    {
                        LOG_WARN("[WARNING] Credentials JSON file could not be opened: " + json_path + ". Run tools/start_server.cmd or set CDP_CREDENTIALS_PATH.");
                    }
                }
                catch (const std::exception &e)
                {
                    LOG_WARN("[WARNING] Failed to load API key from JSON: " + std::string(e.what()));
                }

                // Fallback to hardcoded value
                LOG_INFO("[AUTH] Using fallback API key id from secrets header");
                return coinbase_dtc_core::exchanges::coinbase::secrets::CDP_API_KEY_ID;
            }

            std::string SSLWebSocketClient::load_private_key()
            {
                const std::string json_path = resolve_credentials_path();
                LOG_INFO("[AUTH] Loading private key from credentials file: " + json_path);

                try
                {
                    // Try to load from JSON file first
                    std::ifstream file(json_path);

                    if (file.is_open())
                    {
                        nlohmann::json json_data;
                        file >> json_data;

                        if (json_data.contains("privateKey"))
                        {
                            std::string key = json_data["privateKey"];
                            if (!key.empty())
                            {
                                LOG_INFO("[AUTH] Private key loaded from JSON (length " + std::to_string(key.size()) + ")");
                                return key;
                            }
                            LOG_WARN("[WARNING] privateKey field in JSON is empty");
                        }
                        else
                        {
                            LOG_WARN("[WARNING] Credentials JSON missing 'privateKey' field");
                        }
                    }
                    else
                    {
                        LOG_WARN("[WARNING] Credentials JSON file could not be opened for private key: " + json_path + ". Run tools/start_server.cmd or set CDP_CREDENTIALS_PATH.");
                    }
                }
                catch (const std::exception &e)
                {
                    LOG_WARN("[WARNING] Failed to load private key from JSON: " + std::string(e.what()));
                }

                // Fallback to hardcoded value
                LOG_INFO("[AUTH] Using fallback private key from secrets header");
                return coinbase_dtc_core::exchanges::coinbase::secrets::CDP_PRIVATE_KEY;
            }

            std::string SSLWebSocketClient::generate_jwt_token()
            {
                if (!credentials_loaded_)
                {
                    LOG_ERROR("[ERROR] Cannot generate JWT - credentials not loaded");
                    return "";
                }

                try
                {
                    // Create JWT token for Coinbase Advanced Trade WebSocket auth
                    auto now = std::chrono::system_clock::now();
                    // Generate a simple nonce for JWT header
                    auto nonce_val = std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count());
                    auto token = jwt::create<jwt::traits::nlohmann_json>()
                                     .set_issuer("cdp")
                                     .set_subject(api_key_id_)
                                     // Advanced Trade WS does not require audience; use exact host if present
                                     //.set_audience("advanced-trade-ws.coinbase.com")
                                     .set_issued_at(now)
                                     .set_expires_at(now + std::chrono::seconds(30))
                                     .set_not_before(now - std::chrono::seconds(5))
                                     .set_type("JWT")
                                     .set_key_id(api_key_id_)
                                     .set_header_claim("nonce", nonce_val)
                                     .sign(jwt::algorithm::es256("", private_key_, "", ""));

                    LOG_INFO("[SUCCESS] JWT token generated");
                    return token;
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR("[ERROR] Failed to generate JWT token: " + std::string(e.what()));
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

                LOG_INFO("[WS] Connecting to " + host + ":" + std::to_string(port) + " with SSL");

                // Create TCP socket
                if (!connect_tcp_socket())
                {
                    LOG_ERROR("[ERROR] Failed to create TCP socket");
                    return false;
                }

                // Create SSL connection
                if (!create_ssl_socket())
                {
                    LOG_ERROR("[ERROR] Failed to create SSL socket");
                    return false;
                }

                // Perform SSL handshake
                if (!perform_ssl_handshake())
                {
                    LOG_ERROR("[ERROR] SSL handshake failed");
                    return false;
                }

                // Perform WebSocket handshake
                if (!perform_websocket_handshake())
                {
                    LOG_ERROR("[ERROR] WebSocket handshake failed");
                    return false;
                }

                connected_.store(true);
                should_stop_.store(false);

                // Start worker threads
                worker_thread_ = std::thread(&SSLWebSocketClient::worker_loop, this);
                ping_thread_ = std::thread(&SSLWebSocketClient::ping_loop, this);

                LOG_INFO("[SUCCESS] SSL WebSocket connected to " + host);

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
                    LOG_ERROR("[ERROR] Failed to create socket");
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
                    LOG_ERROR("[ERROR] Failed to resolve hostname: " + host_);
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
                    LOG_ERROR("[ERROR] Failed to connect to " + host_ + ":" + std::to_string(port_));
#ifdef _WIN32
                    closesocket(socket_fd_);
#else
                    close(socket_fd_);
#endif
                    return false;
                }

                LOG_INFO("[SUCCESS] TCP connection established");
                return true;
            }

            bool SSLWebSocketClient::create_ssl_socket()
            {
                // Create SSL object
                ssl_ = SSL_new(ssl_ctx_);
                if (!ssl_)
                {
                    LOG_ERROR("[ERROR] Failed to create SSL object");
                    return false;
                }

                // Set socket file descriptor
                if (SSL_set_fd(ssl_, socket_fd_) != 1)
                {
                    LOG_ERROR("[ERROR] Failed to set SSL file descriptor");
                    return false;
                }

                // Set hostname for SNI
                if (SSL_set_tlsext_host_name(ssl_, host_.c_str()) != 1)
                {
                    LOG_WARN("[WARNING] Failed to set SNI hostname");
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
                    LOG_ERROR("[ERROR] SSL connect failed: " + std::string(error_buf));
                    return false;
                }

                // Verify certificate (DEVELOPMENT: Skip verification)
                if (!verify_certificate())
                {
                    LOG_WARN("[WARNING] Certificate verification failed - continuing anyway (DEVELOPMENT MODE)");
                    // Continue anyway for development
                }

                LOG_INFO("[SUCCESS] SSL handshake completed");
                return true;
            }

            bool SSLWebSocketClient::verify_certificate()
            {
                // DEVELOPMENT MODE: Skip certificate verification for Coinbase testing
                LOG_DEBUG("[DEVELOPMENT] Skipping certificate verification");
                return true;

                // PRODUCTION CODE (commented out):
                /*
                X509 *cert = SSL_get_peer_certificate(ssl_);
                if (!cert)
                {
                    LOG_ERROR("[ERROR] No peer certificate");
                    return false;
                }

                int verify_result = SSL_get_verify_result(ssl_);
                if (verify_result != X509_V_OK)
                {
                    LOG_WARN("[WARNING] Certificate verification failed: " +
                                               std::string(X509_verify_cert_error_string(verify_result)));
                    X509_free(cert);
                    return false;
                }

                X509_free(cert);
                LOG_INFO("[SUCCESS] Certificate verified");
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
                request << "Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits\r\n";
                request << "User-Agent: CoinbaseDTC/1.0\r\n";
                request << "\r\n";

                std::string request_str = request.str();

                // Send handshake request
                if (!send_ssl_data(request_str.c_str(), request_str.length()))
                {
                    LOG_ERROR("[ERROR] Failed to send WebSocket handshake");
                    return false;
                }

                // Read handshake response
                char buffer[4096];
                int bytes_received = receive_ssl_data(buffer, sizeof(buffer) - 1);
                if (bytes_received <= 0)
                {
                    LOG_ERROR("[ERROR] Failed to receive WebSocket handshake response");
                    return false;
                }

                buffer[bytes_received] = '\\0';
                std::string response(buffer);

                LOG_DEBUG("[DEBUG] WebSocket handshake response:\\n" + response);

                // Validate response
                if (!validate_websocket_response(response))
                {
                    LOG_ERROR("[ERROR] Invalid WebSocket handshake response");
                    return false;
                }

                LOG_INFO("[SUCCESS] WebSocket handshake completed");
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
                        LOG_ERROR("[ERROR] SSL_write failed: " + std::string(error_buf));
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
                    LOG_WARN("[WARNING] SSL_read failed: " + std::string(error_buf));
                    return -1;
                }

                return received;
            }

            void SSLWebSocketClient::disconnect()
            {
                if (!connected_.load())
                    return;

                LOG_INFO("[WS] Disconnecting SSL WebSocket");

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

                LOG_INFO("[SUCCESS] SSL WebSocket disconnected");
            }

            void SSLWebSocketClient::worker_loop()
            {
                LOG_INFO("[WORKER] SSL WebSocket worker thread started");

                while (!should_stop_.load() && connected_.load())
                {
                    process_incoming_messages();
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }

                LOG_INFO("[WORKER] SSL WebSocket worker thread ended");
            }

            void SSLWebSocketClient::ping_loop()
            {
                LOG_DEBUG("[PING] WebSocket ping thread started");

                while (!should_stop_.load() && connected_.load())
                {
                    std::this_thread::sleep_for(std::chrono::seconds(30));

                    if (connected_.load())
                    {
                        // Send ping frame (opcode 0x9)
                        std::vector<uint8_t> ping_frame = create_websocket_frame("ping", 0x9);
                        send_ssl_data((char *)ping_frame.data(), ping_frame.size());
                        LOG_DEBUG("[PING] Sent WebSocket ping");
                    }
                }

                LOG_DEBUG("[PING] WebSocket ping thread ended");
            }

            void SSLWebSocketClient::process_incoming_messages()
            {
                char buffer[4096];
                int bytes_received = receive_ssl_data(buffer, sizeof(buffer));

                if (bytes_received > 0)
                {
                    incoming_buffer_.insert(incoming_buffer_.end(), buffer, buffer + bytes_received);

                    // Process complete WebSocket frames
                    while (incoming_buffer_.size() >= 2)
                    {
                        // Check if we have enough data for a complete frame
                        size_t frame_size = calculate_frame_size(incoming_buffer_);
                        if (frame_size == 0 || incoming_buffer_.size() < frame_size)
                        {
                            break; // Wait for more data
                        }

                        // Extract complete frame
                        std::vector<uint8_t> frame_data(incoming_buffer_.begin(), incoming_buffer_.begin() + frame_size);
                        incoming_buffer_.erase(incoming_buffer_.begin(), incoming_buffer_.begin() + frame_size);

                        // Parse WebSocket frame
                        std::string message = parse_websocket_frame(frame_data);

                        // Only process text frames as JSON - ignore binary/control frames
                        if (!message.empty() && message_callback_ && is_valid_json_start(message))
                        {
                            message_callback_(message);
                        }
                        else if (!message.empty() && !is_valid_json_start(message))
                        {
                            LOG_DEBUG("[DEBUG] Ignoring non-JSON WebSocket frame (binary/control data)");
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
                    LOG_ERROR("[ERROR] Cannot send message - not connected");
                    return false;
                }

                std::vector<uint8_t> frame = create_websocket_frame(message);
                bool success = send_ssl_data((char *)frame.data(), frame.size());

                if (success)
                {
                    messages_sent_.fetch_add(1);
                    LOG_DEBUG("[SEND] Message sent: " + message.substr(0, 100) + "...");
                }

                return success;
            }

            bool SSLWebSocketClient::authenticate_with_jwt()
            {
                // Advanced Trade WS uses JWT embedded in subscribe messages (e.g., level2).
                // Do not send a standalone auth message; simply indicate credentials are loaded.
                if (!credentials_loaded_)
                {
                    LOG_WARN("[WARNING] Cannot authenticate - JWT credentials not loaded");
                    return false;
                }

                LOG_INFO("[AUTH] Skipping standalone auth; JWT will be included in subscribe requests");
                return true;
            }

            bool SSLWebSocketClient::subscribe_to_ticker(const std::vector<std::string> &symbols)
            {
                // Detect endpoint flavor and build appropriate payload
                nlohmann::json subscribe_message;
                const bool is_advanced = host_.find("advanced-trade-ws.coinbase.com") != std::string::npos;
                if (is_advanced)
                {
                    // Advanced Trade WS
                    subscribe_message = {
                        {"type", "subscribe"},
                        {"channel", "market_trades"},
                        {"product_ids", symbols}};
                    if (credentials_loaded_)
                    {
                        std::string jwt_token = generate_jwt_token();
                        subscribe_message["jwt"] = jwt_token;
                    }
                }
                else
                {
                    // Legacy Exchange WS expects channels array
                    subscribe_message = {
                        {"type", "subscribe"},
                        {"channels", nlohmann::json::array({nlohmann::json{{"name", "matches"}, {"product_ids", symbols}}})}};
                }

                LOG_DEBUG(std::string("[SEND] Subscribe Ticker Payload: ") + subscribe_message.dump());
                return send_message(subscribe_message.dump());
            }

            bool SSLWebSocketClient::unsubscribe_from_ticker(const std::vector<std::string> &symbols)
            {
                nlohmann::json unsubscribe_message;
                const bool is_advanced = host_.find("advanced-trade-ws.coinbase.com") != std::string::npos;
                if (is_advanced)
                {
                    unsubscribe_message = {
                        {"type", "unsubscribe"},
                        {"channel", "market_trades"},
                        {"product_ids", symbols}};
                }
                else
                {
                    unsubscribe_message = {
                        {"type", "unsubscribe"},
                        {"channels", nlohmann::json::array({nlohmann::json{{"name", "matches"}, {"product_ids", symbols}}})}};
                }
                LOG_DEBUG(std::string("[SEND] Unsubscribe Ticker Payload: ") + unsubscribe_message.dump());
                return send_message(unsubscribe_message.dump());
            }

            bool SSLWebSocketClient::subscribe_to_level2(const std::vector<std::string> &symbols)
            {
                nlohmann::json subscribe_message;
                const bool is_advanced = host_.find("advanced-trade-ws.coinbase.com") != std::string::npos;
                if (is_advanced)
                {
                    // Advanced Trade WS subscribe format
                    subscribe_message = {
                        {"type", "subscribe"},
                        {"channel", "level2"},
                        {"product_ids", symbols}};

                    if (credentials_loaded_)
                    {
                        std::string jwt_token = generate_jwt_token();
                        subscribe_message["jwt"] = jwt_token;
                    }
                }
                else
                {
                    // Legacy Exchange WS expects channels array
                    subscribe_message = {
                        {"type", "subscribe"},
                        {"channels", nlohmann::json::array({nlohmann::json{{"name", "level2"}, {"product_ids", symbols}}})}};
                }

                LOG_DEBUG(std::string("[SEND] Subscribe Level2 Payload: ") + subscribe_message.dump());
                return send_message(subscribe_message.dump());
            }

            bool SSLWebSocketClient::unsubscribe_from_level2(const std::vector<std::string> &symbols)
            {
                nlohmann::json unsubscribe_message;
                const bool is_advanced = host_.find("advanced-trade-ws.coinbase.com") != std::string::npos;
                if (is_advanced)
                {
                    unsubscribe_message = {
                        {"type", "unsubscribe"},
                        {"channel", "level2"},
                        {"product_ids", symbols}};
                }
                else
                {
                    unsubscribe_message = {
                        {"type", "unsubscribe"},
                        {"channels", nlohmann::json::array({nlohmann::json{{"name", "level2"}, {"product_ids", symbols}}})}};
                }
                LOG_DEBUG(std::string("[SEND] Unsubscribe Level2 Payload: ") + unsubscribe_message.dump());
                return send_message(unsubscribe_message.dump());
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

            std::string SSLWebSocketClient::parse_websocket_frame(const std::vector<uint8_t> &frame_data)
            {
                if (frame_data.size() < 2)
                    return "";

                // Parse WebSocket frame header
                uint8_t first_byte = frame_data[0];
                uint8_t second_byte = frame_data[1];

                bool fin = (first_byte & 0x80) != 0;
                bool rsv1 = (first_byte & 0x40) != 0; // Per-Message-Deflate extension
                bool rsv2 = (first_byte & 0x20) != 0;
                bool rsv3 = (first_byte & 0x10) != 0;
                uint8_t opcode = first_byte & 0x0F;

                bool mask = (second_byte & 0x80) != 0;
                uint64_t payload_length = second_byte & 0x7F;

                size_t header_length = 2;

                // Extended payload length
                if (payload_length == 126)
                {
                    if (frame_data.size() < 4)
                        return "";
                    payload_length = (static_cast<uint64_t>(frame_data[2]) << 8) |
                                     frame_data[3];
                    header_length = 4;
                }
                else if (payload_length == 127)
                {
                    if (frame_data.size() < 10)
                        return "";
                    payload_length = 0;
                    for (int i = 0; i < 8; i++)
                    {
                        payload_length = (payload_length << 8) | frame_data[2 + i];
                    }
                    header_length = 10;
                }

                // Masking key (if present)
                std::vector<uint8_t> masking_key(4);
                if (mask)
                {
                    if (frame_data.size() < header_length + 4)
                        return "";
                    std::copy(frame_data.begin() + header_length,
                              frame_data.begin() + header_length + 4,
                              masking_key.begin());
                    header_length += 4;
                }

                // Extract payload
                if (frame_data.size() < header_length + payload_length)
                    return "";

                std::vector<uint8_t> payload(frame_data.begin() + header_length,
                                             frame_data.begin() + header_length + payload_length);

                // Unmask payload if needed
                if (mask)
                {
                    for (size_t i = 0; i < payload.size(); i++)
                    {
                        payload[i] ^= masking_key[i % 4];
                    }
                }

                // Handle different opcodes
                if (opcode == 0x1) // Text frame
                {
                    // Check if data is compressed (RSV1 bit set for Per-Message-Deflate)
                    if (rsv1)
                    {
                        // Decompress the payload
                        return decompress_deflate(payload);
                    }
                    else
                    {
                        return std::string(payload.begin(), payload.end());
                    }
                }
                else if (opcode == 0x2) // Binary frame
                {
                    // Don't convert binary frames to string - they're not JSON
                    LOG_DEBUG("[DEBUG] Received binary WebSocket frame, ignoring (not JSON)");
                    return ""; // Return empty string to avoid JSON parsing
                }
                else if (opcode == 0x8) // Close frame
                {
                    LOG_INFO("[INFO] Received WebSocket close frame");
                    return "";
                }
                else if (opcode == 0x9) // Ping frame
                {
                    LOG_INFO("[INFO] Received WebSocket ping frame");
                    // Should send pong response
                    return "";
                }
                else if (opcode == 0xA) // Pong frame
                {
                    LOG_INFO("[INFO] Received WebSocket pong frame");
                    return "";
                }

                return "";
            }

            size_t SSLWebSocketClient::calculate_frame_size(const std::vector<uint8_t> &frame_data)
            {
                if (frame_data.size() < 2)
                    return 0;

                uint8_t second_byte = frame_data[1];
                bool mask = (second_byte & 0x80) != 0;
                uint64_t payload_length = second_byte & 0x7F;

                size_t header_length = 2;

                // Extended payload length
                if (payload_length == 126)
                {
                    if (frame_data.size() < 4)
                        return 0;
                    payload_length = (static_cast<uint64_t>(frame_data[2]) << 8) |
                                     frame_data[3];
                    header_length = 4;
                }
                else if (payload_length == 127)
                {
                    if (frame_data.size() < 10)
                        return 0;
                    payload_length = 0;
                    for (int i = 0; i < 8; i++)
                    {
                        payload_length = (payload_length << 8) | frame_data[2 + i];
                    }
                    header_length = 10;
                }

                // Add masking key size if present
                if (mask)
                {
                    header_length += 4;
                }

                return header_length + static_cast<size_t>(payload_length);
            }

            bool SSLWebSocketClient::is_valid_json_start(const std::string &message)
            {
                if (message.empty())
                    return false;

                // Trim whitespace
                size_t start = 0;
                while (start < message.size() && std::isspace(message[start]))
                    start++;

                if (start >= message.size())
                    return false;

                // JSON should start with { or [
                char first_char = message[start];
                return first_char == '{' || first_char == '[';
            }

            std::chrono::steady_clock::time_point SSLWebSocketClient::get_last_message_time() const
            {
                return std::chrono::steady_clock::time_point(
                    std::chrono::steady_clock::duration(last_message_time_.load()));
            }

        } // namespace coinbase
    } // namespace feed
} // namespace open_dtc_server
