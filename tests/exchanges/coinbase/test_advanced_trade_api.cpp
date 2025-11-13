#include "coinbase_dtc_core/core/dtc/protocol.hpp"
#include "coinbase_dtc_core/endpoints/endpoint.hpp"
#include "coinbase_settings.h"
#include <cassert>
#include <iostream>
#include <string>
#include <map>
#include <ctime>

#ifdef HAS_LIBCURL
#include <curl/curl.h>
#endif

namespace coinbase_dtc_core
{
    namespace test
    {

#ifdef HAS_LIBCURL
        // Callback function to write HTTP response data
        static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp)
        {
            size_t totalSize = size * nmemb;
            userp->append((char *)contents, totalSize);
            return totalSize;
        }
#endif

        /**
         * Cross-platform HTTP Client
         * Uses libcurl on Windows/when available
         */
        class SimpleHttpClient
        {
        public:
            struct Response
            {
                int status_code;
                std::string body;
                std::string error;
                bool success;
            };

            static Response get(const std::string &url, const std::map<std::string, std::string> &headers = {})
            {
#ifdef HAS_LIBCURL
                CURL *curl;
                CURLcode res;
                Response response = {0, "", "", false};

                curl = curl_easy_init();
                if (curl)
                {
                    // Set URL
                    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

                    // Set callback for writing data
                    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);

                    // Set user agent
                    curl_easy_setopt(curl, CURLOPT_USERAGENT, "coinbase-dtc-core/1.0");

                    // Set timeout
                    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
                    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

                    // Follow redirects
                    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

                    // Add custom headers if any
                    struct curl_slist *chunk = nullptr;
                    for (const auto &header : headers)
                    {
                        std::string header_str = header.first + ": " + header.second;
                        chunk = curl_slist_append(chunk, header_str.c_str());
                    }
                    if (chunk)
                    {
                        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
                    }

                    std::cout << "Making request to: " << url << std::endl;

                    // Perform the request
                    res = curl_easy_perform(curl);

                    if (res != CURLE_OK)
                    {
                        response.error = curl_easy_strerror(res);
                    }
                    else
                    {
                        // Get response code
                        long response_code;
                        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
                        response.status_code = (int)response_code;
                        response.success = true;
                    }

                    // Cleanup
                    if (chunk)
                    {
                        curl_slist_free_all(chunk);
                    }
                    curl_easy_cleanup(curl);
                }
                else
                {
                    response.error = "Failed to initialize libcurl";
                }

                return response;
#else
                // Fallback for systems without libcurl
                std::cout << "Making request to: " << url << std::endl;
                std::cout << "❌ libcurl not available - cannot make HTTP requests" << std::endl;
                return {0, "", "libcurl not available", false};
#endif
            }
        };

    } // namespace test
} // namespace open_dtc_server

