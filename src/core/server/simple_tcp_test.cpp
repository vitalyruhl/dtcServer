#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

class SimpleTCPServer
{
private:
    std::atomic<bool> running_{false};
    std::thread server_thread_;
    int port_;

#ifdef _WIN32
    SOCKET server_socket_ = INVALID_SOCKET;
    bool winsock_initialized_ = false;
#else
    int server_socket_ = -1;
#endif

public:
    SimpleTCPServer(int port) : port_(port) {}

    ~SimpleTCPServer()
    {
        stop();
    }

    bool start()
    {
        if (running_)
            return false;

        // Initialize Winsock on Windows
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            std::cout << "WSAStartup failed" << std::endl;
            return false;
        }
        winsock_initialized_ = true;
#endif

        // Create socket
        server_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifdef _WIN32
        if (server_socket_ == INVALID_SOCKET)
        {
#else
        if (server_socket_ < 0)
        {
#endif
            std::cout << "Failed to create socket" << std::endl;
            cleanup();
            return false;
        }

        // Set socket options
        int opt = 1;
#ifdef _WIN32
        setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
#else
        setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

        // Bind socket
        sockaddr_in server_addr = {};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port_);

#ifdef _WIN32
        if (bind(server_socket_, (sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
        {
#else
        if (bind(server_socket_, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
#endif
            std::cout << "Failed to bind to port " << port_ << std::endl;
            cleanup();
            return false;
        }

        // Listen
#ifdef _WIN32
        if (listen(server_socket_, SOMAXCONN) == SOCKET_ERROR)
        {
#else
        if (listen(server_socket_, SOMAXCONN) < 0)
        {
#endif
            std::cout << "Failed to listen on port " << port_ << std::endl;
            cleanup();
            return false;
        }

        running_ = true;
        server_thread_ = std::thread(&SimpleTCPServer::server_loop, this);

        std::cout << "Simple TCP Server started on port " << port_ << std::endl;
        return true;
    }

    void stop()
    {
        if (!running_)
            return;

        running_ = false;

        // Close server socket to break accept()
#ifdef _WIN32
        if (server_socket_ != INVALID_SOCKET)
        {
            closesocket(server_socket_);
            server_socket_ = INVALID_SOCKET;
        }
#else
        if (server_socket_ >= 0)
        {
            close(server_socket_);
            server_socket_ = -1;
        }
#endif

        if (server_thread_.joinable())
        {
            server_thread_.join();
        }

        cleanup();
        std::cout << "Simple TCP Server stopped" << std::endl;
    }

private:
    void server_loop()
    {
        std::cout << "Server listening for connections..." << std::endl;

        while (running_)
        {
            sockaddr_in client_addr = {};
            socklen_t client_len = sizeof(client_addr);

#ifdef _WIN32
            SOCKET client_socket = accept(server_socket_, (sockaddr *)&client_addr, &client_len);
            if (client_socket == INVALID_SOCKET)
            {
                if (running_)
                {
                    std::cout << "Accept failed: " << WSAGetLastError() << std::endl;
                }
                break;
            }
#else
            int client_socket = accept(server_socket_, (sockaddr *)&client_addr, &client_len);
            if (client_socket < 0)
            {
                if (running_)
                {
                    std::cout << "Accept failed" << std::endl;
                }
                break;
            }
#endif

            // Get client IP
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
            std::cout << "✅ New connection from " << client_ip << std::endl;

            // Send a simple response
            std::string response = "Hello from DTC Server!\\n";
#ifdef _WIN32
            send(client_socket, response.c_str(), response.length(), 0);
            closesocket(client_socket);
#else
            send(client_socket, response.c_str(), response.length(), 0);
            close(client_socket);
#endif

            std::cout << "Connection handled and closed" << std::endl;
        }

        std::cout << "Server thread ending" << std::endl;
    }

    void cleanup()
    {
#ifdef _WIN32
        if (winsock_initialized_)
        {
            WSACleanup();
            winsock_initialized_ = false;
        }
#endif
    }
};

int main()
{
    SimpleTCPServer server(11099);

    if (!server.start())
    {
        std::cout << "Failed to start server" << std::endl;
        return 1;
    }

    std::cout << "Server running... Press Enter to stop" << std::endl;
    std::cin.get();

    server.stop();
    return 0;
}