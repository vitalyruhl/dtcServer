#pragma once

#include "coinbase_dtc_core/core/auth/jwt_auth.hpp"
#include "coinbase_dtc_core/core/auth/cdp_credentials.hpp"
#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace open_dtc_server
{
    namespace exchanges
    {
        namespace coinbase
        {

            // Account balance structure
            struct AccountBalance
            {
                std::string currency;      // e.g., "BTC", "USD", "ETH"
                std::string available;     // Available for trading
                std::string hold;          // On hold (in orders)
                std::string total_balance; // Total = available + hold
                bool active = false;       // Account is active
                std::string account_id;    // UUID of this account
                std::string name;          // Display name
            };

            // Product type enumeration
            enum class ProductType
            {
                ALL,
                SPOT,   // Regular spot trading pairs (BTC-USD, ETH-USD, etc.)
                FUTURE, // Futures contracts
                UNKNOWN // Unknown or other product types
            };

            // Product information structure
            struct Product
            {
                std::string product_id;         // e.g., "BTC-USD", "ETH-USD"
                std::string display_name;       // Display name
                std::string base_currency;      // Base currency (BTC, ETH, etc.)
                std::string quote_currency;     // Quote currency (USD, EUR, etc.)
                ProductType product_type;       // SPOT, FUTURE, etc.
                bool trading_disabled = false;  // Trading enabled/disabled
                std::string status;             // online, offline, etc.
                double price_increment = 0.01;  // Minimum price increment
                double base_min_size = 0.001;   // Minimum order size
                double base_max_size = 10000.0; // Maximum order size
            };

            // Complete portfolio structure
            struct Portfolio
            {
                std::string portfolio_id;
                std::string name;
                bool is_default = false;
                std::vector<AccountBalance> balances;
                double total_value_usd = 0.0; // Calculated total in USD
            };

            /**
             * Coinbase REST API Client for Account/Portfolio Data
             * Handles authenticated requests to Coinbase Advanced Trade API
             */
            class CoinbaseRestClient
            {
            public:
                explicit CoinbaseRestClient(const auth::CDPCredentials &credentials);
                ~CoinbaseRestClient() = default;

                // Account Management
                bool get_accounts(std::vector<AccountBalance> &accounts);
                bool get_account(const std::string &account_id, AccountBalance &account);

                // Portfolio Management
                bool get_portfolios(std::vector<Portfolio> &portfolios);
                bool get_portfolio(const std::string &portfolio_id, Portfolio &portfolio);

                // Account summary with all balances
                bool get_portfolio_summary(Portfolio &summary);

                // Market Data
                bool get_products(std::vector<std::string> &symbols);
                bool get_products_filtered(std::vector<Product> &products, ProductType type = ProductType::ALL);
                bool get_product_types(std::vector<ProductType> &types);

                // Configuration
                void set_sandbox_mode(bool sandbox);
                void set_timeout(int timeout_seconds);

                // Status
                bool test_connection();
                std::string get_last_error() const { return last_error_; }

            private:
                // HTTP request helper
                struct HttpResponse
                {
                    int status_code = 0;
                    std::string body;
                    std::string error_message;
                };

                HttpResponse make_authenticated_request(const std::string &method,
                                                        const std::string &path,
                                                        const std::string &body = "");

                // JSON parsing helpers
                bool parse_accounts_response(const std::string &json, std::vector<AccountBalance> &accounts);
                bool parse_portfolios_response(const std::string &json, std::vector<Portfolio> &portfolios);
                bool parse_account_response(const std::string &json, AccountBalance &account);
                bool parse_products_response(const std::string &json, std::vector<std::string> &symbols);
                bool parse_products_filtered_response(const std::string &json, std::vector<Product> &products, ProductType filter_type);

                // Helper methods
                ProductType parse_product_type(const std::string &product_id) const;
                std::string product_type_to_string(ProductType type) const;

                // URL building
                std::string build_url(const std::string &path) const;

            private:
                auth::CDPCredentials credentials_;
                std::unique_ptr<auth::JWTAuthenticator> authenticator_;
                std::string base_url_;
                bool sandbox_mode_;
                int timeout_seconds_;
                std::string last_error_;
            };

        } // namespace coinbase
    } // namespace exchanges
} // namespace open_dtc_server