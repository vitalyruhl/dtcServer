#include "coinbase_dtc_core/exchanges/coinbase/rest_client.hpp"
#include "coinbase_dtc_core/exchanges/coinbase/endpoint.hpp"
#include "coinbase_dtc_core/core/util/advanced_log.hpp"
#include <curl/curl.h>
#include <stdexcept>
#include <sstream>

namespace open_dtc_server
{
    namespace exchanges
    {
        namespace coinbase
        {

            // Helper callback for CURL to write response data
            static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
            {
                ((std::string *)userp)->append((char *)contents, size * nmemb);
                return size * nmemb;
            }

            CoinbaseRestClient::CoinbaseRestClient(const auth::CDPCredentials &credentials)
                : credentials_(credentials), sandbox_mode_(false), timeout_seconds_(30)
            {
                if (!credentials_.is_valid())
                {
                    throw std::invalid_argument("Invalid Coinbase credentials provided");
                }

                authenticator_ = std::make_unique<auth::JWTAuthenticator>(credentials_);
                base_url_ = endpoints::TRADE_BASE; // Production by default

                LOG_INFO("[COINBASE-REST] Initialized REST client");
            }

            void CoinbaseRestClient::set_sandbox_mode(bool sandbox)
            {
                sandbox_mode_ = sandbox;
                base_url_ = sandbox ? endpoints::SANDBOX_BASE : endpoints::TRADE_BASE;
                LOG_INFO("[COINBASE-REST] Switched to " + std::string(sandbox ? "sandbox" : "production") + " mode");
            }

            void CoinbaseRestClient::set_timeout(int timeout_seconds)
            {
                timeout_seconds_ = timeout_seconds;
            }

            bool CoinbaseRestClient::test_connection()
            {
                LOG_INFO("[COINBASE-REST] Testing connection to: " + base_url_);
                auto response = make_authenticated_request("GET", "time", "");

                LOG_INFO("[COINBASE-REST] Connection test result - Status: " + std::to_string(response.status_code) + ", Body: " + response.body + ", Error: " + response.error_message);

                if (response.status_code == 200)
                {
                    LOG_INFO("[COINBASE-REST] Connection test successful");
                    return true;
                }
                else
                {
                    last_error_ = "Connection test failed: HTTP " + std::to_string(response.status_code) + " - " + response.body + " | " + response.error_message;
                    LOG_INFO("[COINBASE-REST] " + last_error_);
                    return false;
                }
            }
            bool CoinbaseRestClient::get_accounts(std::vector<AccountBalance> &accounts)
            {
                LOG_INFO("[COINBASE-REST] Fetching accounts...");

                auto response = make_authenticated_request("GET", "accounts", "");

                if (response.status_code != 200)
                {
                    last_error_ = "Failed to get accounts: HTTP " + std::to_string(response.status_code) + " - " + response.body;
                    LOG_INFO("[COINBASE-REST] " + last_error_);
                    return false;
                }

                return parse_accounts_response(response.body, accounts);
            }

            bool CoinbaseRestClient::get_portfolios(std::vector<Portfolio> &portfolios)
            {
                LOG_INFO("[COINBASE-REST] Fetching portfolios...");

                auto response = make_authenticated_request("GET", "portfolios", "");

                if (response.status_code != 200)
                {
                    last_error_ = "Failed to get portfolios: HTTP " + std::to_string(response.status_code) + " - " + response.body;
                    LOG_INFO("[COINBASE-REST] " + last_error_);
                    return false;
                }

                return parse_portfolios_response(response.body, portfolios);
            }

            bool CoinbaseRestClient::get_products(std::vector<std::string> &symbols)
            {
                LOG_INFO("[COINBASE-REST] Fetching available products/symbols...");

                auto response = make_authenticated_request("GET", "market/products", "");

                if (response.status_code != 200)
                {
                    last_error_ = "Failed to get products: HTTP " + std::to_string(response.status_code) + " - " + response.body;
                    LOG_INFO("[COINBASE-REST] " + last_error_);
                    return false;
                }

                return parse_products_response(response.body, symbols);
            }

