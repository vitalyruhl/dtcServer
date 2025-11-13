#include "coinbase_dtc_core/core/auth/jwt_auth.hpp"
#include <iostream>
#include <curl/curl.h>

// Callback function for CURL to write response data
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp)
{
    size_t total_size = size * nmemb;
    userp->append((char *)contents, total_size);
    return total_size;
}

int main()
{
    std::cout << "🔍 Detailed JWT Diagnostic Test..." << std::endl;

    try
    {
        // Load credentials - try environment variables first, then files
        auto creds = open_dtc_server::auth::CDPCredentials::from_environment();
        if (!creds.is_valid())
        {
            // Fallback to JSON file
            creds = open_dtc_server::auth::CDPCredentials::from_json_file("secrets/cdp_api_key_ECDSA.json");
        }

        if (!creds.is_valid())
        {
            std::cout << "❌ No valid credentials found!" << std::endl;
            std::cout << "   Try setting environment variables CDP_API_KEY_ID and CDP_PRIVATE_KEY" << std::endl;
            std::cout << "   Or provide secrets/cdp_api_key_ECDSA.json file" << std::endl;
            return 1;
        }

        std::cout << "\n📋 Credential Details:" << std::endl;
        std::cout << "   Key ID: " << creds.key_id << std::endl;
        std::cout << "   Private Key Length: " << creds.private_key.length() << " chars" << std::endl;
        std::cout << "   Has PEM Headers: " << (creds.private_key.find("-----BEGIN") != std::string::npos ? "Yes" : "No") << std::endl;

        // Create authenticator
        open_dtc_server::auth::JWTAuthenticator auth(creds);

        // Test the simplest endpoint with detailed JWT inspection
        std::string method = "GET";
        std::string path = "/api/v3/brokerage/key_permissions";

        std::cout << "\n🔐 JWT Token Details:" << std::endl;
        std::cout << "   Method: " << method << std::endl;
        std::cout << "   Path: " << path << std::endl;

        // Generate JWT token
        std::string jwt_token = auth.generate_token(method, path, "");

        std::cout << "\n🎫 Generated JWT Token:" << std::endl;
        std::cout << "   Length: " << jwt_token.length() << " chars" << std::endl;
        std::cout << "   Full Token: " << jwt_token << std::endl;

        // Parse JWT header and payload (simple base64 inspection)
        size_t first_dot = jwt_token.find('.');
        size_t second_dot = jwt_token.find('.', first_dot + 1);

        if (first_dot != std::string::npos && second_dot != std::string::npos)
        {
            std::string header = jwt_token.substr(0, first_dot);
            std::string payload = jwt_token.substr(first_dot + 1, second_dot - first_dot - 1);
            std::string signature = jwt_token.substr(second_dot + 1);

            std::cout << "   Header: " << header << std::endl;
            std::cout << "   Payload: " << payload << std::endl;
            std::cout << "   Signature Length: " << signature.length() << " chars" << std::endl;
        }

        // Test the request
        std::cout << "\n🌐 Making Request..." << std::endl;
        std::string url = "https://api.coinbase.com" + path;
        std::string auth_header = "Authorization: Bearer " + jwt_token;

        CURL *curl = curl_easy_init();
        if (!curl)
        {
            std::cout << "❌ Failed to initialize curl" << std::endl;
            return 1;
        }

        std::string response;
        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, auth_header.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "User-Agent: coinbase-dtc-core/1.0");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); // Enable verbose output

        std::cout << "\n📡 Request Details:" << std::endl;
        std::cout << "   URL: " << url << std::endl;
        std::cout << "   Auth Header: " << auth_header.substr(0, 50) << "..." << std::endl;

        CURLcode res = curl_easy_perform(curl);
        long status_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);

        std::cout << "\n📥 Response:" << std::endl;
        std::cout << "   Status: " << status_code << std::endl;
        std::cout << "   Body: " << response << std::endl;

        if (res != CURLE_OK)
        {
            std::cout << "❌ Request failed: " << curl_easy_strerror(res) << std::endl;
        }

        // Clean up curl resources safely
        if (headers)
        {
            curl_slist_free_all(headers);
        }
        if (curl)
        {
            curl_easy_cleanup(curl);
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}