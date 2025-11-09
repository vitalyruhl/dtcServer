#include "coinbase_dtc_core/auth/jwt_auth.hpp"
#include <iostream>
#include <curl/curl.h>

int main() {
    std::cout << "🔍 Simple API Key Test..." << std::endl;

    try {
        // Load credentials
        auto creds = coinbase_dtc_core::auth::CDPCredentials::from_json_file("secrets/cdp_api_key_ECDSA.json");
        if (!creds.is_valid()) {
            std::cout << "❌ No valid credentials found!" << std::endl;
            return 1;
        }

        std::cout << "✅ Loaded credentials for key: " << creds.key_id.substr(0, 50) << "..." << std::endl;

        coinbase_dtc_core::auth::JWTAuthenticator auth(creds);

        // Test the specific endpoint that should work with VIEW permissions
        std::string method = "GET";
        std::string path = "/api/v3/brokerage/accounts";  // This should work with VIEW
        std::string jwt_token = auth.generate_token(method, path, "");

        std::cout << "🎫 Generated JWT token (length: " << jwt_token.length() << ")" << std::endl;

        // Setup curl with timeout
        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cout << "❌ Failed to initialize curl" << std::endl;
            return 1;
        }

        std::string response;
        struct curl_slist* headers = nullptr;
        std::string auth_header = "Authorization: Bearer " + jwt_token;
        headers = curl_slist_append(headers, auth_header.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        std::string url = "https://api.coinbase.com" + path;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);  // 10 second timeout
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void* contents, size_t size, size_t nmemb, std::string* userp) -> size_t {
            userp->append((char*)contents, size * nmemb);
            return size * nmemb;
        });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

        std::cout << "🌐 Making request to: " << url << std::endl;

        CURLcode res = curl_easy_perform(curl);
        long status_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);

        std::cout << "\n📥 Response:" << std::endl;
        std::cout << "   Status: " << status_code << std::endl;
        
        if (res != CURLE_OK) {
            std::cout << "   Error: " << curl_easy_strerror(res) << std::endl;
        } else {
            std::cout << "   Body: " << response << std::endl;
            
            if (status_code == 200) {
                std::cout << "\n🎉 SUCCESS! Your API key is working!" << std::endl;
                std::cout << "The key has VIEW permissions and authentication is working correctly." << std::endl;
            } else if (status_code == 401) {
                std::cout << "\n❌ 401 Unauthorized - There might be an issue with:" << std::endl;
                std::cout << "   1. API key not fully activated yet" << std::endl;
                std::cout << "   2. Wrong environment (sandbox vs production)" << std::endl;
                std::cout << "   3. JWT format issue" << std::endl;
            } else if (status_code == 403) {
                std::cout << "\n⚠️  403 Forbidden - Permission issue" << std::endl;
                std::cout << "   Your key might need additional permissions for this endpoint" << std::endl;
            }
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}