            bool CoinbaseRestClient::get_products_filtered(std::vector<Product> &products, ProductType type)
            {
                LOG_INFO("[COINBASE-REST] *** get_products_filtered CALLED *** Type: " + product_type_to_string(type));

                LOG_INFO("[COINBASE-REST] Making authenticated request to market/products...");
                auto response = make_authenticated_request("GET", "market/products", "");

                LOG_INFO("[COINBASE-REST] API Response - Status Code: " + std::to_string(response.status_code));
                if (response.status_code != 200)
                {
                    last_error_ = "Failed to get products: HTTP " + std::to_string(response.status_code) + " - " + response.body;
                    LOG_INFO("[COINBASE-REST] *** API ERROR *** " + last_error_);
                    return false;
                }

                LOG_INFO("[COINBASE-REST] API SUCCESS - Response body length: " + std::to_string(response.body.length()));
                LOG_INFO("[COINBASE-REST] Response preview: " + response.body.substr(0, 200) + "...");

                return parse_products_filtered_response(response.body, products, type);
            }

            bool CoinbaseRestClient::get_product_types(std::vector<ProductType> &types)
            {
                types.clear();
                types.push_back(ProductType::ALL);
                types.push_back(ProductType::SPOT);
                types.push_back(ProductType::FUTURE);

                LOG_INFO("[COINBASE-REST] Available product types: ALL, SPOT, FUTURE");
                return true;
            }

            bool CoinbaseRestClient::get_portfolio_summary(Portfolio &summary)
            {
                LOG_INFO("[COINBASE-REST] Getting portfolio summary...");

                // Get all accounts first
                std::vector<AccountBalance> accounts;
                if (!get_accounts(accounts))
                {
                    return false;
                }

                // Create summary portfolio
                summary.portfolio_id = "default";
                summary.name = "Default Portfolio";
                summary.is_default = true;
                summary.balances = std::move(accounts);
                summary.total_value_usd = 0.0;

                // Calculate total value (would need price data for accurate calculation)
                for (const auto &balance : summary.balances)
                {
                    if (balance.currency == "USD" || balance.currency == "USDC")
                    {
                        try
                        {
                            summary.total_value_usd += std::stod(balance.total_balance);
                        }
                        catch (const std::exception &)
                        {
                            // Ignore parsing errors
                        }
                    }
                }

                LOG_INFO("[COINBASE-REST] Portfolio summary: " + std::to_string(summary.balances.size()) + " accounts, $" + std::to_string(summary.total_value_usd) + " USD value");
                return true;
            }

            CoinbaseRestClient::HttpResponse CoinbaseRestClient::make_authenticated_request(const std::string &method, const std::string &path, const std::string &body)
            {
                HttpResponse result;

                CURL *curl = curl_easy_init();
                if (!curl)
                {
                    result.error_message = "Failed to initialize CURL";
                    return result;
                }

                try
                {
                    std::string url = build_url(path);
                    std::string response_body;

                    // Generate JWT token for this request
                    // The path needs to be the full API path starting with /api
                    std::string jwt_path = "/api/v3/brokerage/" + path;
                    std::string jwt_token = authenticator_->generate_token(method, jwt_path, body);

                    // Set up CURL options
                    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
                    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)timeout_seconds_);
                    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
                    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

                    // Set headers
                    struct curl_slist *headers = nullptr;
                    std::string auth_header = "Authorization: Bearer " + jwt_token;
                    std::string content_type = "Content-Type: application/json";

                    headers = curl_slist_append(headers, auth_header.c_str());
                    headers = curl_slist_append(headers, content_type.c_str());
                    headers = curl_slist_append(headers, "User-Agent: coinbase-dtc-core/1.0");

                    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

                    // Set method-specific options
                    if (method == "POST")
                    {
                        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
                    }
                    else if (method == "GET")
                    {
                        // Default behavior
                    }

                    // Perform the request
                    CURLcode res = curl_easy_perform(curl);

                    if (res == CURLE_OK)
                    {
                        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.status_code);
                        result.body = response_body;
                    }
                    else
                    {
                        result.error_message = "CURL error: " + std::string(curl_easy_strerror(res));
                        result.status_code = -1;
                    }

                    // Cleanup
                    curl_slist_free_all(headers);
                }
                catch (const std::exception &e)
                {
                    result.error_message = "Request exception: " + std::string(e.what());
                    result.status_code = -1;
                }

