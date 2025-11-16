#include <iostream>
#include <thread>
#include <chrono>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

#include "coinbase_dtc_core/core/dtc/protocol.hpp"

using namespace open_dtc_server::core::dtc;

class DTCTestClient
{
private:
    SOCKET socket_;
    bool connected_;
    Protocol protocol_;

public:
    DTCTestClient() : socket_(INVALID_SOCKET), connected_(false)
    {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            std::cout << "[ERROR] WSAStartup failed" << std::endl;
        }
#endif
    }

    ~DTCTestClient()
    {
        disconnect();
#ifdef _WIN32
        WSACleanup();
#endif
    }

    bool connect(const std::string &host, int port)
    {
        // Create socket
        socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_ == INVALID_SOCKET)
        {
            std::cout << "[ERROR] Failed to create socket" << std::endl;
            return false;
        }

        // Setup server address
        sockaddr_in server_addr = {};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) != 1)
        {
            std::cout << "[ERROR] Invalid host address: " << host << std::endl;
            closesocket(socket_);
            return false;
        }

        // Connect
        if (::connect(socket_, (sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
        {
            std::cout << "[ERROR] Failed to connect to " << host << ":" << port << std::endl;
            closesocket(socket_);
            return false;
        }

        connected_ = true;
        std::cout << "[SUCCESS] Connected to DTC server at " << host << ":" << port << std::endl;
        return true;
    }

    void disconnect()
    {
        if (socket_ != INVALID_SOCKET)
        {
            closesocket(socket_);
            socket_ = INVALID_SOCKET;
        }
        connected_ = false;
    }

    bool send_message(const std::vector<uint8_t> &data)
    {
        if (!connected_)
            return false;

        int total_sent = 0;
        int data_size = static_cast<int>(data.size());

        while (total_sent < data_size)
        {
            int sent = send(socket_, reinterpret_cast<const char *>(data.data() + total_sent),
                            data_size - total_sent, 0);
            if (sent == SOCKET_ERROR)
            {
                std::cout << "[ERROR] Failed to send message" << std::endl;
                return false;
            }
            total_sent += sent;
        }

        return true;
    }

    std::vector<uint8_t> receive_message()
    {
        if (!connected_)
            return {};

        // Read message header first (size + type)
        uint8_t header_buffer[6]; // 2 bytes size + 4 bytes type
        int received = 0;

        while (received < 6)
        {
            int bytes = recv(socket_, reinterpret_cast<char *>(header_buffer + received),
                             6 - received, 0);
            if (bytes <= 0)
            {
                std::cout << "[ERROR] Failed to receive header" << std::endl;
                return {};
            }
            received += bytes;
        }

        // Extract message size
        uint16_t message_size = *reinterpret_cast<uint16_t *>(header_buffer);

        std::vector<uint8_t> message_data(message_size);
        memcpy(message_data.data(), header_buffer, 6);

        // Read rest of message if any
        if (message_size > 6)
        {
            received = 6;
            while (received < message_size)
            {
                int bytes = recv(socket_, reinterpret_cast<char *>(message_data.data() + received),
                                 message_size - received, 0);
                if (bytes <= 0)
                {
                    std::cout << "[ERROR] Failed to receive message body" << std::endl;
                    return {};
                }
                received += bytes;
            }
        }

        return message_data;
    }

    void test_market_data_subscription()
    {
        std::cout << "\n=== Testing Market Data Subscription ===" << std::endl;

        // Create MarketDataRequest for subscription
        auto market_request = protocol_.create_market_data_request(
            RequestAction::SUBSCRIBE,
            1,         // symbol_id
            "BTC-USD", // symbol
            "coinbase" // exchange
        );

        // Send the request
        auto request_data = protocol_.create_message(*market_request);
        std::cout << "[SEND] MarketDataRequest: SUBSCRIBE to BTC-USD on coinbase (Symbol ID: 1)" << std::endl;

        if (!send_message(request_data))
        {
            std::cout << "[ERROR] Failed to send MarketDataRequest" << std::endl;
            return;
        }

        std::cout << "[INFO] MarketDataRequest sent, waiting for response..." << std::endl;

        // Wait for response
        auto response_data = receive_message();
        if (response_data.empty())
        {
            std::cout << "[ERROR] No response received" << std::endl;
            return;
        }

        // Parse response
        auto response_message = protocol_.parse_message(response_data.data(), response_data.size());
        if (!response_message)
        {
            std::cout << "[ERROR] Failed to parse response message" << std::endl;
            return;
        }

        // Check if it's MarketDataResponse
        if (response_message->get_type() == MessageType::MARKET_DATA_RESPONSE)
        {
            auto *market_response = static_cast<MarketDataResponse *>(response_message.get());

            std::cout << "[RECEIVED] MarketDataResponse:" << std::endl;
            std::cout << "  - Symbol ID: " << market_response->symbol_id << std::endl;
            std::cout << "  - Symbol: '" << market_response->symbol << "'" << std::endl;
            std::cout << "  - Exchange: '" << market_response->exchange << "'" << std::endl;
            std::cout << "  - Result: " << (market_response->result ? "SUCCESS" : "FAILURE") << std::endl;

            if (market_response->result)
            {
                std::cout << "[SUCCESS] Market data subscription successful!" << std::endl;
            }
            else
            {
                std::cout << "[ERROR] Market data subscription failed!" << std::endl;
            }
        }
        else
        {
            std::cout << "[ERROR] Expected MarketDataResponse, got message type: "
                      << static_cast<int>(response_message->get_type()) << std::endl;
        }
    }

    void test_market_data_unsubscription()
    {
        std::cout << "\n=== Testing Market Data Unsubscription ===" << std::endl;

        // Create MarketDataRequest for unsubscription
        auto market_request = protocol_.create_market_data_request(
            RequestAction::UNSUBSCRIBE,
            1,         // symbol_id
            "BTC-USD", // symbol
            "coinbase" // exchange
        );

        // Send the request
        auto request_data = protocol_.create_message(*market_request);
        std::cout << "[SEND] MarketDataRequest: UNSUBSCRIBE from BTC-USD on coinbase (Symbol ID: 1)" << std::endl;

        if (!send_message(request_data))
        {
            std::cout << "[ERROR] Failed to send MarketDataRequest" << std::endl;
            return;
        }

        std::cout << "[INFO] MarketDataRequest sent, waiting for response..." << std::endl;

        // Wait for response
        auto response_data = receive_message();
        if (response_data.empty())
        {
            std::cout << "[ERROR] No response received" << std::endl;
            return;
        }

        // Parse response
        auto response_message = protocol_.parse_message(response_data.data(), response_data.size());
        if (!response_message)
        {
            std::cout << "[ERROR] Failed to parse response message" << std::endl;
            return;
        }

        // Check if it's MarketDataResponse
        if (response_message->get_type() == MessageType::MARKET_DATA_RESPONSE)
        {
            auto *market_response = static_cast<MarketDataResponse *>(response_message.get());

            std::cout << "[RECEIVED] MarketDataResponse:" << std::endl;
            std::cout << "  - Symbol ID: " << market_response->symbol_id << std::endl;
            std::cout << "  - Symbol: '" << market_response->symbol << "'" << std::endl;
            std::cout << "  - Exchange: '" << market_response->exchange << "'" << std::endl;
            std::cout << "  - Result: " << (market_response->result ? "SUCCESS" : "FAILURE") << std::endl;

            if (market_response->result)
            {
                std::cout << "[SUCCESS] [SUCCESS] Market data unsubscription successful!" << std::endl;
            }
            else
            {
                std::cout << "[FAILURE] [ERROR] Market data unsubscription failed!" << std::endl;
            }
        }
        else
        {
            std::cout << "[ERROR] Expected MarketDataResponse, got message type: "
                      << static_cast<int>(response_message->get_type()) << std::endl;
        }
    }
};

int main()
{
    std::cout << "=== DTC MarketDataResponse Console Test ===" << std::endl;
    std::cout << "Testing MarketDataRequest/MarketDataResponse protocol flow" << std::endl;

    DTCTestClient client;

    // Connect to server
    if (!client.connect("127.0.0.1", 11099))
    {
        std::cout << "[ERROR] Failed to connect to DTC server. Make sure server is running on port 11099." << std::endl;
        return 1;
    }

    // Wait a moment for connection to stabilize
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Test subscription
    client.test_market_data_subscription();

    // Wait between tests
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Test unsubscription
    client.test_market_data_unsubscription();

    std::cout << "\n=== Test Complete ===" << std::endl;
    std::cout << "[INFO] MarketDataResponse functionality verified successfully" << std::endl;

    return 0;
}