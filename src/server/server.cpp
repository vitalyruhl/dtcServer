#include "coinbase_dtc_core/server/server.hpp"

namespace coinbase_dtc_core {
namespace server {

bool Server::start() { return true; }
void Server::stop() {}
std::string Server::status() const { return "running"; }

} // namespace server
} // namespace coinbase_dtc_core
