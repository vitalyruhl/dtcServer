#include "coinbase_dtc_core/exchanges/coinbase/coinbase_feed.hpp"
#include "coinbase_dtc_core/core/util/log.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

using namespace open_dtc_server;
class TestCallbacks
{
public:
    std::atomic<int> trade_count{0};
    std::atomic<int> level2_count{0};

    void on_trade(const exchanges::base::MarketTrade &trade)
    {

        trade_count++;
        util::log("[CALLBACK] Trade received: " + trade.symbol + " Price: " + std::to_string(trade.price) +
                  " Volume: " + std::to_string(trade.quantity) +
                  " Side: " + trade.side);
    }

    void on_level2(const exchanges::base::MarketLevel2 &level2)
    {
        level2_count++;
              util::log("[CALLBACK] Level2 received: " + level2.symbol + 
                            \" Bids: \" + std::to_string(level2.bids.size()) + \n                 \" Asks: \" + std::to_string(level2.asks.size()));\n    }\n};\n\nint main() {\n    util::log(\"[TEST] Starting Coinbase Feed tests...\");\n    \n    try {\n        // Test 1: Feed creation\n        exchanges::coinbase::CoinbaseFeed feed;\n        util::log(\"[TEST] ✅ Coinbase feed created\");\n        \n        // Test 2: Exchange name\n        std::string exchange_name = feed.get_exchange_name();\n        if (exchange_name == \"coinbase\") {\n            util::log(\"[TEST] ✅ Exchange name correct: \" + exchange_name);\n        }\n        \n        // Test 3: Symbol normalization\n        std::string normalized = feed.normalize_symbol(\"BTC-USD\");\n        util::log(\"[TEST] Symbol normalization: BTC-USD -> \" + normalized);\n        \n        // Test 4: Connection test\n        bool connected = feed.connect();\n        if (connected) {\n            util::log(\"[TEST] ✅ Connection successful\");\n            \n            // Test 5: Set up callbacks\n            TestCallbacks callbacks;\n            feed.set_trade_callback([&callbacks](const exchanges::base::MarketTrade& trade) {\n                callbacks.on_trade(trade);\n            });\n            \n            feed.set_level2_callback([&callbacks](const exchanges::base::MarketLevel2& level2) {\n                callbacks.on_level2(level2);\n            });\n            \n            // Test 6: Subscribe to symbols\n            bool sub_trades = feed.subscribe_trades(\"BTC-USD\");\n            bool sub_level2 = feed.subscribe_level2(\"BTC-USD\");\n            \n            if (sub_trades && sub_level2) {\n                util::log(\"[TEST] ✅ Subscriptions successful\");\n                \n                // Test 7: Wait for some data\n                util::log(\"[TEST] Waiting for market data (5 seconds)...\");\n                std::this_thread::sleep_for(std::chrono::seconds(5));\n                \n                util::log(\"[TEST] Received \" + std::to_string(callbacks.trade_count.load()) + \" trades\");\n                util::log(\"[TEST] Received \" + std::to_string(callbacks.level2_count.load()) + \" level2 updates\");\n                \n                // Test 8: Check subscribed symbols\n                auto subscribed = feed.get_subscribed_symbols();\n                util::log(\"[TEST] Subscribed symbols: \" + std::to_string(subscribed.size()));\n                \n                // Test 9: Unsubscribe\n                bool unsub = feed.unsubscribe(\"BTC-USD\");\n                if (unsub) {\n                    util::log(\"[TEST] ✅ Unsubscribe successful\");\n                }\n            }\n            \n            // Test 10: Disconnect\n            feed.disconnect();\n            if (!feed.is_connected()) {\n                util::log(\"[TEST] ✅ Disconnection successful\");\n            }\n        }\n        \n        util::log(\"[TEST] All Coinbase Feed tests completed successfully! ✅\");\n        return 0;\n        \n    } catch (const std::exception& e) {\n        util::log(\"[ERROR] Coinbase feed test failed: \" + std::string(e.what()));\n        return 1;\n    }\n}"