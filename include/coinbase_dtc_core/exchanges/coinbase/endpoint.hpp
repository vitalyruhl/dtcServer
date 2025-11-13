#pragma once

#ifndef COINBASE_DTC_ENDPOINTS_HPP
#define COINBASE_DTC_ENDPOINTS_HPP

#include <string>

namespace open_dtc_server
{
    namespace endpoints
    {

        // Base URLs
        inline constexpr const char *TRADE_BASE = "https://api.coinbase.com/api/v3/brokerage/";
        inline constexpr const char *SANDBOX_BASE = "https://api-sandbox.coinbase.com/api/v3/brokerage/";

        // Public market data endpoints (only available on production, not sandbox!)
        inline constexpr const char *TIME = "time";                               // GET
        inline constexpr const char *MARKET_PRODUCT_BOOK = "market/product_book"; // GET
        inline constexpr const char *MARKET_PRODUCTS = "market/products";         // GET
        inline constexpr const char *MARKET_PRODUCT = "market/products/";         // GET + product_id appended
        inline constexpr const char *MARKET_PRODUCT_CANDLES = "market/products/"; // + {product_id}/candles
        inline constexpr const char *MARKET_TICKER = "market/products/";          // + {product_id}/ticker

        // Private endpoints (available in both sandbox and production with auth)
        inline constexpr const char *ACCOUNTS = "accounts";                         // GET
        inline constexpr const char *ACCOUNT = "accounts/";                         // GET + account_id
        inline constexpr const char *ORDERS = "orders";                             // POST/GET
        inline constexpr const char *ORDERS_BATCH_CANCEL = "orders/batch_cancel";   // POST
        inline constexpr const char *ORDERS_HISTORICAL = "orders/historical/batch"; // GET
        inline constexpr const char *ORDERS_FILLS = "orders/historical/fills";      // GET
        inline constexpr const char *ORDERS_PREVIEW = "orders/preview";             // POST

        // Helper: build full URL from base + resource (resource should not include base)
        inline std::string make_url(const std::string &base, const std::string &resource)
        {
            // Ensure trailing slash on base
            if (!base.empty() && base.back() != '/')
            {
                return base + "/" + resource;
            }
            return base + resource;
        }

        // Helper overload that appends a path component (e.g., product id)
        inline std::string make_url_with_id(const std::string &base, const std::string &resource_prefix, const std::string &id, const std::string &suffix = "")
        {
            std::string res = std::string(resource_prefix) + id;
            if (!suffix.empty())
                res += "/" + suffix;
            return make_url(base, res);
        }

    } // namespace endpoints
} // namespace open_dtc_server

#endif // COINBASE_DTC_ENDPOINTS_HPP
