#include "coinbase_dtc_core/feed/coinbase/feed.hpp"

namespace coinbase_dtc_core {
namespace feed {
namespace coinbase {

bool Feed::connect() { return true; }
std::vector<std::string> Feed::subscribed_channels() const { return {}; }

} // namespace coinbase
} // namespace feed
} // namespace coinbase_dtc_core