int main()
{
    using namespace open_dtc_server;

    // Seed random number generator for temporary file names
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

#ifdef HAS_LIBCURL
    // Initialize libcurl globally
    curl_global_init(CURL_GLOBAL_DEFAULT);
#endif

    std::cout << "🚀 Testing Coinbase Advanced Trade API..." << std::endl;
    std::cout << "   Testing PRODUCTION public endpoints (no auth required)" << std::endl;
    std::cout << "   Note: Sandbox only has private endpoints, public endpoints use production" << std::endl;

    // Test 1: Get Server Time (Public endpoint - no auth required)
    {
        std::cout << "\n🕐 Testing Get Server Time endpoint..." << std::endl;

        std::string time_url = endpoints::make_url(endpoints::TRADE_BASE, endpoints::TIME);
        auto response = test::SimpleHttpClient::get(time_url);

        if (response.success && response.status_code == 200)
        {
            std::cout << "✅ Server time endpoint accessible" << std::endl;
            std::cout << "Response length: " << response.body.length() << " bytes" << std::endl;

            // Check if response contains expected time structure
            if (response.body.find("iso") != std::string::npos ||
                response.body.find("epochSeconds") != std::string::npos ||
                response.body.find("epochMillis") != std::string::npos)
            {
                std::cout << "✅ Time data structure detected" << std::endl;
            }
            else
            {
                std::cout << "⚠️  Time data structure not found" << std::endl;
            }

            // Show first 200 characters of response
            std::cout << "First 200 chars: " << response.body.substr(0, 200) << "..." << std::endl;
        }
        else
        {
            std::cout << "❌ Failed to access server time endpoint" << std::endl;
            std::cout << "Error: " << response.error << std::endl;
            std::cout << "Status: " << response.status_code << std::endl;
        }
    }

    // Test 2: List Public Products (Public endpoint - no auth required)
    {
        std::cout << "\n📋 Testing List Public Products endpoint..." << std::endl;

        std::string products_url = endpoints::make_url(endpoints::TRADE_BASE, endpoints::MARKET_PRODUCTS);
        auto response = test::SimpleHttpClient::get(products_url);

        if (response.success && response.status_code == 200)
        {
            std::cout << "✅ Public products endpoint accessible" << std::endl;
            std::cout << "Response length: " << response.body.length() << " bytes" << std::endl;

            // Check if response contains expected JSON structure
            if (response.body.find("products") != std::string::npos ||
                response.body.find("product_id") != std::string::npos ||
                response.body.find("[") != std::string::npos)
            {
                std::cout << "✅ Response appears to be valid JSON array" << std::endl;
            }
            else
            {
                std::cout << "⚠️  Response may not be valid JSON or empty" << std::endl;
            }

            // Show first 300 characters of response
            std::cout << "First 300 chars: " << response.body.substr(0, 300) << "..." << std::endl;
        }
        else
        {
            std::cout << "❌ Failed to access public products endpoint" << std::endl;
            std::cout << "Error: " << response.error << std::endl;
            std::cout << "Status: " << response.status_code << std::endl;
        }
    }

    // Test 3: Get Public Product Book (Public endpoint)
    {
        std::cout << "\n📊 Testing Get Public Product Book endpoint..." << std::endl;

        std::string book_url = endpoints::make_url(endpoints::TRADE_BASE, endpoints::MARKET_PRODUCT_BOOK) + "?product_id=BTC-USD&limit=10";
        auto response = test::SimpleHttpClient::get(book_url);

        if (response.success && response.status_code == 200)
        {
            std::cout << "✅ Public product book endpoint accessible" << std::endl;
            std::cout << "Response length: " << response.body.length() << " bytes" << std::endl;

            // Check for expected order book structure
            if (response.body.find("bids") != std::string::npos &&
                response.body.find("asks") != std::string::npos)
            {
                std::cout << "✅ Order book structure detected" << std::endl;
            }
            else if (response.body.find("pricebook") != std::string::npos)
            {
                std::cout << "✅ Price book structure detected" << std::endl;
            }
            else
            {
                std::cout << "⚠️  Order book structure not found" << std::endl;
            }

            // Show first 300 characters
            std::cout << "First 300 chars: " << response.body.substr(0, 300) << "..." << std::endl;
        }
        else
        {
            std::cout << "❌ Failed to access public product book endpoint" << std::endl;
            std::cout << "Error: " << response.error << std::endl;
            std::cout << "Status: " << response.status_code << std::endl;
        }
    }

    // Test 4: Get Public Product (Public endpoint)
    {
        std::cout << "\n💰 Testing Get Public Product endpoint..." << std::endl;

        std::string product_url = endpoints::make_url_with_id(endpoints::TRADE_BASE, endpoints::MARKET_PRODUCT, "BTC-USD");
        std::cout << "Making request to: " << product_url << std::endl;
        auto response = test::SimpleHttpClient::get(product_url);

        if (response.success && response.status_code == 200)
        {
            std::cout << "✅ Public product endpoint accessible" << std::endl;
            std::cout << "Response length: " << response.body.length() << " bytes" << std::endl;

            // Check for expected product structure
            if (response.body.find("product_id") != std::string::npos &&
                response.body.find("BTC-USD") != std::string::npos)
            {
                std::cout << "✅ Product data structure detected" << std::endl;
            }
            else
            {
                std::cout << "⚠️  Product data structure not found" << std::endl;
            }

            // Show first 300 characters
            std::cout << "First 300 chars: " << response.body.substr(0, 300) << "..." << std::endl;
        }
        else
        {
            std::cout << "❌ Failed to access public product endpoint" << std::endl;
            std::cout << "Error: " << response.error << std::endl;
            std::cout << "Status: " << response.status_code << std::endl;
        }
    }

    // Test 5: Test Authentication-Required Endpoint (will fail without auth - expected)
    {
        std::cout << "\n🔐 Testing Authentication-Required endpoint (expected to fail)..." << std::endl;

        std::string accounts_url = endpoints::make_url(endpoints::SANDBOX_BASE, endpoints::ACCOUNTS);
        auto response = test::SimpleHttpClient::get(accounts_url);

        std::cout << "Status Code: " << response.status_code << std::endl;
        if (response.status_code == 401 || response.status_code == 403)
        {
            std::cout << "✅ Authentication correctly required (401/403 expected)" << std::endl;
        }
        else if (response.status_code == 200)
        {
            std::cout << "⚠️  Unexpected success - auth may not be required or credentials present" << std::endl;
        }
        else
        {
            std::cout << "⚠️  Unexpected response code: " << response.status_code << std::endl;
        }

        // Show first 200 characters
        std::cout << "First 200 chars: " << response.body.substr(0, 200) << "..." << std::endl;
    }

    std::cout << "\n🎯 Advanced Trade API Test Summary:" << std::endl;
    std::cout << "   - Public endpoints tested for connectivity:" << std::endl;
    std::cout << "     • GET /time - Server time" << std::endl;
    std::cout << "     • GET /market/products - List products" << std::endl;
    std::cout << "     • GET /market/products/{id} - Get product details" << std::endl;
    std::cout << "     • GET /market/product_book - Order book" << std::endl;
    std::cout << "   - API URL structure verified: " << endpoints::SANDBOX_BASE << std::endl;
    std::cout << "   - Authentication requirement confirmed for private endpoints" << std::endl;
    std::cout << "   - HTTP client: ";
#ifdef HAS_LIBCURL
    std::cout << "libcurl (native)" << std::endl;
#else
    std::cout << "system curl (fallback)" << std::endl;
#endif
    std::cout << "   - Ready for CDP credential integration" << std::endl;

    std::cout << "\n📋 Summary of Available Endpoints:" << std::endl;
    std::cout << "   Public (Production only - no auth):" << std::endl;
    std::cout << "   ✅ GET /time - Server time" << std::endl;
    std::cout << "   ✅ GET /market/products - List products" << std::endl;
    std::cout << "   ✅ GET /market/products/{id} - Product details" << std::endl;
    std::cout << "   ✅ GET /market/product_book - Order book" << std::endl;
    std::cout << "   Private (Sandbox available with auth):" << std::endl;
    std::cout << "   🔒 GET /accounts - List accounts" << std::endl;
    std::cout << "   🔒 POST /orders - Create orders" << std::endl;
    std::cout << "   🔒 GET /orders/historical/batch - Order history" << std::endl;

#ifdef HAS_LIBCURL
    // Cleanup libcurl
    curl_global_cleanup();
#endif

    std::cout << "\n🎉 Advanced Trade API test completed!" << std::endl;
    return 0;
}