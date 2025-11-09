#include "coinbase_dtc_core/auth/jwt_auth.hpp"
#include "coinbase_dtc_core/endpoints/endpoint.hpp"

#ifdef HAS_LIBCURL
#include <curl/curl.h>
#endif

#include <iostream>
#include <string>
#include <cstdlib>

// Simple HTTP client for testing (reuse from advanced_trade_api test)
namespace test {

struct HttpResponse {
    bool success = false;
    long status_code = 0;
    std::string body;
    std::string error;
};

class AuthenticatedHttpClient {
public:
#ifdef HAS_LIBCURL
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
    
    static HttpResponse get_with_auth(const std::string& url, const std::string& auth_token) {
        HttpResponse response;
        
        CURL* curl = curl_easy_init();
        if (!curl) {
            response.error = "Failed to initialize libcurl";
            return response;
        }
        
        // Set headers
        struct curl_slist* headers = nullptr;
        std::string auth_header = "Authorization: " + auth_token;
        headers = curl_slist_append(headers, auth_header.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        
        CURLcode res = curl_easy_perform(curl);
        
        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.status_code);
            response.success = true;
        } else {
            response.error = curl_easy_strerror(res);
        }
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        return response;
    }
#else
    static HttpResponse get_with_auth(const std::string& url, const std::string& auth_token) {
        HttpResponse response;
        response.error = "libcurl not available";
        return response;
    }
#endif

    static std::string get_client_type() {
#ifdef HAS_LIBCURL
        return "libcurl (native)";
#else
        return "not available";
#endif
    }
};

} // namespace test

