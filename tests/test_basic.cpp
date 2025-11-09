#include "coinbase_dtc_core/dtc/protocol.hpp"
#include <cassert>
#include <iostream>

int main() {
    coinbase_dtc_core::dtc::Protocol p;
    auto v = p.version();
    std::cout << "Protocol version: " << v << "\n";
    assert(!v.empty());
    return 0;
}
