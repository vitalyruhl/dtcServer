#!/bin/bash
# Test script to validate JWT authentication using curl

echo "🔍 Testing Coinbase API Authentication..."

# Test public endpoint first
echo ""
echo "📊 Testing public endpoint (no auth)..."
curl -s -w "Status: %{http_code}\n" \
  "https://api.coinbase.com/api/v3/brokerage/market/products?limit=1" \
  | jq -r '.products[0].product_id // "Error"' 2>/dev/null || echo "JSON parse error"

# Generate JWT token using our executable
echo ""
echo "🎫 Generating JWT token..."
JWT_TOKEN=$(./out/build/windows-vs2022/Debug/test_jwt_auth.exe 2>/dev/null | grep "JWT Token:" | cut -d' ' -f3)

if [ -z "$JWT_TOKEN" ]; then
    echo "❌ Failed to generate JWT token"
    exit 1
fi

echo "✅ JWT Token generated (${#JWT_TOKEN} chars)"

# Test authenticated endpoint
echo ""
echo "🔑 Testing authenticated endpoint..."
curl -s -w "\nStatus: %{http_code}\n" \
  -H "Authorization: Bearer $JWT_TOKEN" \
  -H "Content-Type: application/json" \
  "https://api.coinbase.com/api/v3/brokerage/accounts"

echo ""
echo "Done."