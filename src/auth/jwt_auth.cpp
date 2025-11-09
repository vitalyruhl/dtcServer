#include "coinbase_dtc_core/auth/jwt_auth.hpp"

#include <iostream>  // For std::cout debug output
#include <algorithm> // For std::min

#ifdef HAS_JWT_CPP
#include <jwt-cpp/jwt.h>
// Include the correct traits for nlohmann json
#ifdef HAS_NLOHMANN_JSON  
#include <jwt-cpp/traits/nlohmann-json/traits.h>
using json_traits = jwt::traits::nlohmann_json;
#else
// Use default picojson traits if nlohmann is not available
using json_traits = jwt::traits::picojson;
#endif
#endif

#ifdef HAS_NLOHMANN_JSON
#include <nlohmann/json.hpp>
#endif

#include <fstream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <cstdlib>

namespace coinbase_dtc_core {
namespace auth {

// CDPCredentials implementation
CDPCredentials CDPCredentials::from_json_file(const std::string& filepath) {
#ifdef HAS_NLOHMANN_JSON
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open credentials file: " + filepath);
    }
    
    nlohmann::json j;
    file >> j;
    
    CDPCredentials creds;
    creds.key_id = j.value("name", "");  // ECDSA keys use "name" field
    if (creds.key_id.empty()) {
        creds.key_id = j.value("id", "");  // Fallback to "id" for Ed25519 keys
    }
    creds.private_key = j.value("privateKey", "");
    creds.passphrase = j.value("passphrase", "");
    
