#include "coinbase_dtc_core/exchanges/factory/exchange_factory.hpp"
#include "coinbase_dtc_core/exchanges/coinbase/coinbase_feed.hpp"
#include "coinbase_dtc_core/exchanges/binance/binance_feed.hpp"
#include "coinbase_dtc_core/core/util/log.hpp"
#include <stdexcept>

namespace open_dtc_server
{
    namespace exchanges
    {
        namespace factory
        {

            std::unique_ptr<base::ExchangeFeedBase> ExchangeFactory::create_feed(const base::ExchangeConfig &config)
            {
                util::log("[FACTORY] Creating feed for exchange: " + config.name);

                // Convert exchange name to lowercase for comparison
                std::string exchange_name = config.name;
                std::transform(exchange_name.begin(), exchange_name.end(), exchange_name.begin(), ::tolower);

                if (exchange_name == "coinbase")
                {
                    auto feed = std::make_unique<coinbase::CoinbaseFeed>();
                    // Configure Coinbase-specific settings if needed
                    return std::move(feed);
                }
                else if (exchange_name == "binance")
                {
                    auto feed = std::make_unique<binance::BinanceFeed>();
                    // Configure Binance-specific settings if needed
                    return std::move(feed);
                }
                else
                {
                    throw std::invalid_argument("Unsupported exchange: " + config.name +
                                                ". Supported exchanges: coinbase, binance");
                }
            }

            std::vector<std::string> ExchangeFactory::get_supported_exchanges()
            {
                return {"coinbase", "binance"};
            }

            bool ExchangeFactory::is_exchange_supported(const std::string &exchange_name)
            {
                auto supported = get_supported_exchanges();
                std::string lower_name = exchange_name;
                std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

                return std::find(supported.begin(), supported.end(), lower_name) != supported.end();
            }

        } // namespace factory
    } // namespace exchanges
} // namespace open_dtc_server