int main() {
    std::cout << "🔐 Testing JWT Authentication for Coinbase CDP API..." << std::endl;
    std::cout << "   HTTP client: " << test::AuthenticatedHttpClient::get_client_type() << std::endl;
    std::cout << "" << std::endl;
    
    // Declare credentials outside try block for later use
    coinbase_dtc_core::auth::CDPCredentials env_creds;
    
    try {
        // Test 1: Load credentials from environment
        std::cout << "📋 Testing credential loading..." << std::endl;
        
        env_creds = coinbase_dtc_core::auth::CDPCredentials::from_environment();
        if (env_creds.is_valid()) {
            std::cout << "✅ Loaded credentials from environment variables" << std::endl;
            std::cout << "   Key ID: " << env_creds.key_id << std::endl;
            std::cout << "   Private key: " << (env_creds.private_key.length() > 0 ? "[PRESENT]" : "[MISSING]") << std::endl;
        } else {
            std::cout << "⚠️  No valid credentials found in environment variables" << std::endl;
            std::cout << "   Expected: CDP_API_KEY_ID and CDP_PRIVATE_KEY" << std::endl;
        }
        
        // Test 2: Try to load from JSON file  
        try {
            // Try ECDSA key first (current/recommended)
            auto file_creds = coinbase_dtc_core::auth::CDPCredentials::from_json_file("secrets/cdp_api_key_ECDSA.json");
            if (file_creds.is_valid()) {
                std::cout << "✅ Loaded ECDSA credentials from JSON file" << std::endl;
                std::cout << "   Key ID: " << file_creds.key_id << std::endl;
                std::cout << "   Private key: " << (file_creds.private_key.length() > 0 ? "[PRESENT]" : "[MISSING]") << std::endl;
                env_creds = file_creds; // Use ECDSA credentials
            }
        } catch (const std::exception& e1) {
            std::cout << "ℹ️  Could not load ECDSA key: " << e1.what() << std::endl;
            
            // Fallback to default path (legacy/Ed25519 key)
            try {
                auto file_creds = coinbase_dtc_core::auth::CDPCredentials::from_json_file("secrets/cdp_api_key.json");
                if (file_creds.is_valid()) {
                    std::cout << "⚠️  Loaded legacy credentials (may not work with Advanced Trade)" << std::endl;
                    std::cout << "   Key ID: " << file_creds.key_id << std::endl;
                    std::cout << "   Private key: " << (file_creds.private_key.length() > 0 ? "[PRESENT]" : "[MISSING]") << std::endl;
                    env_creds = file_creds;
                }
            } catch (const std::exception& e2) {
                std::cout << "ℹ️  No valid credentials found in JSON files" << std::endl;
                std::cout << "   Expected: secrets/cdp_api_key_ECDSA.json (preferred)" << std::endl;
                std::cout << "   Fallback: secrets/cdp_api_key.json (legacy)" << std::endl;
                std::cout << "   Template: secrets/cdp_api_key.json.template" << std::endl;
            }
        }
        
        // Test 3: JWT Token Generation
        if (env_creds.is_valid()) {
            std::cout << "\n🔑 Testing JWT token generation..." << std::endl;
            
            coinbase_dtc_core::auth::JWTAuthenticator authenticator(env_creds);
            
            std::string token = authenticator.generate_token("GET", "/accounts", "");
            std::cout << "✅ Generated JWT token" << std::endl;
            std::cout << "   Token length: " << token.length() << " characters" << std::endl;
            std::cout << "   Token preview: " << token.substr(0, 50) << "..." << std::endl;
            
            // Test 4: Make authenticated API request
            std::cout << "\n🌐 Testing authenticated API request..." << std::endl;
            
            using namespace coinbase_dtc_core::endpoints;
            std::string accounts_url = make_url(TRADE_BASE, "accounts");
            std::string auth_header = coinbase_dtc_core::auth::jwt_utils::make_auth_header(token);
            
            std::cout << "Making authenticated request to: " << accounts_url << std::endl;
            auto response = test::AuthenticatedHttpClient::get_with_auth(accounts_url, auth_header);
            
            if (response.success) {
                std::cout << "✅ Authenticated request successful!" << std::endl;
                std::cout << "   Status: " << response.status_code << std::endl;
                std::cout << "   Response length: " << response.body.length() << " bytes" << std::endl;
                std::cout << "   Response body: " << response.body << std::endl;
                
                if (response.status_code == 200) {
                    std::cout << "✅ Authentication working - got account data!" << std::endl;
                    std::cout << "   First 200 chars: " << response.body.substr(0, 200) << "..." << std::endl;
                } else if (response.status_code == 401) {
                    std::cout << "❌ Authentication failed - this could mean:" << std::endl;
                    std::cout << "   - New API key needs a few minutes to activate" << std::endl;
                    std::cout << "   - Wrong environment (sandbox vs production)" << std::endl;
                    std::cout << "   - Missing permissions for /accounts endpoint" << std::endl;
                    std::cout << "   - Key restrictions (IP allowlist, etc.)" << std::endl;
                } else {
                    std::cout << "⚠️  Unexpected status code: " << response.status_code << std::endl;
                }
            } else {
                std::cout << "❌ Request failed: " << response.error << std::endl;
            }
            
        } else {
            std::cout << "\n⚠️  Skipping JWT tests - no valid credentials available" << std::endl;
            std::cout << "   To test JWT authentication, provide credentials via:" << std::endl;
            std::cout << "   1. Environment variables: CDP_API_KEY_ID, CDP_PRIVATE_KEY" << std::endl;
            std::cout << "   2. JSON file: secrets/cdp_api_key.json" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "❌ JWT test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n🎯 JWT Authentication Test Summary:" << std::endl;
    std::cout << "   - Credential loading: implemented and tested" << std::endl;
    std::cout << "   - JWT token generation: " << (env_creds.is_valid() ? "working" : "needs credentials") << std::endl;
    std::cout << "   - API authentication: " << (env_creds.is_valid() ? "tested" : "needs credentials") << std::endl;
    std::cout << "   - Ready for integration with HTTP client" << std::endl;
    
    std::cout << "\n🔐 JWT Authentication test completed!" << std::endl;
    
    return 0;
}