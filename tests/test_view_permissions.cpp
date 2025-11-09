#include "coinbase_dtc_core/auth/jwt_auth.hpp"
#include <iostream>
#include <curl/curl.h>

// Helper function to make HTTP request
std::pair<long, std::string> make_request(const std::string& url, const std::string& auth_token = "") {
    CURL* curl = curl_easy_init();
    if (!curl) return {-1, "Failed to init curl"};
    
    std::string response;
    struct curl_slist* headers = nullptr;
    
    if (!auth_token.empty()) {
        std::string auth_header = "Authorization: Bearer " + auth_token;
        headers = curl_slist_append(headers, auth_header.c_str());
    }
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void* contents, size_t size, size_t nmemb, std::string* userp) -> size_t {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    
    CURLcode res = curl_easy_perform(curl);
    long status_code = -1;
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
    }
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    return {status_code, response};
}

int main() {
    std::cout << "🔍 Testing Permissions vs Public Endpoints..." << std::endl;

    // Test 1: Public endpoint (no auth needed)
    std::cout << "\n📊 Test 1: Public Market Data (no auth)" << std::endl;
    auto [status1, resp1] = make_request("https://api.coinbase.com/api/v3/brokerage/market/products");
    std::cout << "   Status: " << status1 << std::endl;
    if (status1 == 200) {
        std::cout << "   ✅ Public endpoint works!" << std::endl;
    } else {
        std::cout << "   ❌ Unexpected: " << resp1.substr(0, 100) << std::endl;
    }

    // Load credentials for authenticated tests
    try {
        auto creds = coinbase_dtc_core::auth::CDPCredentials::from_json_file("secrets/cdp_api_key_ECDSA.json");
        if (!creds.is_valid()) {
            std::cout << "❌ No valid credentials found!" << std::endl;
            return 1;
        }

        coinbase_dtc_core::auth::JWTAuthenticator auth(creds);

        // Test 2: Key permissions (requires 'view' permission)
        std::cout << "\n🔑 Test 2: Key Permissions (should work with 'view' permission)" << std::endl;
        std::string method = "GET";
        std::string path = "/api/v3/brokerage/key_permissions";
        std::string jwt_token = auth.generate_token(method, path, "");
        
        auto [status2, resp2] = make_request("https://api.coinbase.com" + path, jwt_token);
        std::cout << "   Status: " << status2 << std::endl;
        std::cout << "   Response: " << resp2 << std::endl;

        // Test 3: Accounts (requires 'view' permission)
        std::cout << "\n💼 Test 3: Accounts (should work with 'view' permission)" << std::endl;
        path = "/api/v3/brokerage/accounts";
        jwt_token = auth.generate_token(method, path, "");
        
        auto [status3, resp3] = make_request("https://api.coinbase.com" + path, jwt_token);
        std::cout << "   Status: " << status3 << std::endl;
        if (status3 == 200) {
            std::cout << "   ✅ Accounts endpoint works!" << std::endl;
        } else {
            std::cout << "   Response: " << resp3 << std::endl;
        }

        // Test 4: Products (should work with 'view')
        std::cout << "\n📈 Test 4: Products with Auth (should work with 'view')" << std::endl;
        path = "/api/v3/brokerage/market/products";
        jwt_token = auth.generate_token(method, path, "");
        
        auto [status4, resp4] = make_request("https://api.coinbase.com" + path, jwt_token);
        std::cout << "   Status: " << status4 << std::endl;
        if (status4 == 200) {
            std::cout << "   ✅ Authenticated products endpoint works!" << std::endl;
        } else {
            std::cout << "   Response: " << resp4 << std::endl;
        }

        // Test 5: Orders (requires 'trade' permission - should fail)
        std::cout << "\n🛒 Test 5: Orders (requires 'trade' permission - should fail)" << std::endl;
        path = "/api/v3/brokerage/orders/historical/batch";
        jwt_token = auth.generate_token(method, path, "");
        
        auto [status5, resp5] = make_request("https://api.coinbase.com" + path, jwt_token);
        std::cout << "   Status: " << status5 << std::endl;
        if (status5 == 403) {
            std::cout << "   ✅ Expected 403 - need 'trade' permission" << std::endl;
        } else if (status5 == 401) {
            std::cout << "   ⚠️  401 - Still authentication issue" << std::endl;
        } else {
            std::cout << "   Response: " << resp5 << std::endl;
        }

        std::cout << "\n📋 Summary:" << std::endl;
        std::cout << "   Public endpoint: " << status1 << std::endl;
        std::cout << "   Key permissions: " << status2 << std::endl;
        std::cout << "   Accounts: " << status3 << std::endl;
        std::cout << "   Products: " << status4 << std::endl;
        std::cout << "   Orders: " << status5 << std::endl;

    } catch (const std::exception& e) {
        std::cout << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}