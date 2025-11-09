#pragma once

#ifndef CDP_CREDENTIALS_H
#define CDP_CREDENTIALS_H

#include <string>
#include <fstream>
#include <iostream>

namespace coinbase_dtc_core {
namespace credentials {

/**
 * Coinbase Developer Platform (CDP) Credentials Handler
 * 
 * Handles the new CDP API authentication format which uses:
 * - API Key ID (public identifier)
 * - Private Key (for request signing)
 * - JWT-based authentication
 */

struct CDPCredentials {
    std::string api_key_id;
    std::string private_key;    // Base64 encoded
    bool valid = false;
    
    bool isValid() const {
        return valid && !api_key_id.empty() && !private_key.empty();
    }
};

class CDPCredentialsManager {
public:
    // Load credentials from JSON file
    static CDPCredentials loadFromFile(const std::string& file_path) {
        CDPCredentials creds;
        
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "Warning: Could not open CDP API key file: " << file_path << std::endl;
            return creds;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        // Simple JSON parsing for the CDP format
        // Format: {"id": "...", "privateKey": "..."}
        creds = parseSimpleJson(content);
        
        if (creds.isValid()) {
            std::cout << "✅ CDP credentials loaded successfully" << std::endl;
            std::cout << "API Key ID: " << creds.api_key_id.substr(0, 8) << "..." << std::endl;
        } else {
            std::cerr << "❌ Failed to parse CDP credentials from: " << file_path << std::endl;
        }
        
        return creds;
    }
    
    // Load from environment variables
    static CDPCredentials loadFromEnvironment() {
        CDPCredentials creds;
        
        const char* env_id = std::getenv("CDP_API_KEY_ID");
        const char* env_key = std::getenv("CDP_PRIVATE_KEY");
        
        if (env_id && env_key) {
            creds.api_key_id = env_id;
            creds.private_key = env_key;
            creds.valid = true;
            std::cout << "✅ CDP credentials loaded from environment" << std::endl;
        }
        
        return creds;
    }
    
    // Get credentials from multiple sources (priority: env vars, then file)
    static CDPCredentials getCredentials() {
        // Try environment variables first
        auto env_creds = loadFromEnvironment();
        if (env_creds.isValid()) {
            return env_creds;
        }
        
        // Try loading from default file location
        auto file_creds = loadFromFile("secrets/cdp_api_key.json");
        if (file_creds.isValid()) {
            return file_creds;
        }
        
        // Try alternative file locations
        for (const auto& path : {"./cdp_api_key.json", "../secrets/cdp_api_key.json"}) {
            auto alt_creds = loadFromFile(path);
            if (alt_creds.isValid()) {
                return alt_creds;
            }
        }
        
        std::cout << "ℹ️  No CDP credentials found - using public API mode" << std::endl;
        return CDPCredentials{};  // Return invalid credentials
    }

private:
    // Simple JSON parser for the specific CDP format
    static CDPCredentials parseSimpleJson(const std::string& json) {
        CDPCredentials creds;
        
        try {
            // Find "id" field
            size_t id_pos = json.find("\"id\"");
            if (id_pos != std::string::npos) {
                size_t id_start = json.find("\"", id_pos + 4);
                if (id_start != std::string::npos) {
                    id_start++; // Skip opening quote
                    size_t id_end = json.find("\"", id_start);
                    if (id_end != std::string::npos) {
                        creds.api_key_id = json.substr(id_start, id_end - id_start);
                    }
                }
            }
            
            // Find "privateKey" field
            size_t key_pos = json.find("\"privateKey\"");
            if (key_pos != std::string::npos) {
                size_t key_start = json.find("\"", key_pos + 12);
                if (key_start != std::string::npos) {
                    key_start++; // Skip opening quote
                    size_t key_end = json.find("\"", key_start);
                    if (key_end != std::string::npos) {
                        creds.private_key = json.substr(key_start, key_end - key_start);
                    }
                }
            }
            
            // Validate that we got both fields
            creds.valid = !creds.api_key_id.empty() && !creds.private_key.empty();
            
        } catch (const std::exception& e) {
            std::cerr << "Error parsing CDP JSON: " << e.what() << std::endl;
            creds.valid = false;
        }
        
        return creds;
    }
};

} // namespace credentials
} // namespace coinbase_dtc_core

#endif // CDP_CREDENTIALS_H