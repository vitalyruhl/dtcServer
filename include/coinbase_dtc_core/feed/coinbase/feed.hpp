#pragma once
#include <string>
#include <vector>

namespace coinbase_dtc_core {
namespace feed {
namespace coinbase {

class Feed {
public:
    bool connect();
    std::vector<std::string> subscribed_channels() const;
};

} // namespace coinbase
} // namespace feed
} // namespace coinbase_dtc_core
