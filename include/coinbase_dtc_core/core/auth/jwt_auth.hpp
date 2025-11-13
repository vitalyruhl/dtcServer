#pragma once

#ifndef COINBASE_DTC_JWT_AUTH_HPP
#define COINBASE_DTC_JWT_AUTH_HPP

#include <string>
#include <chrono>

namespace open_dtc_server
{
    namespace auth
    {

        /**
         * @brief CDP (Coinbase Developer Platform) credentials structure
         */
        struct CDPCredentials
        {
            std::string key_id;      // CDP API Key ID (from "id" field)
            std::string private_key; // Base64-encoded private key (from "privateKey" field)
            std::string passphrase;  // Optional passphrase for private key

            // Load from JSON file
            static CDPCredentials from_json_file(const std::string &filepath);

            // Load from environment variables
            static CDPCredentials from_environment();

            // Validate credentials
            bool is_valid() const;
        };

        /**
         * @brief JWT token generator for Coinbase Advanced Trade API
         *
         * Generates JWT tokens using ES256 algorithm (ECDSA) for authentication
         * with Coinbase Developer Platform (CDP) credentials.
         * NOTE: Advanced Trade API requires ECDSA keys, NOT Ed25519!
         */
        class JWTAuthenticator
        {
        public:
            /**
             * @brief Construct authenticator with CDP credentials
             * @param credentials CDP credentials (key name and private key)
             */
            explicit JWTAuthenticator(const CDPCredentials &credentials);

            /**
             * @brief Generate JWT token for API request
             * @param method HTTP method (GET, POST, etc.)
             * @param path API path (e.g., "/accounts")
             * @param body Request body (empty for GET requests)
             * @return JWT token string
             */
            std::string generate_token(
                const std::string &method,
                const std::string &path,
                const std::string &body = "");

            /**
             * @brief Check if token needs refresh
             * @return true if current token is expired or near expiry
             */
            bool needs_refresh() const;

            /**
             * @brief Get current valid token (generates new if needed)
             * @param method HTTP method
             * @param path API path
             * @param body Request body
             * @return Current valid JWT token
             */
            std::string get_current_token(
                const std::string &method,
                const std::string &path,
                const std::string &body = "");

        private:
            CDPCredentials credentials_;
            std::string current_token_;
            std::chrono::system_clock::time_point token_expiry_;

            // Token validity duration (default: 2 minutes)
            static constexpr std::chrono::seconds TOKEN_LIFETIME{120};

            // Refresh buffer (refresh when 30 seconds left)
            static constexpr std::chrono::seconds REFRESH_BUFFER{30};
        };

        /**
         * @brief Utility functions for JWT authentication
         */
        namespace jwt_utils
        {
            /**
             * @brief Create authorization header value
             * @param token JWT token
             * @return "Bearer <token>" string for Authorization header
             */
            std::string make_auth_header(const std::string &token);

            /**
             * @brief Generate nonce for JWT claims
             * @return Random nonce string
             */
            std::string generate_nonce();

            /**
             * @brief Get current Unix timestamp
             * @return Current time as Unix timestamp
             */
            int64_t current_timestamp();

            /**
             * @brief Convert base64-encoded private key to PEM format
             * @param base64_key Base64-encoded EC private key from CDP
             * @return PEM-formatted private key string
             */
            std::string base64_to_pem_private_key(const std::string &base64_key);

            /**
             * @brief Convert CDP base64 private key to Ed25519 PEM format
             * @param base64_key Base64-encoded Ed25519 private key from CDP
             * @return Ed25519 PEM-formatted private key string
             */
            std::string base64_to_ed25519_pem(const std::string &base64_key);

            /**
             * @brief Decode base64 string to raw bytes
             * @param encoded Base64-encoded string
             * @return Decoded raw bytes as string
             */
            std::string base64_decode(const std::string &encoded);
        }

    } // namespace auth
} // namespace open_dtc_server

#endif // COINBASE_DTC_JWT_AUTH_HPP