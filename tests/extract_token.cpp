#include "coinbase_dtc_core/auth/jwt_auth.hpp"
#include <iostream>

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

        // Output ONLY the token for easy extraction
        std::cout << jwt_token << std::endl;

    } catch (const std::exception& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}