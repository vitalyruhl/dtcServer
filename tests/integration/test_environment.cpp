#include "coinbase_dtc_core/core/auth/jwt_auth.hpp"
#include <iostream>
#include <curl/curl.h>
#include <vector>

struct TestEndpoint
{
    std::string name;
    std::string base_url;
    std::string path;
    std::string expected_permission;
};

std::pair<long, std::string> make_request(const std::string &url, const std::string &auth_token = "")
{
    CURL *curl = curl_easy_init();
    if (!curl)
        return {-1, "Failed to init curl"};

    std::string response;
    struct curl_slist *headers = nullptr;

    if (!auth_token.empty())
    {
        std::string auth_header = "Authorization: Bearer " + auth_token;
        headers = curl_slist_append(headers, auth_header.c_str());
    }
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void *contents, size_t size, size_t nmemb, std::string *userp) -> size_t
                     {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb; });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode res = curl_easy_perform(curl);
    long status_code = -1;
    if (res == CURLE_OK)
    {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return {status_code, response};
}

int main()
{
    std::cout << "🌍 Environment Detection Test..." << std::endl;

    // Test environments and endpoints
    std::vector<TestEndpoint> tests = {
        {"Production Time", "https://api.coinbase.com", "/api/v3/brokerage/time", "None"},
        {"Sandbox Time", "https://api.sandbox.coinbase.com", "/api/v3/brokerage/time", "None"},
        {"Production Products", "https://api.coinbase.com", "/api/v3/brokerage/market/products?limit=1", "None"},
        {"Sandbox Products", "https://api.sandbox.coinbase.com", "/api/v3/brokerage/market/products?limit=1", "None"},
        {"Production Portfolios", "https://api.coinbase.com", "/api/v3/brokerage/portfolios", "View"},
        {"Sandbox Portfolios", "https://api.sandbox.coinbase.com", "/api/v3/brokerage/portfolios", "View"},
        {"Production Accounts", "https://api.coinbase.com", "/api/v3/brokerage/accounts", "View"},
        {"Sandbox Accounts", "https://api.sandbox.coinbase.com", "/api/v3/brokerage/accounts", "View"}};

    try
    {
        // Load credentials for authenticated tests
        auto creds = coinbase_dtc_core::auth::CDPCredentials::from_json_file("secrets/cdp_api_key_ECDSA.json");
        if (!creds.is_valid())
        {
            std::cout << "❌ No valid credentials found!" << std::endl;
            return 1;
        }

        coinbase_dtc_core::auth::JWTAuthenticator auth(creds);

        for (const auto &test : tests)
        {
            std::cout << "\n🧪 Testing " << test.name << "..." << std::endl;
            std::cout << "   URL: " << test.base_url << test.path << std::endl;
            std::cout << "   Required Permission: " << test.expected_permission << std::endl;

            std::string jwt_token = "";
            if (test.expected_permission != "None")
            {
                jwt_token = auth.generate_token("GET", test.path, "");
            }

            std::string url = test.base_url + test.path;
            auto [status, response] = make_request(url, jwt_token);

            std::cout << "   Status: " << status;

            if (status == 200)
            {
                std::cout << " ✅ SUCCESS!" << std::endl;
                if (response.length() > 100)
                {
                    std::cout << "   Response: " << response.substr(0, 100) << "..." << std::endl;
                }
                else
                {
                    std::cout << "   Response: " << response << std::endl;
                }
            }
            else if (status == 401)
            {
                std::cout << " ❌ Unauthorized" << std::endl;
                if (test.expected_permission == "None")
                {
                    std::cout << "   ⚠️  Unexpected - public endpoint should work!" << std::endl;
                }
            }
            else if (status == 403)
            {
                std::cout << " ⚠️  Forbidden (insufficient permissions)" << std::endl;
            }
            else if (status == 404)
            {
                std::cout << " 🔍 Not Found (wrong environment?)" << std::endl;
            }
            else if (status == -1)
            {
                std::cout << " ❌ Network Error" << std::endl;
            }
            else
            {
                std::cout << " ❓ Other: " << response << std::endl;
            }
        }

        std::cout << "\n📋 Environment Analysis:" << std::endl;
        std::cout << "   - If sandbox endpoints work: Your key is for sandbox environment" << std::endl;
        std::cout << "   - If production endpoints work: Your key is for production environment" << std::endl;
        std::cout << "   - If both fail: Check API key activation or JWT format" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cout << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}