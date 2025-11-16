#include "coinbase_dtc_core/core/dtc/protocol.hpp"
#include "coinbase_dtc_core/core/util/log.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
typedef int SOCKET;
#endif

using namespace open_dtc_server::core::dtc;

class SimpleDTCClient
{
private:
    SOCKET socket_;
    bool connected_;
    Protocol protocol_;
    std::vector<uint8_t> buffer_;

public:
    SimpleDTCClient() : socket_(INVALID_SOCKET), connected_(false) {}

    bool connect_to_server(const std::string &host, uint16_t port)
    {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            std::cout << "WSAStartup failed" << std::endl;
            return false;
        }
#endif

        socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_ == INVALID_SOCKET)
        {
            std::cout << "Failed to create socket" << std::endl;
            return false;
        }

        sockaddr_in server_addr = {};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr);

        if (::connect(socket_, (sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
        {
            std::cout << "Failed to connect to server" << std::endl;
            return false;
        }

        connected_ = true;
        std::cout << "Connected to " << host << ":" << port << std::endl;
        return true;
    }

    bool send_logon()
    {
        if (!connected_)
            return false;

        LogonRequest logon;
        logon.client_name = "TestConsoleClient";
        logon.username = "testuser";

        auto message_data = protocol_.create_message(logon);
        return send_raw_data(message_data);
    }

    bool send_security_definition_request(const std::string &product_type)
    {
        if (!connected_)
            return false;

        SecurityDefinitionForSymbolRequest request;
        request.request_id = 1;
        request.symbol = ""; // Empty for "get all symbols"
        request.exchange = "coinbase";
        request.product_type = product_type;

        auto message_data = protocol_.create_message(request);
        std::cout << "Sending SecurityDefinitionRequest with product_type: " << product_type << std::endl;
        return send_raw_data(message_data);
    }

    void process_messages()
    {
        if (!connected_)
            return;

        char temp_buffer[4096];
        int bytes_received = recv(socket_, temp_buffer, sizeof(temp_buffer), 0);

        if (bytes_received <= 0)
        {
            std::cout << "Connection lost or no data" << std::endl;
            return;
        }

        // Add to buffer
        buffer_.insert(buffer_.end(), temp_buffer, temp_buffer + bytes_received);

        // Process complete messages
        while (buffer_.size() >= 4)
        {
            uint16_t message_size = *reinterpret_cast<const uint16_t *>(buffer_.data());

            if (message_size < 4 || message_size > 65535)
            {
                std::cout << "Invalid message size: " << message_size << std::endl;
                buffer_.clear();
                return;
            }

            if (buffer_.size() < message_size)
            {
                break; // Need more data
            }

            // Parse message
            auto message = protocol_.parse_message(buffer_.data(), message_size);
            if (message)
            {
                handle_message(message.get());
            }

            // Remove processed message
            buffer_.erase(buffer_.begin(), buffer_.begin() + message_size);
        }
    }

private:
    bool send_raw_data(const std::vector<uint8_t> &data)
    {
        if (!connected_)
            return false;

        int result = send(socket_, (const char *)data.data(), data.size(), 0);
        return result != SOCKET_ERROR;
    }

    void handle_message(DTCMessage *message)
    {
        switch (message->get_type())
        {
        case MessageType::LOGON_RESPONSE:
        {
            auto *logon = static_cast<LogonResponse *>(message);
            std::cout << "LogonResponse received: result=" << logon->result
                      << ", text='" << logon->result_text << "'" << std::endl;
            break;
        }

        case MessageType::SECURITY_DEFINITION_RESPONSE:
        {
            auto *symbol_resp = static_cast<SecurityDefinitionResponse *>(message);
            std::cout << "SecurityDefinitionResponse received:" << std::endl;
            std::cout << "  Symbol: '" << symbol_resp->symbol << "'" << std::endl;
            std::cout << "  Exchange: '" << symbol_resp->exchange << "'" << std::endl;
            std::cout << "  Description: '" << symbol_resp->description << "'" << std::endl;
            std::cout << "  Min Tick: " << symbol_resp->min_price_increment << std::endl;
            std::cout << "  Request ID: " << symbol_resp->request_id << std::endl;
            std::cout << std::endl;
            break;
        }

        default:
            std::cout << "Received message type: " << static_cast<uint16_t>(message->get_type()) << std::endl;
            break;
        }
    }
};

int main()
{
    std::cout << "=== DTC Console Test Client ===" << std::endl;

    SimpleDTCClient client;

    // Connect to server
    if (!client.connect_to_server("127.0.0.1", 11099))
    {
        std::cout << "Failed to connect to DTC server" << std::endl;
        return 1;
    }

    // Send logon
    if (!client.send_logon())
    {
        std::cout << "Failed to send logon" << std::endl;
        return 1;
    }

    // Wait for logon response
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    client.process_messages();

    // Send security definition request
    if (!client.send_security_definition_request("SPOT"))
    {
        std::cout << "Failed to send security definition request" << std::endl;
        return 1;
    }

    // Wait and process responses
    std::cout << "Waiting for SecurityDefinition responses..." << std::endl;
    for (int i = 0; i < 50; i++)
    { // Wait up to 5 seconds
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        client.process_messages();
    }

    std::cout << "Test completed." << std::endl;
    return 0;
}