# API Secrets Configuration

## Overview

This folder contains API credentials and sensitive configuration for the coinbase-dtc-core project.

**🔒 SECURITY: This folder is excluded from version control via `.gitignore`**

## Setup Instructions

### 1. Create Your Credentials File

```bash
# Copy the template to create your credentials file
cp coinbase.h.template coinbase.h

# Edit with your actual API credentials
# Use your favorite editor (vim, nano, VS Code, etc.)
```

### 2. Get Coinbase API Credentials

For **public market data only** (recommended to start):
- No API credentials needed
- Leave `COINBASE_API_KEY`, `COINBASE_API_SECRET`, and `COINBASE_PASSPHRASE` as empty strings
- The application will use public endpoints only

For **authenticated API access** (optional):
1. Go to [Coinbase Cloud](https://cloud.coinbase.com/)
2. Create API key with **View** permissions only
3. Copy the API Key, Secret, and Passphrase to `coinbase.h`

### 3. File Structure

```
secrets/
├── README.md              # This file (tracked in git)
├── coinbase.h.template     # Template file (tracked in git)  
└── coinbase.h             # Your actual credentials (NOT tracked)
```

## Security Best Practices

### ✅ Do This
- **Use minimal permissions**: View only for market data
- **Rotate credentials** regularly  
- **Use IP whitelisting** for production
- **Monitor API usage** for unusual activity
- **Store backup** of credentials in secure password manager

### ❌ Never Do This
- **Commit credentials** to version control
- **Share credentials** in chat/email
- **Use production credentials** for development/testing
- **Give excessive permissions** (Trade/Transfer unless needed)
- **Hardcode credentials** in source files outside this folder

## Environment Variables Alternative

Instead of using the header file, you can use environment variables:

### In Docker Container
```bash
# Set environment variables in container
export COINBASE_API_KEY="your-key-here"
export COINBASE_API_SECRET="your-secret-here"  
export COINBASE_PASSPHRASE="your-passphrase-here"
```

### In docker-compose.yml
```yaml
environment:
  - COINBASE_API_KEY=${COINBASE_API_KEY}
  - COINBASE_API_SECRET=${COINBASE_API_SECRET}
  - COINBASE_PASSPHRASE=${COINBASE_PASSPHRASE}
```

### In .env file (also in .gitignore)
```bash
COINBASE_API_KEY=your-key-here
COINBASE_API_SECRET=your-secret-here
COINBASE_PASSPHRASE=your-passphrase-here
```

## Code Usage

### Include in C++ Code
```cpp
#include "../secrets/coinbase.h"

using namespace coinbase_dtc_core::secrets;

// Use the credentials
std::string api_key = COINBASE_API_KEY;
std::string api_secret = COINBASE_API_SECRET;

// Use public API if no credentials
if (api_key.empty()) {
    // Use public endpoints only
    std::string url = COINBASE_PUBLIC_API_URL;
} else {
    // Use authenticated endpoints  
    std::string url = COINBASE_ADVANCED_API_URL;
}
```

## Testing

### Verify Setup
```cpp
// Test that credentials are loaded
assert(!COINBASE_PUBLIC_API_URL.empty());
assert(SUPPORTED_PRODUCTS.size() > 0);

// Log configuration (without exposing secrets)
std::cout << "API URL: " << COINBASE_PUBLIC_API_URL << std::endl;
std::cout << "Supported products: " << SUPPORTED_PRODUCTS.size() << std::endl;
std::cout << "Has API key: " << (!COINBASE_API_KEY.empty() ? "Yes" : "No") << std::endl;
```

## Troubleshooting

### Common Issues

1. **File not found error**:
   - Make sure you copied `coinbase.h.template` to `coinbase.h`
   - Check that the file is in the `secrets/` directory

2. **Empty credentials**:
   - For public API testing, empty credentials are fine
   - For authenticated API, make sure you filled in real values

3. **Build errors**:
   - Check that `#include "../secrets/coinbase.h"` path is correct
   - Ensure the secrets directory is in the same level as src/

4. **API authentication errors**:
   - Verify your API key has correct permissions
   - Check that IP whitelisting allows your server IP
   - Ensure API key is active and not expired

## Next Steps

1. **Start with public API**: Test with empty credentials first
2. **Add authentication**: Only when you need private data/trading
3. **Implement rate limiting**: Respect Coinbase API limits
4. **Add error handling**: Handle authentication failures gracefully

For more information, see:
- `dev-info/coinbase.md` - Coinbase API integration guide
- `TODO.md` - Development roadmap
- `dev-info/docker-info.md` - Development environment setup