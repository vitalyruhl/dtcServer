#include "coinbase_dtc_core/server/server.hpp"
#include "coinbase_dtc_core/util/log.hpp"
#include "coinbase_dtc_core/dtc/protocol.hpp"
#include "coinbase_dtc_core/feed/coinbase/feed.hpp"
#include <iostream>

int main() {
    using namespace coinbase_dtc_core;
    util::log("coinbase_dtc_server starting");

    dtc::Protocol proto;
    util::log(std::string("DTC protocol v") + proto.version());

    server::Server srv;
    if (srv.start()) {
        util::log("Server status: " + srv.status());
    }

    feed::coinbase::Feed feed;
    if (feed.connect()) {
        util::log("Connected to Coinbase feed (stub)");
    }

    util::log("coinbase_dtc_server exiting");
    return 0;
}
