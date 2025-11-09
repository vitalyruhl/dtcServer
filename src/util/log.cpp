#include "coinbase_dtc_core/util/log.hpp"
#include <iostream>

namespace coinbase_dtc_core {
namespace util {

void log(const std::string& message) { std::cout << "[coinbase-dtc-core] " << message << '\n'; }

} // namespace util
} // namespace coinbase_dtc_core
