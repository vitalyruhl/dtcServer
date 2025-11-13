#include "coinbase_dtc_core/core/util/log.hpp"
#include <iostream>

namespace open_dtc_server
{
    namespace util
    {

        void log(const std::string &message) { std::cout << "[coinbase-dtc-core] " << message << '\n'; }

    } // namespace util
} // namespace open_dtc_server