                curl_easy_cleanup(curl);
                return result;
            }

            bool CoinbaseRestClient::parse_accounts_response(const std::string &json, std::vector<AccountBalance> &accounts)
            {
                try
                {
                    auto parsed = nlohmann::json::parse(json);

                    if (!parsed.contains("accounts") || !parsed["accounts"].is_array())
                    {
                        last_error_ = "Invalid accounts response format";
                        return false;
                    }

                    accounts.clear();
                    for (const auto &account_json : parsed["accounts"])
                    {
                        AccountBalance balance;
                        balance.account_id = account_json.value("uuid", "");
                        balance.name = account_json.value("name", "");
                        balance.currency = account_json.value("currency", "");
                        balance.active = account_json.value("active", false);

                        // Parse available_balance
                        if (account_json.contains("available_balance") && account_json["available_balance"].contains("value"))
                        {
                            balance.available = account_json["available_balance"]["value"];
                        }

                        // Parse hold
                        if (account_json.contains("hold") && account_json["hold"].contains("value"))
                        {
                            balance.hold = account_json["hold"]["value"];
                        }

                        // Calculate total balance
                        try
                        {
                            double avail = balance.available.empty() ? 0.0 : std::stod(balance.available);
                            double held = balance.hold.empty() ? 0.0 : std::stod(balance.hold);
                            balance.total_balance = std::to_string(avail + held);
                        }
                        catch (const std::exception &)
                        {
                            balance.total_balance = "0";
                        }

                        // Only include accounts with some balance or active status
                        if (balance.active || (!balance.available.empty() && balance.available != "0" && balance.available != "0.00"))
                        {
                            accounts.push_back(balance);
                        }
                    }

                    LOG_INFO("[COINBASE-REST] Parsed " + std::to_string(accounts.size()) + " account balances");
                    return true;
                }
                catch (const std::exception &e)
                {
                    last_error_ = "JSON parsing error: " + std::string(e.what());
                    LOG_INFO("[COINBASE-REST] " + last_error_);
                    return false;
                }
            }

            bool CoinbaseRestClient::parse_portfolios_response(const std::string &json, std::vector<Portfolio> &portfolios)
            {
                try
                {
                    auto parsed = nlohmann::json::parse(json);

                    if (!parsed.contains("portfolios") || !parsed["portfolios"].is_array())
                    {
                        last_error_ = "Invalid portfolios response format";
                        return false;
                    }

                    portfolios.clear();
                    for (const auto &portfolio_json : parsed["portfolios"])
                    {
                        Portfolio portfolio;
                        portfolio.portfolio_id = portfolio_json.value("name", ""); // Coinbase uses 'name' as ID
                        portfolio.name = portfolio_json.value("name", "");
                        portfolio.is_default = portfolio_json.value("deleted", false) == false; // Active portfolios

                        portfolios.push_back(portfolio);
                    }

                    LOG_INFO("[COINBASE-REST] Parsed " + std::to_string(portfolios.size()) + " portfolios");
                    return true;
                }
                catch (const std::exception &e)
                {
                    last_error_ = "JSON parsing error: " + std::string(e.what());
                    LOG_INFO("[COINBASE-REST] " + last_error_);
                    return false;
                }
            }

            bool CoinbaseRestClient::parse_products_response(const std::string &json, std::vector<std::string> &symbols)
            {
                try
                {
                    auto parsed = nlohmann::json::parse(json);
                    if (!parsed.contains("products"))
                    {
                        last_error_ = "Invalid products response: missing 'products' field";
                        return false;
                    }

                    symbols.clear();
                    for (const auto &product : parsed["products"])
                    {
                        std::string product_id = product.value("product_id", "");
                        std::string status = product.value("status", "");
                        bool trading_disabled = product.value("trading_disabled", true);

                        // Only include active trading pairs
                        if (!product_id.empty() && status == "online" && !trading_disabled)
                        {
                            symbols.push_back(product_id);
                        }
                    }

                    LOG_INFO("[COINBASE-REST] Parsed " + std::to_string(symbols.size()) + " active trading symbols");
                    return true;
                }
                catch (const std::exception &e)
                {
                    last_error_ = "Error parsing products response: " + std::string(e.what());
                    return false;
                }
            }

            bool CoinbaseRestClient::parse_products_filtered_response(const std::string &json, std::vector<Product> &products, ProductType filter_type)
            {
                LOG_INFO("[COINBASE-REST] *** parse_products_filtered_response CALLED ***");
                LOG_INFO("[COINBASE-REST] Filter type: " + product_type_to_string(filter_type));
                LOG_INFO("[COINBASE-REST] JSON length: " + std::to_string(json.length()));

                try
                {
                    LOG_INFO("[COINBASE-REST] Parsing JSON response...");
                    auto parsed = nlohmann::json::parse(json);

                    if (!parsed.contains("products"))
                    {
                        last_error_ = "Invalid products response: missing 'products' field";
                        LOG_INFO("[COINBASE-REST] *** JSON ERROR *** " + last_error_);
                        return false;
                    }

                    LOG_INFO("[COINBASE-REST] Found products array with " + std::to_string(parsed["products"].size()) + " items");
                    products.clear();
                    int processed_count = 0;
                    int filtered_count = 0;

                    for (const auto &product_json : parsed["products"])
                    {
                        processed_count++;
                        std::string product_id = product_json.value("product_id", "");
                        std::string status = product_json.value("status", "");
                        bool trading_disabled = product_json.value("trading_disabled", true);

                        // Only include active trading pairs
                        if (product_id.empty() || status != "online" || trading_disabled)
                        {
                            continue;
                        }

                        Product product;
                        product.product_id = product_id;
                        product.display_name = product_json.value("display_name", product_id);
                        product.base_currency = product_json.value("base_currency", "");
                        product.quote_currency = product_json.value("quote_currency", "");
                        product.status = status;
                        product.trading_disabled = trading_disabled;
                        product.product_type = parse_product_type(product_id);

                        // Parse numeric values safely
                        try
                        {
                            product.price_increment = std::stod(product_json.value("price_increment", "0.01"));
                            product.base_min_size = std::stod(product_json.value("base_min_size", "0.001"));
                            product.base_max_size = std::stod(product_json.value("base_max_size", "10000"));
                        }
                        catch (...)
                        {
                            // Use defaults if parsing fails
                            product.price_increment = 0.01;
                            product.base_min_size = 0.001;
                            product.base_max_size = 10000.0;
                        }

                        // Apply filter
                        if (filter_type == ProductType::ALL || product.product_type == filter_type)
                        {
                            products.push_back(product);
                            filtered_count++;
                            if (filtered_count <= 5)
                            {
                                LOG_INFO("[COINBASE-REST] Added product: " + product_id + " (type: " + product_type_to_string(product.product_type) + ")");
                            }
                        }
                    }

                    LOG_INFO("[COINBASE-REST] *** PARSING COMPLETE *** Processed: " + std::to_string(processed_count) + ", Filtered: " + std::to_string(filtered_count) + " (" + product_type_to_string(filter_type) + ")");
                    return true;
                }
                catch (const std::exception &e)
                {
                    last_error_ = "Error parsing filtered products response: " + std::string(e.what());
                    LOG_INFO("[COINBASE-REST] *** PARSE EXCEPTION *** " + last_error_);
                    LOG_INFO("[COINBASE-REST] JSON causing error: " + json.substr(0, 500) + "...");
                    return false;
                }
            }

            std::string CoinbaseRestClient::build_url(const std::string &path) const
            {
                // Remove leading slash if present
                std::string clean_path = path;
                if (!clean_path.empty() && clean_path[0] == '/')
                {
                    clean_path = clean_path.substr(1);
                }

                return base_url_ + clean_path;
            }

            // Helper methods for product type handling
            open_dtc_server::exchanges::coinbase::ProductType CoinbaseRestClient::parse_product_type(const std::string &product_id) const
            {
                // Simple heuristic for determining product type
                if (product_id.find("-PERP") != std::string::npos ||
                    product_id.find("FUTURE") != std::string::npos)
                {
                    return ProductType::FUTURE;
                }
                else if (product_id.find("-USD") != std::string::npos ||
                         product_id.find("-EUR") != std::string::npos ||
                         product_id.find("-GBP") != std::string::npos)
                {
                    return ProductType::SPOT;
                }
                return ProductType::UNKNOWN;
            }

            std::string CoinbaseRestClient::product_type_to_string(ProductType type) const
            {
                switch (type)
                {
                case ProductType::ALL:
                    return "ALL";
                case ProductType::SPOT:
                    return "SPOT";
                case ProductType::FUTURE:
                    return "FUTURE";
                case ProductType::UNKNOWN:
                    return "UNKNOWN";
                default:
                    return "UNKNOWN";
                }
            }

        } // namespace coinbase
    } // namespace exchanges
} // namespace open_dtc_server
