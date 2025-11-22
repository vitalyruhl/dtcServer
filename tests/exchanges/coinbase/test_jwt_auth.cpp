#include "coinbase_dtc_core/core/auth/jwt_auth.hpp"
#include "coinbase_dtc_core/exchanges/coinbase/endpoint.hpp"

#ifdef HAS_LIBCURL
#include <curl/curl.h>
#endif

#include <cstdlib>
#include <iostream>
#include <string>

namespace test
{
    struct HttpResponse
    {
        bool success = false;
        long status_code = 0;
        std::string body;
        std::string error;
    };

    class AuthenticatedHttpClient
    {
    public:
#ifdef HAS_LIBCURL
        static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp)
        {
            userp->append(static_cast<char *>(contents), size * nmemb);
            return size * nmemb;
        }

        static HttpResponse get_with_auth(const std::string &url, const std::string &auth_token)
        {
            HttpResponse response;

            CURL *curl = curl_easy_init();
            if (!curl)
            {
                response.error = "Failed to initialize libcurl";
                return response;
            }

            curl_slist *headers = nullptr;
            std::string auth_header = "Authorization: " + auth_token;
            headers = curl_slist_append(headers, auth_header.c_str());
            headers = curl_slist_append(headers, "Content-Type: application/json");

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

            const CURLcode res = curl_easy_perform(curl);

            if (res == CURLE_OK)
            {
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.status_code);
                response.success = true;
            }
            else
            {
                response.error = curl_easy_strerror(res);
            }

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            return response;
        }
#else
        static HttpResponse get_with_auth(const std::string &, const std::string &)
        {
            HttpResponse response;
            response.error = "libcurl not available";
            return response;
        }
#endif

        static std::string get_client_type()
        {
#ifdef HAS_LIBCURL
            return "libcurl (native)";
#else
            return "not available";
#endif
        }
    };

} // namespace test

