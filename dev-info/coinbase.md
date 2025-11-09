# Coinbase API Integration Guide

## Overview

This document provides comprehensive information about integrating with Coinbase's APIs for market data retrieval. Coinbase offers multiple API options for different use cases.

## API Options

### 1. Coinbase Advanced Trade API (Recommended for Trading)
- **Purpose**: Professional trading, order management, account information
- **Rate Limits**: Higher limits, designed for algorithmic trading
- **Authentication**: API Key + Secret + Passphrase
- **Cost**: Free with rate limits, paid plans for higher limits

### 2. Coinbase Exchange API (Pro/Legacy)
- **Purpose**: Market data, trading (legacy, being phased out)
- **Rate Limits**: Standard rate limits
- **Authentication**: API Key + Secret + Passphrase
- **Status**: Being migrated to Advanced Trade API

### 3. Coinbase Public API (No Auth Required)
- **Purpose**: Public market data only (prices, orderbook, trades)
- **Rate Limits**: Lower limits, no authentication required
- **Authentication**: None
- **Cost**: Free

## Creating Coinbase API Credentials

### Step 1: Create Coinbase Account
1. Go to [Coinbase](https://www.coinbase.com)
2. Sign up for an account
3. Complete identity verification if required
4. Enable 2FA (Two-Factor Authentication) for security

### Step 2: Access API Settings
1. Log into your Coinbase account
2. For **Advanced Trade API**:
   - Go to [Coinbase Developer Portal](https://docs.cloud.coinbase.com/advanced-trade/docs)
   - Click "Get Started" → "Create API Key"
3. For **Exchange API** (legacy):
   - Go to [Coinbase Pro](https://pro.coinbase.com) (if still available)
   - Settings → API

### Step 3: Create API Key (Advanced Trade)

**For Cloud Trading API (Recommended):**
1. Visit [Coinbase Cloud](https://cloud.coinbase.com/)
2. Sign up with your Coinbase account
3. Navigate to API Keys section
4. Click "Create New API Key"

**Configuration:**
- **Name**: `coinbase-dtc-core-api`
- **Permissions**: 
  - ✅ View (required for market data)
  - ❌ Trade (not needed for market data only)
  - ❌ Transfer (not needed)
- **IP Whitelist**: Add your server IP addresses (optional but recommended)

### Step 4: Save Credentials Securely

You'll receive:
- **API Key**: Public identifier
- **API Secret**: Private key for signing requests  
- **Passphrase**: Additional security layer (if using legacy API)

**⚠️ SECURITY IMPORTANT:**
- Store credentials in environment variables or secure key management
- Never commit credentials to version control
- Use minimal permissions (View only for market data)
- Consider IP whitelisting for production

## API Endpoints for Market Data

### Public Market Data (No Auth Required)

#### Base URL
```
https://api.exchange.coinbase.com  # Legacy Pro API
https://api.coinbase.com/v2       # Public API v2
```

#### Key Endpoints

**1. Product Information**
```http
GET /products
GET /products/{product-id}
```

**2. Historical Candles/OHLCV**
```http
GET /products/{product-id}/candles?start={start}&end={end}&granularity={granularity}
```
Granularity options: 60, 300, 900, 3600, 21600, 86400 (seconds)

**3. Recent Trades**
```http
GET /products/{product-id}/trades
```

**4. Order Book**
```http
GET /products/{product-id}/book?level={1|2|3}
```

**5. 24hr Stats**
```http
GET /products/{product-id}/stats
```

### Advanced Trade API (Auth Required)

#### Base URL
```
https://api.coinbase.com/api/v3
```

#### Authentication
Uses REST API with OAuth2-style authentication.

**Example Request Headers:**
```http
CB-ACCESS-KEY: your-api-key
CB-ACCESS-SIGN: signature
CB-ACCESS-TIMESTAMP: timestamp
CB-VERSION: 2023-01-01
Content-Type: application/json
```

## Rate Limits

### Public API
- **10 requests per second** per IP
- **10,000 requests per hour** per IP
- No burst limit

### Advanced Trade API
- **10 requests per second** (default)
- **Higher limits available** with paid plans
- Burst limits apply

### Best Practices
- Implement exponential backoff
- Cache frequently requested data
- Use WebSocket for real-time data when possible
- Monitor rate limit headers

## WebSocket Feeds (Real-time Data)

### Public WebSocket
```
wss://ws-feed.exchange.coinbase.com
```

**Subscription Example:**
```json
{
    "type": "subscribe",
    "product_ids": ["BTC-USD", "ETH-USD"],
    "channels": [
        "level2",
        "heartbeat",
        {
            "name": "ticker",
            "product_ids": ["BTC-USD"]
        }
    ]
}
```

### Available Channels
- **ticker**: Real-time price updates
- **level2**: Order book updates
- **matches**: Trade executions  
- **heartbeat**: Connection health
- **status**: Product status updates

## Implementation Strategy for coinbase-dtc-core

### Phase 1: Public API Integration
```cpp
// Recommended approach for market data only
class CoinbasePublicAPI {
public:
    // Historical data
    std::vector<Candle> getHistoricalCandles(
        const std::string& product,
        const std::string& start,
        const std::string& end,
        int granularity
    );
    
    // Recent trades
    std::vector<Trade> getRecentTrades(const std::string& product);
    
    // Order book
    OrderBook getOrderBook(const std::string& product, int level = 2);
};
```

### Phase 2: WebSocket Integration
```cpp
class CoinbaseWebSocketClient {
public:
    void subscribe(const std::vector<std::string>& products,
                  const std::vector<std::string>& channels);
    void onTicker(std::function<void(const TickerUpdate&)> callback);
    void onTrade(std::function<void(const Trade&)> callback);
};
```

### Phase 3: Authentication ✅ **IMPLEMENTED**
```cpp
// JWT Authentication for CDP Advanced Trade API (ES256/ECDSA)
class CoinbaseJWTAuth {
private:
    std::string api_key_id_;
    std::string private_key_pem_;
    
public:
    // Generate JWT token with ES256 (ECDSA) signing
    std::string generateJWT(const std::string& method,
                           const std::string& path);
    
    // Create authenticated HTTP headers
    std::map<std::string, std::string> createAuthHeaders(
        const std::string& method,
        const std::string& path);
};

// Current Implementation Status:
// ✅ JWT token generation with ES256/ECDSA  
// ✅ Coinbase CDP API format compliance
// ✅ ECDSA P-256 private key support
// ✅ Proper claims structure (no deprecated 'audience')
// ✅ PowerShell authentication tests passing
```

### Phase 4: DTC Integration ❌ **NOT STARTED** (Next Priority)
```cpp  
// Future implementation for DTC protocol integration
class DTCCoinbaseIntegration {
public:
    void forwardMarketDataToDTC(const MarketData& data);
    void handleDTCTradingRequests(const DTCTradeRequest& request);
};
```

## Sample HTTP Requests

### Get BTC-USD Candles (Last 24 Hours)
```bash
curl "https://api.exchange.coinbase.com/products/BTC-USD/candles?start=$(date -d '1 day ago' -u +%Y-%m-%dT%H:%M:%SZ)&end=$(date -u +%Y-%m-%dT%H:%M:%SZ)&granularity=3600"
```

### Get Current BTC-USD Price
```bash
curl "https://api.exchange.coinbase.com/products/BTC-USD/ticker"
```

### Get Order Book
```bash
curl "https://api.exchange.coinbase.com/products/BTC-USD/book?level=2"
```

## Error Handling

### Common HTTP Status Codes
- **200**: Success
- **400**: Bad Request (invalid parameters)
- **401**: Unauthorized (invalid credentials)
- **403**: Forbidden (insufficient permissions)  
- **404**: Not Found (invalid product ID)
- **429**: Rate Limited
- **500**: Internal Server Error

### Example Error Response
```json
{
    "message": "rate limit exceeded",
    "error_details": {
        "rate_limit": {
            "max_requests": 10,
            "window_size": 1,
            "current_requests": 11
        }
    }
}
```

## Security Considerations

1. **API Key Management**
   - Store in environment variables or secure vault
   - Use minimal required permissions
   - Rotate keys regularly

2. **Network Security**
   - Use HTTPS only
   - Implement certificate pinning
   - Add IP whitelisting when possible

3. **Request Signing**
   - Implement proper HMAC signature generation
   - Use current timestamps to prevent replay attacks
   - Validate responses to detect tampering

## Testing Strategy

### Unit Tests
- Mock HTTP responses for API client testing
- Test rate limiting logic
- Verify request signing accuracy

### Integration Tests  
- Test against Coinbase sandbox environment
- Verify data parsing and validation
- Test error handling scenarios

### Load Testing
- Verify rate limit compliance
- Test connection pooling efficiency
- Measure data processing throughput

## Useful Resources

- [Coinbase Advanced Trade API Docs](https://docs.cloud.coinbase.com/advanced-trade/docs)
- [Coinbase Exchange API Docs](https://docs.cloud.coinbase.com/exchange/docs) (legacy)
- [WebSocket Feed Documentation](https://docs.cloud.coinbase.com/exchange/docs/websocket-overview)
- [API Status Page](https://status.coinbase.com/)
- [Developer Forum](https://forums.coinbase.com/)