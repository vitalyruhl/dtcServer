#include "coinbase_dtc_core/exchanges/coinbase/rest_client.hpp"
#include "coinbase_dtc_core/core/auth/cdp_credentials.hpp"
#include "coinbase_dtc_core/core/util/log.hpp"
#include <iostream>
#include <iomanip>

using namespace open_dtc_server;

void print_account_balance(const exchanges::coinbase::AccountBalance &balance)
{
    std::cout << "  [" << balance.currency << "] " << balance.name << std::endl;
    std::cout << "    Account ID: " << balance.account_id << std::endl;
    std::cout << "    Available:  " << balance.available << " " << balance.currency << std::endl;
    std::cout << "    On Hold:    " << balance.hold << " " << balance.currency << std::endl;
    std::cout << "    Total:      " << balance.total_balance << " " << balance.currency << std::endl;
    std::cout << "    Active:     " << (balance.active ? "Yes" : "No") << std::endl;
    std::cout << std::endl;
}

void print_portfolio_summary(const exchanges::coinbase::Portfolio &portfolio)
{
    std::cout << "=== PORTFOLIO SUMMARY ===" << std::endl;
    std::cout << "Portfolio: " << portfolio.name << " (ID: " << portfolio.portfolio_id << ")" << std::endl;
    std::cout << "Default: " << (portfolio.is_default ? "Yes" : "No") << std::endl;
    std::cout << "Total USD Value: $" << std::fixed << std::setprecision(2) << portfolio.total_value_usd << std::endl;
    std::cout << "Number of Assets: " << portfolio.balances.size() << std::endl;
    std::cout << std::endl;

    double total_usd = 0.0;
    for (const auto &balance : portfolio.balances)
    {
        print_account_balance(balance);

        // Sum up USD values
        if (balance.currency == "USD" || balance.currency == "USDC")
        {
            try
            {
                total_usd += std::stod(balance.total_balance);
            }
            catch (...)
            {
            }
        }
    }

    std::cout << "Total Calculated USD: $" << std::fixed << std::setprecision(2) << total_usd << std::endl;
}

int main()
{
    std::cout << "=== COINBASE ACCOUNT INFO TEST ===" << std::endl;
    std::cout << "Testing real Coinbase API integration..." << std::endl
              << std::endl;

    try
    {
        // Load credentials
        std::cout << "Loading Coinbase credentials..." << std::endl;
        auto credentials = auth::CDPCredentials::from_json_file("secrets/coinbase/cdp_api_key_ECDSA.json");

        if (!credentials.is_valid())
        {
            std::cerr << "ERROR: Invalid or missing credentials in secrets/coinbase/cdp_api_key_ECDSA.json" << std::endl;
            std::cerr << "Please ensure you have valid Coinbase API credentials." << std::endl;
            return 1;
        }

        std::cout << "Credentials loaded successfully." << std::endl;
        std::cout << "API Key ID: " << credentials.key_id << std::endl
                  << std::endl;

        // Create REST client
        exchanges::coinbase::CoinbaseRestClient client(credentials);

        // Test connection first
        std::cout << "Testing connection to Coinbase API..." << std::endl;
        if (!client.test_connection())
        {
            std::cerr << "ERROR: Connection test failed: " << client.get_last_error() << std::endl;
            return 1;
        }
        std::cout << "Connection successful!" << std::endl
                  << std::endl;

        // Get portfolio summary (all account balances)
        std::cout << "Fetching complete portfolio information..." << std::endl;
        exchanges::coinbase::Portfolio summary;
        if (client.get_portfolio_summary(summary))
        {
            print_portfolio_summary(summary);
        }
        else
        {
            std::cerr << "ERROR: Failed to get portfolio summary: " << client.get_last_error() << std::endl;
            return 1;
        }

        // Also test individual accounts call
        std::cout << "=== DETAILED ACCOUNT BREAKDOWN ===" << std::endl;
        std::vector<exchanges::coinbase::AccountBalance> accounts;
        if (client.get_accounts(accounts))
        {
            std::cout << "Found " << accounts.size() << " accounts:" << std::endl
                      << std::endl;

            for (const auto &account : accounts)
            {
                print_account_balance(account);
            }
        }
        else
        {
            std::cerr << "ERROR: Failed to get accounts: " << client.get_last_error() << std::endl;
            return 1;
        }

        // Test portfolios endpoint
        std::cout << "=== PORTFOLIO MANAGEMENT ===" << std::endl;
        std::vector<exchanges::coinbase::Portfolio> portfolios;
        if (client.get_portfolios(portfolios))
        {
            std::cout << "Found " << portfolios.size() << " portfolios:" << std::endl;
            for (const auto &portfolio : portfolios)
            {
                std::cout << "  - " << portfolio.name << " (Default: " << (portfolio.is_default ? "Yes" : "No") << ")" << std::endl;
            }
        }
        else
        {
            std::cout << "Note: Portfolios endpoint failed (may not be available): " << client.get_last_error() << std::endl;
        }

        std::cout << std::endl
                  << "=== TEST COMPLETED SUCCESSFULLY ===" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
        return 1;
    }
}