    return creds;
#else
    throw std::runtime_error("JSON support not available - cannot load credentials from file");
#endif
}

CDPCredentials CDPCredentials::from_environment() {
    CDPCredentials creds;
    
    const char* key_id = std::getenv("CDP_API_KEY_ID");
    const char* private_key = std::getenv("CDP_PRIVATE_KEY");
    const char* passphrase = std::getenv("CDP_PASSPHRASE");
    
    if (key_id) creds.key_id = key_id;
    if (private_key) creds.private_key = private_key;
    if (passphrase) creds.passphrase = passphrase;
    
    return creds;
}

bool CDPCredentials::is_valid() const {
    return !key_id.empty() && !private_key.empty();
}

// JWTAuthenticator implementation
JWTAuthenticator::JWTAuthenticator(const CDPCredentials& credentials)
    : credentials_(credentials) 
{
    if (!credentials_.is_valid()) {
        throw std::invalid_argument("Invalid CDP credentials provided");
    }
}

std::string JWTAuthenticator::generate_token(
    const std::string& method,
    const std::string& path,
    const std::string& body
) {
#ifdef HAS_JWT_CPP
    auto now = std::chrono::system_clock::now();
    auto exp = now + TOKEN_LIFETIME;
    
    // Create JWT token with ES256 algorithm (ECDSA - required for Coinbase Advanced Trade)
    // Format: Coinbase updated format as of April 2024 - removed audience claim
    std::string uri = method + " api.coinbase.com" + path;  // Full URI format
    
    auto token = jwt::create<json_traits>()
        .set_algorithm("ES256")  // ES256 required for Advanced Trade API
        .set_type("JWT")
        .set_key_id(credentials_.key_id)
        .set_header_claim("nonce", jwt_utils::generate_nonce())  // Nonce in header
        .set_issuer("cdp")           // Required: CDP issuer
        .set_subject(credentials_.key_id)  // Required: API key ID
        .set_not_before(now)         // Required: Not before timestamp
        .set_expires_at(exp)         // Required: Expiration (2 minutes)
        .set_payload_claim("uri", uri);  // Required: Full URI format
    
    // Sign with ES256 (ECDSA) private key - required for Advanced Trade API
    std::string signing_key = credentials_.private_key;
    
    // Debug: Show what we're working with
    std::cout << "Debug: Using ES256 (ECDSA) algorithm for Advanced Trade API" << std::endl;
    std::cout << "Debug: Key ID: " << credentials_.key_id << std::endl;
    std::cout << "Debug: Private key length: " << signing_key.length() << " chars" << std::endl;
    
    // Check if key is already in PEM format (ECDSA keys from CDP)
    if (signing_key.find("-----BEGIN EC PRIVATE KEY-----") != std::string::npos) {
        std::cout << "Debug: Detected ECDSA PEM format key" << std::endl;
        try {
            auto signed_token = token.sign(jwt::algorithm::es256("", signing_key, "", ""));
            
            std::cout << "✅ Successfully signed JWT token with ES256 (ECDSA PEM)!" << std::endl;
            
            // Update cache
            current_token_ = signed_token;
            token_expiry_ = exp;
            
            return signed_token;
        } catch (const std::exception& e) {
            std::cout << "❌ ECDSA PEM signing failed: " << e.what() << std::endl;
            throw std::runtime_error("Failed to sign JWT with ECDSA PEM key: " + std::string(e.what()));
        }
    }
    
    // Try other formats for backwards compatibility
    try {
        // Try 1: Direct base64 string (for older key formats)
        std::cout << "Debug: Trying direct base64 format for ECDSA..." << std::endl;
        auto signed_token = token.sign(jwt::algorithm::es256("", signing_key, "", ""));
        
        std::cout << "✅ Successfully signed JWT token with ES256!" << std::endl;
        
        // Update cache
        current_token_ = signed_token;
        token_expiry_ = exp;
        
        return signed_token;
    } catch (const std::exception& e1) {
        std::cout << "Debug: Direct base64 failed: " << e1.what() << std::endl;
        
        try {
            // Try 2: Convert to PEM format for ECDSA
            std::cout << "Debug: Trying PEM format conversion..." << std::endl;
            std::string pem_key = jwt_utils::base64_to_ed25519_pem(signing_key); // Reuse the PEM function
            
            auto signed_token = token.sign(jwt::algorithm::es256("", pem_key, "", ""));
            
            std::cout << "✅ Successfully signed JWT token with ES256 (converted PEM)!" << std::endl;
            
            // Update cache
            current_token_ = signed_token;
            token_expiry_ = exp;
            
            return signed_token;
        } catch (const std::exception& e2) {
            std::cout << "Debug: PEM conversion failed: " << e2.what() << std::endl;
            throw std::runtime_error("Failed to sign JWT with ES256. Make sure you have an ECDSA key (not Ed25519). Error: " + std::string(e2.what()));
        }
    }
#else
    throw std::runtime_error("JWT support not available - cannot generate token");
#endif
}

bool JWTAuthenticator::needs_refresh() const {
    auto now = std::chrono::system_clock::now();
    return current_token_.empty() || 
           (now + REFRESH_BUFFER) >= token_expiry_;
}

std::string JWTAuthenticator::get_current_token(
    const std::string& method,
    const std::string& path,
    const std::string& body
) {
    if (needs_refresh()) {
        return generate_token(method, path, body);
    }
    return current_token_;
}

// JWT utilities implementation
namespace jwt_utils {

std::string make_auth_header(const std::string& token) {
    return "Bearer " + token;
}

std::string generate_nonce() {
    // Generate a random nonce
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    
    std::stringstream ss;
    ss << std::hex << dis(gen);
    return ss.str();
}

int64_t current_timestamp() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

// Convert CDP base64 private key to Ed25519 PEM format
std::string base64_to_ed25519_pem(const std::string& base64_key) {
    // CDP provides Ed25519 private keys as base64-encoded raw key material
    // We need to wrap it in proper Ed25519 PEM headers
    std::string pem_key = "-----BEGIN PRIVATE KEY-----\n";
    
    // Insert line breaks every 64 characters as per PEM standard
    for (size_t i = 0; i < base64_key.length(); i += 64) {
        size_t remaining = base64_key.length() - i;
        size_t chunk_size = (remaining < 64) ? remaining : 64;
        pem_key += base64_key.substr(i, chunk_size);
        if (i + chunk_size < base64_key.length()) {
            pem_key += "\n";
        }
    }
    
    pem_key += "\n-----END PRIVATE KEY-----\n";
    return pem_key;
}

// Simple base64 decoder for Ed25519 keys
std::string base64_decode(const std::string& encoded) {
    // This is a simple base64 decoder - for production, use a proper library
    static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string decoded;
    int val = 0, valb = -8;
    
    for (unsigned char c : encoded) {
        if (chars.find(c) == std::string::npos) break;
        val = (val << 6) + chars.find(c);
        valb += 6;
        if (valb >= 0) {
            decoded.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return decoded;
}

} // namespace jwt_utils

} // namespace auth
} // namespace coinbase_dtc_core