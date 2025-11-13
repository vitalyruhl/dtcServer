#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>

namespace coinbase_dtc_core {
namespace server {

// Symbol information structure
struct SymbolInfo {
    uint32_t symbol_id;          // DTC symbol ID
    std::string symbol;          // Universal symbol (e.g., "BTC-USD", "STRK-USDC")
    std::string display_name;    // Human-readable name
    std::string base_currency;   // Base currency (e.g., "BTC", "STRK")
    std::string quote_currency;  // Quote currency (e.g., "USD", "USDC")
    std::string exchange;        // Exchange identifier ("coinbase", "binance", etc.)
    bool active;                 // Whether symbol is actively trading
    double min_price_increment;  // Minimum price increment
    double min_size_increment;   // Minimum size increment
    
    SymbolInfo() = default;
    SymbolInfo(uint32_t id, const std::string& sym, const std::string& display, 
               const std::string& base, const std::string& quote, const std::string& exch = "coinbase")
        : symbol_id(id), symbol(sym), display_name(display)
        , base_currency(base), quote_currency(quote), exchange(exch), active(true)
        , min_price_increment(0.01), min_size_increment(0.00000001) {}
};

// Symbol subscription management
struct SymbolSubscription {
    uint32_t symbol_id;
    std::string client_id;
    bool trades_subscribed;
    bool level2_subscribed;
    uint64_t subscription_time;
    
    SymbolSubscription(uint32_t id, const std::string& client)
        : symbol_id(id), client_id(client), trades_subscribed(false)
        , level2_subscribed(false), subscription_time(0) {}
};

class SymbolManager {
public:
    SymbolManager();
    ~SymbolManager();
    
    // Symbol management
    bool add_symbol(const SymbolInfo& symbol);
    bool remove_symbol(uint32_t symbol_id);
    void initialize_default_symbols();
    
    // Symbol lookup
    std::shared_ptr<SymbolInfo> get_symbol_by_id(uint32_t symbol_id) const;
    std::shared_ptr<SymbolInfo> get_symbol_by_name(const std::string& symbol_name) const;
    std::shared_ptr<SymbolInfo> get_symbol_by_exchange_symbol(const std::string& exchange_symbol, const std::string& exchange = "coinbase") const;
    std::vector<std::shared_ptr<SymbolInfo>> get_all_symbols() const;
    std::vector<std::shared_ptr<SymbolInfo>> get_active_symbols() const;
    
    // Symbol ID management
    uint32_t get_next_symbol_id();
    bool is_valid_symbol_id(uint32_t symbol_id) const;
    
    // Subscription management
    bool subscribe_symbol(const std::string& client_id, uint32_t symbol_id, bool trades = true, bool level2 = true);
    bool unsubscribe_symbol(const std::string& client_id, uint32_t symbol_id);
    bool unsubscribe_all(const std::string& client_id);
    
    // Get subscriptions
    std::vector<uint32_t> get_client_subscriptions(const std::string& client_id) const;
    std::vector<std::string> get_symbol_subscribers(uint32_t symbol_id) const;
    bool is_client_subscribed(const std::string& client_id, uint32_t symbol_id) const;
    
    // Statistics and status
    size_t get_symbol_count() const;
    size_t get_active_symbol_count() const;
    size_t get_subscription_count() const;
    std::string get_status() const;
    
    // Symbol validation
    bool validate_symbol_request(const std::string& symbol_name) const;
    uint32_t resolve_symbol_name(const std::string& symbol_name) const;
    
private:
    mutable std::mutex symbols_mutex_;
    mutable std::mutex subscriptions_mutex_;
    
    // Symbol storage
    std::unordered_map<uint32_t, std::shared_ptr<SymbolInfo>> symbols_by_id_;
    std::unordered_map<std::string, std::shared_ptr<SymbolInfo>> symbols_by_name_;
    
    // Subscription tracking
    std::unordered_map<std::string, std::vector<SymbolSubscription>> client_subscriptions_;
    std::unordered_map<uint32_t, std::vector<std::string>> symbol_subscribers_;
    
    // ID generation
    uint32_t next_symbol_id_;
    
    // Helper methods
    void add_symbol_internal(std::shared_ptr<SymbolInfo> symbol);
    void remove_symbol_internal(uint32_t symbol_id);
    std::string generate_client_key(const std::string& client_id, uint32_t symbol_id) const;
};

} // namespace server
} // namespace coinbase_dtc_core