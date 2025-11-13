#include "coinbase_dtc_core/exchanges/base/exchange_feed.hpp"
#include "coinbase_dtc_core/exchanges/coinbase/coinbase_feed.hpp"
#include "coinbase_dtc_core/exchanges/binance/binance_feed.hpp"
#include "coinbase_dtc_core/exchanges/factory/exchange_factory.hpp"
#include "coinbase_dtc_core/core/util/log.hpp"
#include <stdexcept>
#include <sstream>
#include <utility>

namespace open_dtc_server
{
    namespace exchanges
    {
        namespace base
        {

            // Multi-Exchange Feed Implementation
            MultiExchangeFeed::MultiExchangeFeed()
            {
                util::log("[MULTI] Multi-exchange feed initialized");
            }

            MultiExchangeFeed::~MultiExchangeFeed()
            {
                // Disconnect all exchanges
                for (auto &[name, feed] : exchanges_)
                {
                    if (feed && feed->is_connected())
                    {
                        feed->disconnect();
                    }
                }
                util::log("[MULTI] Multi-exchange feed destroyed");
            }

            bool MultiExchangeFeed::add_exchange(const ExchangeConfig &config)
            {
                std::lock_guard<std::mutex> lock(exchanges_mutex_);

                if (exchanges_.find(config.name) != exchanges_.end())
                {
                    util::log("[MULTI] Exchange " + config.name + " already exists");
                    return false;
                }

                try
                {
                    auto feed = exchanges::factory::ExchangeFactory::create_feed(config);

                    // Set callbacks to aggregate data
                    feed->set_trade_callback([this](const MarketTrade &trade)
                                             { this->on_trade_data(trade); });

                    feed->set_level2_callback([this](const MarketLevel2 &level2)
                                              { this->on_level2_data(level2); });

                    exchanges_[config.name] = std::move(feed);

                    util::log("[MULTI] Added exchange: " + config.name);
                    return true;
                }
                catch (const std::exception &e)
                {
                    util::log("[MULTI] Failed to add exchange " + config.name + ": " + e.what());
                    return false;
                }
            }

            bool MultiExchangeFeed::remove_exchange(const std::string &exchange_name)
            {
                std::lock_guard<std::mutex> lock(exchanges_mutex_);

                auto it = exchanges_.find(exchange_name);
                if (it == exchanges_.end())
                {
                    return false;
                }

                if (it->second->is_connected())
                {
                    it->second->disconnect();
                }

                exchanges_.erase(it);
                util::log("[MULTI] Removed exchange: " + exchange_name);
                return true;
            }

            std::vector<std::string> MultiExchangeFeed::get_active_exchanges() const
            {
                std::lock_guard<std::mutex> lock(exchanges_mutex_);

                std::vector<std::string> active;
                for (const auto &[name, feed] : exchanges_)
                {
                    if (feed && feed->is_connected())
                    {
                        active.push_back(name);
                    }
                }
                return active;
            }

            bool MultiExchangeFeed::subscribe_symbol(const std::string &symbol, const std::string &exchange)
            {
                std::lock_guard<std::mutex> lock(exchanges_mutex_);

                bool success = false;

                if (exchange.empty())
                {
                    // Subscribe on all exchanges
                    for (auto &[name, feed] : exchanges_)
                    {
                        if (feed && feed->is_connected())
                        {
                            if (feed->subscribe_trades(symbol) && feed->subscribe_level2(symbol))
                            {
                                success = true;
                                util::log("[MULTI] Subscribed " + symbol + " on " + name);
                            }
                        }
                    }
                }
                else
                {
                    // Subscribe on specific exchange
                    auto it = exchanges_.find(exchange);
                    if (it != exchanges_.end() && it->second && it->second->is_connected())
                    {
                        success = it->second->subscribe_trades(symbol) && it->second->subscribe_level2(symbol);
                        if (success)
                        {
                            util::log("[MULTI] Subscribed " + symbol + " on " + exchange);
                        }
                    }
                }

                return success;
            }

            bool MultiExchangeFeed::unsubscribe_symbol(const std::string &symbol, const std::string &exchange)
            {
                std::lock_guard<std::mutex> lock(exchanges_mutex_);

                bool success = false;

                if (exchange.empty())
                {
                    // Unsubscribe from all exchanges
                    for (auto &[name, feed] : exchanges_)
                    {
                        if (feed && feed->is_connected())
                        {
                            if (feed->unsubscribe(symbol))
                            {
                                success = true;
                                util::log("[MULTI] Unsubscribed " + symbol + " from " + name);
                            }
                        }
                    }
                }
                else
                {
                    // Unsubscribe from specific exchange
                    auto it = exchanges_.find(exchange);
                    if (it != exchanges_.end() && it->second)
                    {
                        success = it->second->unsubscribe(symbol);
                        if (success)
                        {
                            util::log("[MULTI] Unsubscribed " + symbol + " from " + exchange);
                        }
                    }
                }

                return success;
            }

            std::string MultiExchangeFeed::get_status() const
            {
                std::lock_guard<std::mutex> lock(exchanges_mutex_);

                std::ostringstream ss;
                ss << "Multi-Exchange Feed Status:\n";
                ss << "  Active Exchanges: " << exchanges_.size() << "\n";

                for (const auto &[name, feed] : exchanges_)
                {
                    ss << "  " << name << ": " << (feed->is_connected() ? "Connected" : "Disconnected") << "\n";
                }

                return ss.str();
            }

            size_t MultiExchangeFeed::get_total_subscriptions() const
            {
                std::lock_guard<std::mutex> lock(exchanges_mutex_);

                size_t total = 0;
                for (const auto &[name, feed] : exchanges_)
                {
                    if (feed)
                    {
                        total += feed->get_subscribed_symbols().size();
                    }
                }
                return total;
            }

            void MultiExchangeFeed::on_trade_data(const MarketTrade &trade)
            {
                if (trade_callback_)
                {
                    trade_callback_(trade);
                }
            }

            void MultiExchangeFeed::on_level2_data(const MarketLevel2 &level2)
            {
                if (level2_callback_)
                {
                    level2_callback_(level2);
                }
            }

        } // namespace base
    } // namespace exchanges
} // namespace open_dtc_server