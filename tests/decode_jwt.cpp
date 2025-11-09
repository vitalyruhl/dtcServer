#include "coinbase_dtc_core/auth/jwt_auth.hpp"
#include <iostream>
#include <jwt-cpp/jwt.h>

#ifdef HAS_NLOHMANN_JSON  
#include <jwt-cpp/traits/nlohmann-json/traits.h>
using json_traits = jwt::traits::nlohmann_json;
#else
// Use default picojson traits if nlohmann is not available
using json_traits = jwt::traits::picojson;
#endif

int main() {
    try {
        // Load credentials
        auto creds = coinbase_dtc_core::auth::CDPCredentials::from_json_file("secrets/cdp_api_key_ECDSA.json");
        if (!creds.is_valid()) {
            std::cout << "ERROR: No valid credentials found!" << std::endl;
            return 1;
        }

        coinbase_dtc_core::auth::JWTAuthenticator auth(creds);

        // Generate JWT token for accounts endpoint
        std::string method = "GET";
        std::string path = "/api/v3/brokerage/accounts";
        std::string jwt_token = auth.generate_token(method, path, "");

        std::cout << "🔍 JWT Token Analysis" << std::endl;
        std::cout << "=====================" << std::endl;
        std::cout << "\n🎫 Full Token:" << std::endl;
        std::cout << jwt_token << std::endl;
        
        try {
            // Decode without verification to inspect claims
            auto decoded_token = jwt::decode<json_traits>(jwt_token);
            
            std::cout << "\n📋 Header:" << std::endl;
            std::cout << "   Algorithm: " << decoded_token.get_algorithm() << std::endl;
            std::cout << "   Type: " << decoded_token.get_type() << std::endl;
            std::cout << "   Key ID: " << decoded_token.get_key_id() << std::endl;
            
            std::cout << "\n📋 Payload:" << std::endl;
            std::cout << "   Issuer: " << decoded_token.get_issuer() << std::endl;
            std::cout << "   Subject: " << decoded_token.get_subject() << std::endl;
            
            // Print audience set
            auto audience = decoded_token.get_audience();
            std::cout << "   Audience: [";
            for (auto it = audience.begin(); it != audience.end(); ++it) {
                if (it != audience.begin()) std::cout << ", ";
                std::cout << *it;
            }
            std::cout << "]" << std::endl;
            
            std::cout << "   Issued At: " << decoded_token.get_issued_at().time_since_epoch().count() << std::endl;
            std::cout << "   Expires At: " << decoded_token.get_expires_at().time_since_epoch().count() << std::endl;
            std::cout << "   Not Before: " << decoded_token.get_not_before().time_since_epoch().count() << std::endl;
            
            // Check for custom claims
            if (decoded_token.has_payload_claim("method")) {
                std::cout << "   Method: " << decoded_token.get_payload_claim("method").as_string() << std::endl;
            }
            if (decoded_token.has_payload_claim("uri")) {
                std::cout << "   URI: " << decoded_token.get_payload_claim("uri").as_string() << std::endl;
            }
            if (decoded_token.has_payload_claim("nonce")) {
                std::cout << "   Nonce: " << decoded_token.get_payload_claim("nonce").as_string() << std::endl;
            }
            
        } catch (const std::exception& decode_error) {
            std::cout << "\n❌ Failed to decode JWT: " << decode_error.what() << std::endl;
        }

    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}