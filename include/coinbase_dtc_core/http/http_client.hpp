#pragma once

#ifndef COINBASE_HTTP_CLIENT_H
#define COINBASE_HTTP_CLIENT_H

#include <string>
#include <memory>

namespace coinbase_dtc_core {
namespace http {

/**
 * Abstract HTTP Client Interface
 * 
 * This interface allows switching between real HTTP client and mock client
 * for testing purposes.
 */

struct HttpResponse {
    int status_code;
    std::string body;
    std::string error_message;
    bool success;
    
    HttpResponse(int code, const std::string& data) 
        : status_code(code), body(data), success(code >= 200 && code < 300) {}
};

class IHttpClient {
public:
    virtual ~IHttpClient() = default;
    virtual HttpResponse get(const std::string& url) = 0;
    virtual HttpResponse post(const std::string& url, const std::string& data) = 0;
};

/**
 * Real HTTP Client (using libcurl or similar)
 * Will be implemented later with actual HTTP library
 */
class RealHttpClient : public IHttpClient {
public:
    HttpResponse get(const std::string& url) override {
        // TODO: Implement real HTTP GET using libcurl
        // For now, return error to indicate not implemented
        return {501, R"({"error": "Real HTTP client not implemented yet"})"};
    }

    HttpResponse post(const std::string& url, const std::string& data) override {
        // TODO: Implement real HTTP POST using libcurl
        return {501, R"({"error": "Real HTTP client not implemented yet"})"};
    }
};

/**
 * Mock HTTP Client for Testing
 */
class MockHttpClient : public IHttpClient {
public:
    HttpResponse get(const std::string& url) override;
    HttpResponse post(const std::string& url, const std::string& data) override;
    
    void setupDefaultMocks();
    void setMockResponse(const std::string& url, const HttpResponse& response);

private:
    std::map<std::string, HttpResponse> mock_responses_;
};

/**
 * HTTP Client Factory
 * 
 * Creates appropriate client based on environment/configuration
 */
class HttpClientFactory {
public:
    static std::unique_ptr<IHttpClient> create() {
        // Check if we're in test mode
        const char* test_mode = std::getenv("COINBASE_TEST_MODE");
        if (test_mode && std::string(test_mode) == "mock") {
            auto mock_client = std::make_unique<MockHttpClient>();
            mock_client->setupDefaultMocks();
            return std::move(mock_client);
        }
        
        // Return real HTTP client for production
        return std::make_unique<RealHttpClient>();
    }
};

} // namespace http
} // namespace coinbase_dtc_core

#endif // COINBASE_HTTP_CLIENT_H