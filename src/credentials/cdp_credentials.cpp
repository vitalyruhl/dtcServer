#include "coinbase_dtc_core/credentials/cdp_credentials.hpp"

namespace coinbase_dtc_core {
namespace credentials {

// Implementation for the forward declaration
CDPCredentials loadCDPCredentials() {
    return CDPCredentialsManager::getCredentials();
}

} // namespace credentials
} // namespace coinbase_dtc_core