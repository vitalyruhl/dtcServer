#include "coinbase_dtc_core/server/symbol_manager.hpp"
#include "coinbase_dtc_core/util/log.hpp"
#include <sstream>
#include <algorithm>
#include <chrono>

namespace coinbase_dtc_core {
namespace server {

SymbolManager::SymbolManager() : next_symbol_id_(1) {
    initialize_default_symbols();
}

SymbolManager::~SymbolManager() {
    // Cleanup handled by smart pointers
}

bool SymbolManager::add_symbol(const SymbolInfo& symbol) {
    std::lock_guard<std::mutex> lock(symbols_mutex_);
    
    // Check if symbol ID already exists
    if (symbols_by_id_.find(symbol.symbol_id) != symbols_by_id_.end()) {
        util::log("[SYMBOL] Symbol ID " + std::to_string(symbol.symbol_id) + " already exists");
        return false;
    }
    
    // Check if Coinbase symbol already exists
    if (symbols_by_name_.find(symbol.symbol) != symbols_by_name_.end()) {
        util::log("[SYMBOL] Symbol " + symbol.symbol + " already exists");
        return false;
    }
    
    auto symbol_ptr = std::make_shared<SymbolInfo>(symbol);
    add_symbol_internal(symbol_ptr);
    
    util::log("[SYMBOL] Added symbol: " + symbol.display_name + " (" + symbol.symbol + 
              ") with ID " + std::to_string(symbol.symbol_id));
    return true;
}

bool SymbolManager::remove_symbol(uint32_t symbol_id) {
    std::lock_guard<std::mutex> lock(symbols_mutex_);
    
    auto it = symbols_by_id_.find(symbol_id);
    if (it == symbols_by_id_.end()) {
        return false;
    }
    
    auto symbol = it->second;
    remove_symbol_internal(symbol_id);
    
    util::log("[SYMBOL] Removed symbol: " + symbol->display_name + " (ID: " + std::to_string(symbol_id) + ")");
    return true;
}

void SymbolManager::initialize_default_symbols() {
    std::lock_guard<std::mutex> lock(symbols_mutex_);
    
    // Add major cryptocurrency pairs with STRK-USDC first as test coin
    std::vector<SymbolInfo> default_symbols = {
        SymbolInfo(1, "STRK-USDC", "Starknet/USDC", "STRK", "USDC", "coinbase"),
        SymbolInfo(2, "USDC-EUR", "USDC/EUR", "USDC", "EUR", "coinbase"),
        SymbolInfo(3, "SOL-USDC", "Solana/USDC", "SOL", "USDC", "coinbase"),
        SymbolInfo(4, "BTC-USDC", "Bitcoin/USDC", "BTC", "USDC", "coinbase"),
        SymbolInfo(5, "ETH-USDC", "Ethereum/USDC", "ETH", "USDC", "coinbase"),
        SymbolInfo(6, "LTC-USDC", "Litecoin/USDC", "LTC", "USDC", "coinbase"),
        SymbolInfo(7, "LINK-USDC", "Chainlink/USDC", "LINK", "USDC", "coinbase"),
        SymbolInfo(8, "XRP-USDC", "XRP/USDC", "XRP", "USDC", "coinbase"),
        SymbolInfo(9, "ADA-USDC", "Cardano/USDC", "ADA", "USDC", "coinbase")
    };
    
    for (const auto& symbol : default_symbols) {
        auto symbol_ptr = std::make_shared<SymbolInfo>(symbol);
        add_symbol_internal(symbol_ptr);
    }
    
    next_symbol_id_ = 10; // Start next IDs from 10
    
    util::log("[SYMBOL] Initialized " + std::to_string(default_symbols.size()) + " default symbols");
}

std::shared_ptr<SymbolInfo> SymbolManager::get_symbol_by_id(uint32_t symbol_id) const {
    std::lock_guard<std::mutex> lock(symbols_mutex_);
    
    auto it = symbols_by_id_.find(symbol_id);
    if (it != symbols_by_id_.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<SymbolInfo> SymbolManager::get_symbol_by_name(const std::string& symbol_name) const {
    std::lock_guard<std::mutex> lock(symbols_mutex_);
    
    auto it = symbols_by_name_.find(symbol_name);
    if (it != symbols_by_name_.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<SymbolInfo> SymbolManager::get_symbol_by_exchange_symbol(const std::string& exchange_symbol, const std::string& exchange) const {
    std::lock_guard<std::mutex> lock(symbols_mutex_);
    
    // For now, exchange symbols are the same as our universal symbols for Coinbase
    // Later this can be extended to handle different exchanges with different symbol formats
    auto it = symbols_by_name_.find(exchange_symbol);
    if (it != symbols_by_name_.end()) {
        if (it->second->exchange == exchange || exchange == "any") {
            return it->second;
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<SymbolInfo>> SymbolManager::get_all_symbols() const {
    std::lock_guard<std::mutex> lock(symbols_mutex_);
    
    std::vector<std::shared_ptr<SymbolInfo>> symbols;
    symbols.reserve(symbols_by_id_.size());
    
    for (const auto& pair : symbols_by_id_) {
        symbols.push_back(pair.second);
    }
    
    return symbols;
}

std::vector<std::shared_ptr<SymbolInfo>> SymbolManager::get_active_symbols() const {
    std::lock_guard<std::mutex> lock(symbols_mutex_);
    
    std::vector<std::shared_ptr<SymbolInfo>> active_symbols;
    
    for (const auto& pair : symbols_by_id_) {
        if (pair.second->active) {
            active_symbols.push_back(pair.second);
        }
    }
    
    return active_symbols;
}

uint32_t SymbolManager::get_next_symbol_id() {
    std::lock_guard<std::mutex> lock(symbols_mutex_);
    return next_symbol_id_++;
}

bool SymbolManager::is_valid_symbol_id(uint32_t symbol_id) const {
    std::lock_guard<std::mutex> lock(symbols_mutex_);
    return symbols_by_id_.find(symbol_id) != symbols_by_id_.end();
}

bool SymbolManager::subscribe_symbol(const std::string& client_id, uint32_t symbol_id, bool trades, bool level2) {
    if (!is_valid_symbol_id(symbol_id)) {
        util::log("[SYMBOL] Invalid symbol ID for subscription: " + std::to_string(symbol_id));
        return false;
    }
    
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    
    // Add to client subscriptions
    auto& client_subs = client_subscriptions_[client_id];
    auto it = std::find_if(client_subs.begin(), client_subs.end(),
                          [symbol_id](const SymbolSubscription& sub) {
                              return sub.symbol_id == symbol_id;
                          });
    
    if (it != client_subs.end()) {
        // Update existing subscription
        it->trades_subscribed = trades;
        it->level2_subscribed = level2;
    } else {
        // Create new subscription
        SymbolSubscription sub(symbol_id, client_id);
        sub.trades_subscribed = trades;
        sub.level2_subscribed = level2;
        sub.subscription_time = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        client_subs.push_back(sub);
    }
    
    // Add to symbol subscribers
    auto& symbol_subs = symbol_subscribers_[symbol_id];
    if (std::find(symbol_subs.begin(), symbol_subs.end(), client_id) == symbol_subs.end()) {
        symbol_subs.push_back(client_id);
    }
    
    auto symbol = get_symbol_by_id(symbol_id);
    util::log("[SYMBOL] Client " + client_id + " subscribed to " + 
              (symbol ? symbol->display_name : std::to_string(symbol_id)));
    
    return true;
}

bool SymbolManager::unsubscribe_symbol(const std::string& client_id, uint32_t symbol_id) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    
    // Remove from client subscriptions
    auto client_it = client_subscriptions_.find(client_id);
    if (client_it != client_subscriptions_.end()) {
        auto& subs = client_it->second;
        subs.erase(std::remove_if(subs.begin(), subs.end(),
                                 [symbol_id](const SymbolSubscription& sub) {
                                     return sub.symbol_id == symbol_id;
                                 }), subs.end());
    }
    
    // Remove from symbol subscribers
    auto symbol_it = symbol_subscribers_.find(symbol_id);
    if (symbol_it != symbol_subscribers_.end()) {
        auto& subs = symbol_it->second;
        subs.erase(std::remove(subs.begin(), subs.end(), client_id), subs.end());
    }
    
    util::log("[SYMBOL] Client " + client_id + " unsubscribed from symbol " + std::to_string(symbol_id));
    return true;
}

bool SymbolManager::unsubscribe_all(const std::string& client_id) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    
    // Get client's subscriptions to clean up symbol subscribers
    auto client_it = client_subscriptions_.find(client_id);
    if (client_it != client_subscriptions_.end()) {
        for (const auto& sub : client_it->second) {
            auto symbol_it = symbol_subscribers_.find(sub.symbol_id);
            if (symbol_it != symbol_subscribers_.end()) {
                auto& subs = symbol_it->second;
                subs.erase(std::remove(subs.begin(), subs.end(), client_id), subs.end());
            }
        }
    }
    
    // Remove all client subscriptions
    client_subscriptions_.erase(client_id);
    
    util::log("[SYMBOL] Removed all subscriptions for client " + client_id);
    return true;
}

std::vector<uint32_t> SymbolManager::get_client_subscriptions(const std::string& client_id) const {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    
    std::vector<uint32_t> symbol_ids;
    auto it = client_subscriptions_.find(client_id);
    if (it != client_subscriptions_.end()) {
        for (const auto& sub : it->second) {
            symbol_ids.push_back(sub.symbol_id);
        }
    }
    
    return symbol_ids;
}

std::vector<std::string> SymbolManager::get_symbol_subscribers(uint32_t symbol_id) const {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    
    auto it = symbol_subscribers_.find(symbol_id);
    if (it != symbol_subscribers_.end()) {
        return it->second;
    }
    return {};
}

bool SymbolManager::is_client_subscribed(const std::string& client_id, uint32_t symbol_id) const {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    
    auto it = client_subscriptions_.find(client_id);
    if (it != client_subscriptions_.end()) {
        for (const auto& sub : it->second) {
            if (sub.symbol_id == symbol_id) {
                return true;
            }
        }
    }
    return false;
}

size_t SymbolManager::get_symbol_count() const {
    std::lock_guard<std::mutex> lock(symbols_mutex_);
    return symbols_by_id_.size();
}

size_t SymbolManager::get_active_symbol_count() const {
    std::lock_guard<std::mutex> lock(symbols_mutex_);
    
    size_t count = 0;
    for (const auto& pair : symbols_by_id_) {
        if (pair.second->active) {
            count++;
        }
    }
    return count;
}

size_t SymbolManager::get_subscription_count() const {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    
    size_t count = 0;
    for (const auto& pair : client_subscriptions_) {
        count += pair.second.size();
    }
    return count;
}

std::string SymbolManager::get_status() const {
    std::stringstream ss;
    ss << "Symbol Manager Status:\n";
    ss << "  Total Symbols: " << get_symbol_count() << "\n";
    ss << "  Active Symbols: " << get_active_symbol_count() << "\n";
    ss << "  Total Subscriptions: " << get_subscription_count() << "\n";
    
    auto symbols = get_active_symbols();
    ss << "  Available Symbols:\n";
    for (const auto& symbol : symbols) {
        auto subscribers = get_symbol_subscribers(symbol->symbol_id);
        ss << "    " << symbol->display_name << " (" << symbol->symbol 
           << ") - ID: " << symbol->symbol_id << " - Subscribers: " << subscribers.size() << "\n";
    }
    
    return ss.str();
}

bool SymbolManager::validate_symbol_request(const std::string& symbol_name) const {
    // Try to find by symbol name
    auto symbol = get_symbol_by_name(symbol_name);
    if (symbol && symbol->active) {
        return true;
    }
    
    // Try to find by display name
    auto symbols = get_active_symbols();
    for (const auto& sym : symbols) {
        if (sym->display_name == symbol_name) {
            return true;
        }
    }
    
    return false;
}

uint32_t SymbolManager::resolve_symbol_name(const std::string& symbol_name) const {
    // Try to find by symbol name
    auto symbol = get_symbol_by_name(symbol_name);
    if (symbol && symbol->active) {
        return symbol->symbol_id;
    }
    
    // Try to find by display name
    auto symbols = get_active_symbols();
    for (const auto& sym : symbols) {
        if (sym->display_name == symbol_name) {
            return sym->symbol_id;
        }
    }
    
    return 0; // Invalid symbol ID
}

void SymbolManager::add_symbol_internal(std::shared_ptr<SymbolInfo> symbol) {
    symbols_by_id_[symbol->symbol_id] = symbol;
    symbols_by_name_[symbol->symbol] = symbol;
}

void SymbolManager::remove_symbol_internal(uint32_t symbol_id) {
    auto it = symbols_by_id_.find(symbol_id);
    if (it != symbols_by_id_.end()) {
        auto symbol = it->second;
        symbols_by_name_.erase(symbol->symbol);
        symbols_by_id_.erase(it);
    }
}

std::string SymbolManager::generate_client_key(const std::string& client_id, uint32_t symbol_id) const {
    return client_id + ":" + std::to_string(symbol_id);
}

} // namespace server
} // namespace coinbase_dtc_core