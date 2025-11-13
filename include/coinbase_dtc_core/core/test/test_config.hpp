#pragma once

#ifndef COINBASE_TEST_CONFIG_H
#define COINBASE_TEST_CONFIG_H

#include <string>

namespace open_dtc_server
{
    namespace test_config
    {

        /**
         * Test Configuration for CI/CD and Public Testing
         *
         * This file provides safe default values for testing without real credentials.
         * It's safe to commit to version control as it contains no secrets.
         *
         * For local development with real credentials, see secrets/coinbase.h
         */

        // Test credentials (empty = public API mode)
        const std::string TEST_API_KEY = "";
        const std::string TEST_API_SECRET = "";
        const std::string TEST_PASSPHRASE = "";

        // Test configuration
        const std::string TEST_PRODUCT = "BTC-USD"; // Use most stable/liquid pair for testing
        const bool USE_SANDBOX = false;             // Set to true if Coinbase provides sandbox
        const int TEST_TIMEOUT_SECONDS = 10;        // Shorter timeout for tests

        // Mock data for offline testing
        namespace mock_data
        {
            // Sample ticker response for unit tests
            const std::string SAMPLE_TICKER_JSON = R"({
        "trade_id": 12345,
        "price": "50000.00",
        "size": "0.01",
        "bid": "49999.00",
        "ask": "50001.00",
        "volume": "123.45",
        "time": "2023-01-01T12:00:00Z"
    })";

            // Sample candles response for unit tests
            const std::string SAMPLE_CANDLES_JSON = R"([
        [1609459200, 29000, 31000, 28500, 30500, 150.25],
        [1609462800, 30500, 32000, 30000, 31500, 200.50]
    ])";

            // Sample products response
            const std::string SAMPLE_PRODUCTS_JSON = R"([
        {
            "id": "BTC-USD",
            "base_currency": "BTC",
            "quote_currency": "USD",
            "quote_increment": "0.01",
            "base_increment": "0.00000001",
            "display_name": "BTC/USD",
            "min_market_funds": "1.00",
            "max_market_funds": "100000.00",
            "margin_enabled": false,
            "cancel_only": false,
            "limit_only": false,
            "post_only": false,
            "trading_disabled": false,
            "auction_mode": false
        }
    ])";
        }

    } // namespace test_config
} // namespace open_dtc_server

#endif // COINBASE_TEST_CONFIG_H