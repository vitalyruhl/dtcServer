#include "coinbase_dtc_core/auth/jwt_auth.hpp"
#include <iostream>
#include <vector>
#include <tuple>
#include <string>
#include <curl/curl.h>

// Simple HTTP client for testing different endpoints
class SimpleHttpClient {
public:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    static void test_endpoint(const std::string& endpoint, const std::string& auth_header, const std::string& description) {
        std::cout << "\n🔍 Testing: " << description << std::endl;
        std::cout << "   Endpoint: " << endpoint << std::endl;

        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cout << "❌ Failed to initialize curl" << std::endl;
            return;
        }

        std::string response;
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, auth_header.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

        CURLcode res = curl_easy_perform(curl);
        long status_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);

        if (res == CURLE_OK) {
            std::cout << "   Status: " << status_code;
            if (status_code == 200) {
                std::cout << " ✅ SUCCESS!" << std::endl;
                std::cout << "   Response: " << response.substr(0, 100) << "..." << std::endl;
            } else if (status_code == 401) {
                std::cout << " ❌ UNAUTHORIZED" << std::endl;
                std::cout << "   Response: " << response << std::endl;
            } else if (status_code == 403) {
                std::cout << " 🚫 FORBIDDEN - Missing permissions" << std::endl;
                std::cout << "   Response: " << response << std::endl;
            } else {
                std::cout << " ⚠️  UNEXPECTED" << std::endl;
                std::cout << "   Response: " << response << std::endl;
            }
        } else {
            std::cout << "❌ Request failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
};

int main() {
    std::cout << "🔐 Testing API Key Permissions..." << std::endl;

    try {
        // Load credentials - try environment variables first, then files
        auto creds = coinbase_dtc_core::auth::CDPCredentials::from_environment();
        if (!creds.is_valid()) {
            // Fallback to JSON file
            creds = coinbase_dtc_core::auth::CDPCredentials::from_json_file("secrets/cdp_api_key_ECDSA.json");
        }
        
        if (!creds.is_valid()) {
            std::cout << "❌ No valid credentials found!" << std::endl;
            std::cout << "   Try setting environment variables CDP_API_KEY_ID and CDP_PRIVATE_KEY" << std::endl;
            std::cout << "   Or provide secrets/cdp_api_key_ECDSA.json file" << std::endl;
            return 1;
        }

        std::cout << "✅ Loaded credentials: " << creds.key_id << std::endl;

        // Create authenticator
        coinbase_dtc_core::auth::JWTAuthenticator auth(creds);

        // Test different endpoints with different permission requirements
        std::vector<std::tuple<std::string, std::string, std::string>> test_endpoints = {
            // Public endpoint (no auth needed - baseline test)
            {"https://api.coinbase.com/api/v3/brokerage/time", "", "Server Time (Public)"},
            
            // Basic authenticated endpoints
            {"https://api.coinbase.com/api/v3/brokerage/key_permissions", "", "Key Permissions (View)"},
            {"https://api.coinbase.com/api/v3/brokerage/accounts", "", "List Accounts (View)"},
            {"https://api.coinbase.com/api/v3/brokerage/products", "", "List Products (View)"},
            
            // Market data endpoints
            {"https://api.coinbase.com/api/v3/brokerage/products/BTC-USD", "", "Get Product BTC-USD (View)"},
            {"https://api.coinbase.com/api/v3/brokerage/best_bid_ask?product_ids=BTC-USD", "", "Best Bid/Ask (View)"},
        };

        for (auto& [url, method, desc] : test_endpoints) {
            if (url.find("time") != std::string::npos) {
                // Test public endpoint without auth
                SimpleHttpClient::test_endpoint(url, "", desc);
            } else {
                // Test authenticated endpoints
                try {
                    std::string jwt_token = auth.generate_token("GET", url.substr(url.find("/api")), "");
                    std::string auth_header = "Authorization: Bearer " + jwt_token;
                    SimpleHttpClient::test_endpoint(url, auth_header, desc);
                } catch (const std::exception& e) {
                    std::cout << "\n❌ Failed to generate JWT for " << desc << ": " << e.what() << std::endl;
                }
            }
        }

        std::cout << "\n📊 Test Summary:" << std::endl;
        std::cout << "   - If all endpoints return 401: Key is not active yet" << std::endl;
        std::cout << "   - If some return 403: Missing specific permissions" << std::endl;
        std::cout << "   - If key_permissions works: Key is active, check individual permissions" << std::endl;
        std::cout << "   - If nothing works: Wait 10-15 minutes and try again" << std::endl;

    } catch (const std::exception& e) {
        std::cout << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}