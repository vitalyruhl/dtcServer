#pragma once

#ifndef COINBASE_API_MOCK_H
#define COINBASE_API_MOCK_H

#include <string>
#include <map>
#include <functional>

namespace coinbase_dtc_core {
namespace test {

/**
 * Mock HTTP Client for Testing
 * 
 * This class simulates Coinbase API responses without making real HTTP calls.
 * Used in unit tests and CI/CD environments.
 */

class MockHttpClient {
public:
    struct HttpResponse {
        int status_code;
        std::string body;
        std::map<std::string, std::string> headers;
    };

    // Mock responses for different endpoints
    static MockHttpClient& getInstance() {
        static MockHttpClient instance;
        return instance;
    }

    // Configure mock responses
    void setMockResponse(const std::string& url, const HttpResponse& response) {
        mock_responses_[url] = response;
    }

    // Simulate HTTP GET request
    HttpResponse get(const std::string& url) {
        if (mock_responses_.find(url) != mock_responses_.end()) {
            return mock_responses_[url];
        }
        
        // Default 404 response for unmocked URLs
        return {404, R"({"message": "Mock endpoint not configured"})", {}};
    }

    // Setup default Coinbase API mock responses
    void setupCoinbaseMocks() {
        // Mock BTC-USD ticker
        setMockResponse(
            "https://api.exchange.coinbase.com/products/BTC-USD/ticker",
            {200, R"({
                "trade_id": 12345,
                "price": "50000.00",
                "size": "0.01",
                "bid": "49999.00", 
                "ask": "50001.00",
                "volume": "123.45",
                "time": "2023-01-01T12:00:00Z"
            })", {{"content-type", "application/json"}}}
        );

        // Mock BTC-USD candles
        setMockResponse(
            "https://api.exchange.coinbase.com/products/BTC-USD/candles",
            {200, R"([
                [1609459200, 29000, 31000, 28500, 30500, 150.25],
                [1609462800, 30500, 32000, 30000, 31500, 200.50],
                [1609466400, 31500, 33000, 31000, 32500, 180.75]
            ])", {{"content-type", "application/json"}}}
        );

        // Mock products list
        setMockResponse(
            "https://api.exchange.coinbase.com/products",
            {200, R"([
                {
                    "id": "BTC-USD",
                    "base_currency": "BTC",
                    "quote_currency": "USD",
                    "base_increment": "0.00000001",
                    "quote_increment": "0.01",
                    "display_name": "BTC/USD",
                    "min_market_funds": "1.00",
                    "max_market_funds": "100000.00",
                    "trading_disabled": false
                },
                {
                    "id": "ETH-USD",
                    "base_currency": "ETH", 
                    "quote_currency": "USD",
                    "base_increment": "0.00000001",
                    "quote_increment": "0.01",
                    "display_name": "ETH/USD",
                    "min_market_funds": "1.00",
                    "max_market_funds": "100000.00",
                    "trading_disabled": false
                }
            ])", {{"content-type", "application/json"}}}
        );

        // Mock order book
        setMockResponse(
            "https://api.exchange.coinbase.com/products/BTC-USD/book",
            {200, R"({
                "sequence": 3,
                "bids": [
                    ["49999.00", "0.5", 1],
                    ["49998.00", "1.0", 2]
                ],
                "asks": [
                    ["50001.00", "0.3", 1],
                    ["50002.00", "0.8", 1]
                ]
            })", {{"content-type", "application/json"}}}
        );

        // Mock rate limit error (for testing error handling)
        setMockResponse(
            "https://api.exchange.coinbase.com/test/rate-limit",
            {429, R"({
                "message": "rate limit exceeded",
                "error_details": {
                    "rate_limit": {
                        "max_requests": 10,
                        "window_size": 1,
                        "current_requests": 11
                    }
                }
            })", {{"content-type", "application/json"}}}
        );

        // Mock server error (for testing error handling)
        setMockResponse(
            "https://api.exchange.coinbase.com/test/server-error",
            {500, R"({"message": "internal server error"})", {{"content-type", "application/json"}}}
        );
    }

    // Enable/disable mocking (for integration tests)
    void setMockingEnabled(bool enabled) {
        mocking_enabled_ = enabled;
    }

    bool isMockingEnabled() const {
        return mocking_enabled_;
    }

    // Clear all mock responses
    void clearMocks() {
        mock_responses_.clear();
    }

private:
    std::map<std::string, HttpResponse> mock_responses_;
    bool mocking_enabled_ = true;

    MockHttpClient() = default;
};

/**
 * Mock Environment Variables
 * 
 * For testing environment variable handling
 */
class MockEnvironment {
public:
    static void setEnvVar(const std::string& name, const std::string& value) {
        env_vars_[name] = value;
    }

    static std::string getEnvVar(const std::string& name) {
        if (env_vars_.find(name) != env_vars_.end()) {
            return env_vars_[name];
        }
        return "";
    }

    static void clearEnvVars() {
        env_vars_.clear();
    }

    static void setupTestEnvironment() {
        // Setup test environment variables
        setEnvVar("COINBASE_API_KEY", "");
        setEnvVar("COINBASE_API_SECRET", "");
        setEnvVar("COINBASE_PASSPHRASE", "");
        setEnvVar("COINBASE_API_URL", "https://api.exchange.coinbase.com");
    }

private:
    static std::map<std::string, std::string> env_vars_;
};

} // namespace test
} // namespace coinbase_dtc_core

#endif // COINBASE_API_MOCK_H