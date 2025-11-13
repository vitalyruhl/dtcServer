#pragma once

#include "coinbase_dtc_core/exchanges/base/exchange_feed.hpp"
#include <memory>
#include <stdexcept>

namespace open_dtc_server
{
    namespace exchanges
    {
        namespace factory
        {

            /**
             * Factory for creating exchange feed instances.
             *
             * This is where new exchanges are registered and created.
             * When adding a new exchange (e.g., Kraken), you add a case here.
             */
            class ExchangeFactory
            {
            public:
                /**
                 * Create an exchange feed instance based on configuration.
                 *
                 * @param config Exchange configuration including exchange name
                 * @return Unique pointer to the created exchange feed
                 * @throws std::invalid_argument if exchange name is not supported
                 */
                static std::unique_ptr<base::ExchangeFeedBase> create_feed(
                    const base::ExchangeConfig &config);

                /**
                 * Check if an exchange is supported.
                 *
                 * @param exchange_name Name of the exchange to check
                 * @return true if exchange is supported, false otherwise
                 */
                static bool is_supported(const std::string &exchange_name);

                /**
                 * Get list of all supported exchanges.
                 *
                 * @return Vector of supported exchange names
                 */
                static std::vector<std::string> get_supported_exchanges();

                /**
                 * Get default configuration for an exchange.
                 *
                 * @param exchange_name Name of the exchange
                 * @return Default configuration for the exchange
                 * @throws std::invalid_argument if exchange is not supported
                 */
                static base::ExchangeConfig get_default_config(const std::string &exchange_name);

            private:
                // Private constructor - this is a static factory
                ExchangeFactory() = delete;
            };

            /**
             * Exception thrown when trying to create an unsupported exchange.
             */
            class UnsupportedExchangeError : public std::invalid_argument
            {
            public:
                explicit UnsupportedExchangeError(const std::string &exchange_name)
                    : std::invalid_argument("Unsupported exchange: " + exchange_name) {}
            };

        } // namespace factory
    } // namespace exchanges
} // namespace open_dtc_server