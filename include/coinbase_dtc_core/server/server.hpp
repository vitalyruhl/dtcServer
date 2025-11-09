#pragma once
#include <string>

namespace coinbase_dtc_core {
namespace server {

class Server {
public:
    bool start();
    void stop();
    std::string status() const;
};

} // namespace server
} // namespace coinbase_dtc_core
