#include "coinbase_dtc_core/core/auth/cdp_credentials.hpp"
#include "coinbase_dtc_core/core/auth/jwt_auth.hpp"
#include "coinbase_dtc_core/core/util/log.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <curl/curl.h>

using namespace open_dtc_server;

// Helper callback for CURL to write response data
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

struct HttpResponse
{
    int status_code = 0;
    std::string body;
    std::string error_message;
};

HttpResponse test_endpoint(const std::string &url, const std::string &jwt_token)
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
        std::string response_body;

        // Set up CURL options
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

        // Set headers
        struct curl_slist *headers = nullptr;
        if (!jwt_token.empty())
        {
            std::string auth_header = "Authorization: Bearer " + jwt_token;
            headers = curl_slist_append(headers, auth_header.c_str());
        }
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "User-Agent: coinbase-dtc-core/1.0");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

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

int main()
{
    std::cout << "=== COINBASE ENDPOINT DIAGNOSTIC ===" << std::endl;

    try
    {
        // Load credentials
        auto credentials = auth::CDPCredentials::from_json_file("secrets/coinbase/cdp_api_key_ECDSA.json");

        if (!credentials.is_valid())
        {
            std::cerr << "ERROR: Invalid credentials" << std::endl;
            return 1;
        }

        auth::JWTAuthenticator auth(credentials);

        // Test endpoints with different paths
        std::vector<std::pair<std::string, std::string>> test_endpoints = {
            {"Time (Public)", "https://api.coinbase.com/api/v3/brokerage/time"},
            {"Accounts (Auth)", "https://api.coinbase.com/api/v3/brokerage/accounts"},
            {"Portfolios (Auth)", "https://api.coinbase.com/api/v3/brokerage/portfolios"},
            {"Products (Public)", "https://api.coinbase.com/api/v3/brokerage/market/products?limit=1"}};

        for (const auto &[name, url] : test_endpoints)
        {
            std::cout << std::endl
                      << "[TEST] Testing: " << name << std::endl;
            std::cout << "   URL: " << url << std::endl;

            // Extract path for JWT signing - only the path part, not the hostname
            std::string path = "";
            size_t pos = url.find("://");
            if (pos != std::string::npos)
            {
                size_t slash_pos = url.find("/", pos + 3); // Find first slash after "://"
                if (slash_pos != std::string::npos)
                {
                    path = url.substr(slash_pos); // Everything after domain
                }
            }

            std::string jwt_token = "";
            if (name.find("Auth") != std::string::npos)
            {
                std::cout << "   Generating JWT for path: " << path << std::endl;
                jwt_token = auth.generate_token("GET", path, "");
                std::cout << "   JWT (first 50 chars): " << jwt_token.substr(0, 50) << "..." << std::endl;
            }
            else
            {
                std::cout << "   No auth required" << std::endl;
            }

            auto response = test_endpoint(url, jwt_token);

            std::cout << "   Status: " << response.status_code;
            if (response.status_code == 200)
            {
                std::cout << " [SUCCESS]" << std::endl;
                if (response.body.length() > 100)
                {
                    std::cout << "   Response: " << response.body.substr(0, 100) << "..." << std::endl;
                }
                else
                {
                    std::cout << "   Response: " << response.body << std::endl;
                }
            }
            else if (response.status_code == 401)
            {
                std::cout << " [UNAUTHORIZED]" << std::endl;
                std::cout << "   Response: " << response.body << std::endl;
            }
            else if (response.status_code == 403)
            {
                std::cout << " [FORBIDDEN]" << std::endl;
                std::cout << "   Response: " << response.body << std::endl;
            }
            else if (response.status_code == -1)
            {
                std::cout << " [NETWORK ERROR]" << std::endl;
                std::cout << "   Error: " << response.error_message << std::endl;
            }
            else
            {
                std::cout << " [OTHER]" << std::endl;
                std::cout << "   Response: " << response.body << std::endl;
            }
        }

        std::cout << std::endl
                  << "=== DIAGNOSTIC COMPLETE ===" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}