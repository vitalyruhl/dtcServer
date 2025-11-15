#include "coinbase_dtc_core/core/dtc/protocol.hpp"
#include "coinbase_dtc_core/core/server/server.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <cstring>
#include <memory>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define closesocket close
#endif

class DTCTestClient
{
public:
    DTCTestClient(const std::string &host = "127.0.0.1", uint16_t port = 11099)
        : host_(host), port_(port), socket_(-1)
    {

#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    }

    ~DTCTestClient()
    {
        disconnect();
#ifdef _WIN32
        WSACleanup();
#endif
    }

    bool connect()
    {
        socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_ == -1)
        {
            std::cerr << "[ERROR] Failed to create socket" << std::endl;
            return false;
        }

        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);

        if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0)
        {
            std::cerr << "[ERROR] Invalid address: " << host_ << std::endl;
            return false;
        }

        if (::connect(socket_, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            std::cerr << "[ERROR] Connection failed to " << host_ << ":" << port_ << std::endl;
            return false;
        }

// Set socket timeout
#ifdef _WIN32
        DWORD timeout = 5000; // 5 second timeout
        setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
#else
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#endif

        std::cout << "[OK] Connected to DTC server at " << host_ << ":" << port_ << std::endl;
        return true;
    }

    void disconnect()
    {
        if (socket_ != -1)
        {
            closesocket(socket_);
            socket_ = -1;
        }
    }

    bool send_logon_request(const std::string &username)
    {
        using namespace coinbase_dtc_core::core::dtc;

        LogonRequest logon_req;
        logon_req.username = username;
        logon_req.password = "testpass";
        logon_req.general_text_data = "DTC Test Client v1.0";

        auto data = logon_req.serialize();

        int sent = send(socket_, reinterpret_cast<const char *>(data.data()),
                        static_cast<int>(data.size()), 0);

        if (sent == static_cast<int>(data.size()))
        {
            std::cout << "[SENT] Logon request for user: " << username << std::endl;
            return true;
        }
        else
        {
            std::cerr << "[ERROR] Failed to send logon request" << std::endl;
            return false;
        }
    }

    bool send_market_data_request(const std::string &symbol)
    {
        using namespace coinbase_dtc_core::core::dtc;

        MarketDataRequest md_req;
        md_req.request_action = RequestAction::SUBSCRIBE;
        md_req.symbol = symbol;

        auto data = md_req.serialize();

        int sent = send(socket_, reinterpret_cast<const char *>(data.data()),
                        static_cast<int>(data.size()), 0);

        if (sent == static_cast<int>(data.size()))
        {
            std::cout << "[SENT] Market data request for: " << symbol << std::endl;
            return true;
        }
        else
        {
            std::cerr << "[ERROR] Failed to send market data request" << std::endl;
            return false;
        }
    }

    bool receive_and_verify_messages()
    {
        using namespace coinbase_dtc_core::core::dtc;

        Protocol protocol;
        std::vector<uint8_t> message_buffer;
        message_buffer.reserve(8192);

        int messages_received = 0;
        int trade_messages = 0;
        bool logon_success = false;

        std::cout << "[LISTEN] Listening for messages..." << std::endl;

        time_t start_time = time(nullptr);
        while (time(nullptr) - start_time < 10 && messages_received < 10)
        { // 10 seconds or 10 messages max
            std::vector<uint8_t> recv_buffer(8192);
            int bytes_received = recv(socket_, reinterpret_cast<char *>(recv_buffer.data()),
                                      static_cast<int>(recv_buffer.size()), 0);

            if (bytes_received > 0)
            {
                message_buffer.insert(message_buffer.end(), recv_buffer.begin(), recv_buffer.begin() + bytes_received);

                size_t buffer_pos = 0;
                while (buffer_pos + sizeof(MessageHeader) <= message_buffer.size())
                {
                    const MessageHeader *header = reinterpret_cast<const MessageHeader *>(message_buffer.data() + buffer_pos);
                    uint16_t expected_size = header->size;

                    if (buffer_pos + expected_size <= message_buffer.size())
                    {
                        auto message = protocol.parse_message(message_buffer.data() + buffer_pos, expected_size);

                        if (message)
                        {
                            messages_received++;
                            auto type = message->get_type();

                            switch (type)
                            {
                            case MessageType::LOGON_RESPONSE:
                            {
                                auto *response = static_cast<LogonResponse *>(message.get());
                                if (response->result)
                                {
                                    logon_success = true;
                                    std::cout << "[OK] Logon successful" << std::endl;
                                }
                                else
                                {
                                    std::cerr << "[ERROR] Logon failed: " << response->result_text << std::endl;
                                }
                                break;
                            }
                            case MessageType::MARKET_DATA_UPDATE_TRADE:
                            {
                                trade_messages++;
                                auto *trade = static_cast<MarketDataUpdateTrade *>(message.get());
                                std::cout << "[TRADE] Price: $" << trade->price << " Volume: " << trade->volume << std::endl;
                                break;
                            }
                            case MessageType::MARKET_DATA_UPDATE_BID_ASK:
                            {
                                auto *book = static_cast<MarketDataUpdateBidAsk *>(message.get());
                                std::cout << "[BOOK] Bid: $" << book->bid_price << " Ask: $" << book->ask_price << std::endl;
                                break;
                            }
                            default:
                                std::cout << "[INFO] Received message type: " << static_cast<int>(type) << std::endl;
                                break;
                            }
                        }

                        buffer_pos += expected_size;
                    }
                    else
                    {
                        break;
                    }
                }

                if (buffer_pos > 0)
                {
                    message_buffer.erase(message_buffer.begin(), message_buffer.begin() + buffer_pos);
                }
            }
            else if (bytes_received == 0)
            {
                std::cout << "[DISC] Server closed connection" << std::endl;
                break;
            }
            else
            {
                // Timeout or error
                break;
            }
        }

        std::cout << "[SUMMARY] Received " << messages_received << " messages total, "
                  << trade_messages << " trade messages" << std::endl;

        // Test passes if we got logon success and at least some market data
        return logon_success && trade_messages > 0;
    }

private:
    std::string host_;
    uint16_t port_;
    int socket_;
};

// Test function
bool run_integration_test()
{
    std::cout << "[TEST] Starting DTC Integration Test..." << std::endl;

    // Give server a moment to fully start
    std::this_thread::sleep_for(std::chrono::seconds(2));

    DTCTestClient client;

    if (!client.connect())
    {
        std::cerr << "[FAILED] Could not connect to server" << std::endl;
        return false;
    }

    if (!client.send_logon_request("testuser"))
    {
        std::cerr << "[FAILED] Could not send logon request" << std::endl;
        return false;
    }

    if (!client.send_market_data_request("BTC-USD"))
    {
        std::cerr << "[FAILED] Could not send market data request" << std::endl;
        return false;
    }

    bool result = client.receive_and_verify_messages();

    if (result)
    {
        std::cout << "[PASSED] Integration test successful!" << std::endl;
    }
    else
    {
        std::cerr << "[FAILED] Integration test failed!" << std::endl;
    }

    return result;
}

int main()
{
    try
    {
        bool success = run_integration_test();
        return success ? 0 : 1;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ERROR] Exception: " << e.what() << std::endl;
        return 1;
    }
}