int main()
{
    std::cout << "[INFO] Testing JWT Authentication for Coinbase CDP API..." << std::endl;
    std::cout << "[INFO] HTTP client: " << test::AuthenticatedHttpClient::get_client_type() << std::endl;

    open_dtc_server::auth::CDPCredentials env_creds;

    try
    {
        std::cout << "[INFO] Testing credential loading..." << std::endl;

        env_creds = open_dtc_server::auth::CDPCredentials::from_environment();
        if (env_creds.is_valid())
        {
            std::cout << "[SUCCESS] Loaded credentials from environment variables" << std::endl;
            std::cout << "   Key ID: " << env_creds.key_id << std::endl;
            std::cout << "   Private key: " << (env_creds.private_key.empty() ? "[MISSING]" : "[PRESENT]") << std::endl;
        }
        else
        {
            std::cout << "[WARNING] No valid credentials found in environment variables" << std::endl;
            std::cout << "   Expected: CDP_API_KEY_ID and CDP_PRIVATE_KEY" << std::endl;
        }

        try
        {
            auto file_creds = open_dtc_server::auth::CDPCredentials::from_json_file("secrets/cdp_api_key_ECDSA.json");
            if (file_creds.is_valid())
            {
                std::cout << "[SUCCESS] Loaded ECDSA credentials from JSON file" << std::endl;
                std::cout << "   Key ID: " << file_creds.key_id << std::endl;
                std::cout << "   Private key: " << (file_creds.private_key.empty() ? "[MISSING]" : "[PRESENT]") << std::endl;
                env_creds = file_creds;
            }
        }
        catch (const std::exception &e1)
        {
            std::cout << "[INFO] Could not load ECDSA key: " << e1.what() << std::endl;

            try
            {
                auto file_creds = open_dtc_server::auth::CDPCredentials::from_json_file("secrets/cdp_api_key.json");
                if (file_creds.is_valid())
                {
                    std::cout << "[WARNING] Loaded legacy credentials (may not work with Advanced Trade)" << std::endl;
                    std::cout << "   Key ID: " << file_creds.key_id << std::endl;
                    std::cout << "   Private key: " << (file_creds.private_key.empty() ? "[MISSING]" : "[PRESENT]") << std::endl;
                    env_creds = file_creds;
                }
            }
            catch (const std::exception &e2)
            {
                std::cout << "[INFO] No valid credentials found in JSON files" << std::endl;
                std::cout << "   Expected: secrets/cdp_api_key_ECDSA.json (preferred)" << std::endl;
                std::cout << "   Fallback: secrets/cdp_api_key.json (legacy)" << std::endl;
                std::cout << "   Template: secrets/cdp_api_key.json.template" << std::endl;
                std::cout << "   Details: " << e2.what() << std::endl;
            }
        }

        if (env_creds.is_valid())
        {
            std::cout << std::endl;
            std::cout << "[INFO] Testing JWT token generation..." << std::endl;

            open_dtc_server::auth::JWTAuthenticator authenticator(env_creds);
            const std::string token = authenticator.generate_token("GET", "/accounts", "");

            std::cout << "[SUCCESS] Generated JWT token" << std::endl;
            std::cout << "   Token length: " << token.length() << " characters" << std::endl;
            std::cout << "   Token preview: " << token.substr(0, 50) << "..." << std::endl;

            std::cout << std::endl;
            std::cout << "[INFO] Testing authenticated API request..." << std::endl;

            using namespace open_dtc_server::endpoints;
            const std::string accounts_url = make_url(TRADE_BASE, "accounts");
            const std::string auth_header = open_dtc_server::auth::jwt_utils::make_auth_header(token);

            std::cout << "[INFO] Making authenticated request to: " << accounts_url << std::endl;
            const auto response = test::AuthenticatedHttpClient::get_with_auth(accounts_url, auth_header);

            if (response.success)
            {
                std::cout << "[SUCCESS] Authenticated request completed" << std::endl;
                std::cout << "   Status: " << response.status_code << std::endl;
                std::cout << "   Response length: " << response.body.length() << " bytes" << std::endl;

                if (response.status_code == 200)
                {
                    std::cout << "[SUCCESS] Authentication working - received account data" << std::endl;
                    std::cout << "   Preview: " << response.body.substr(0, 200) << "..." << std::endl;
                }
                else if (response.status_code == 401)
                {
                    std::cout << "[ERROR] Authentication failed (401)" << std::endl;
                    std::cout << "   Possible causes:" << std::endl;
                    std::cout << "   - Newly created API key not yet active" << std::endl;
                    std::cout << "   - Environment mismatch (sandbox vs production)" << std::endl;
                    std::cout << "   - Missing permissions for /accounts" << std::endl;
                    std::cout << "   - Key restrictions such as IP allowlists" << std::endl;
                }
                else
                {
                    std::cout << "[WARNING] Unexpected status code: " << response.status_code << std::endl;
                }
            }
            else
            {
                std::cout << "[ERROR] Request failed: " << response.error << std::endl;
            }
        }
        else
        {
            std::cout << std::endl;
            std::cout << "[WARNING] Skipping JWT token and HTTP tests - no valid credentials" << std::endl;
            std::cout << "   Provide credentials via environment variables or secrets/cdp_api_key_ECDSA.json" << std::endl;
        }
    }
    catch (const std::exception &ex)
    {
        std::cout << "[ERROR] JWT test failed with exception: " << ex.what() << std::endl;
        return 1;
    }

    std::cout << std::endl;
    std::cout << "[INFO] JWT Authentication Test Summary" << std::endl;
    std::cout << "   - Credential loading: completed" << std::endl;
    std::cout << "   - JWT token generation: " << (env_creds.is_valid() ? "executed" : "skipped (missing credentials)") << std::endl;
    std::cout << "   - API authentication: " << (env_creds.is_valid() ? "executed" : "skipped") << std::endl;

    std::cout << std::endl;
    std::cout << "[SUCCESS] JWT authentication test routine finished" << std::endl;

    return 0;
}
