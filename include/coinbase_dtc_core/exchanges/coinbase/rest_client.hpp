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