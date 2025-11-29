# GUI: DOM, Candles, Trade History Integration

Status Legend: ✅ COMPLETED, 🚧 CURRENT, ⏳ PENDING

Branch: `feature/gui-dom-candles-trades`
Base: `main`

## 1. L2 Auth & Server DOM

# GUI TODO: DOM, Candles, Trades

## Test Review (2025-11-29)
- [DONE] DTC logon/positions flow works with real Coinbase data.
- [DONE] MarketDataRequest for `ETH-USD` succeeds; live 107/108 received.
- [OBSERVED] 146 incremental updates arrive but `size=0.000000` for both sides.
- [OBSERVED] Unsubscribe response logs as "SUCCESS" with ID=0; should be labeled clearly as unsubscribe ACK.
- [OBSERVED] `BTC-USDC` shows "No symbol ID found" → symbol_id mapping/cache incomplete.
- [POLICY] `[MOCKED DATA] DOM Data` was shown; must only appear when explicitly requested.

## Actions
1) DOM quantities in 146
	- Root cause: Server emits price/side/position but not size from Coinbase L2.
	- Do: Parse quantity from Coinbase L2 payload; include in 146 serialization; update GUI to render non-zero size.
	- Files: `src/coinbase_feed/feed.cpp`, `src/server/server.cpp`, `src/dtc_protocol/protocol.cpp`.

2) Coinbase L2 authentication
	- Verify JWT ES256 claims and subscription payload for `level2` channel.
	- Gate per-symbol L2 availability; fallback to L1 when unauthorized.
	- Files: `include/coinbase_dtc_core/feed/coinbase/feed.hpp`, `src/coinbase_feed/feed.cpp`.

3) Symbol ID mapping
	- Ensure 502 SecurityDefinitionResponse carries `symbol_id`; server populates, GUI caches.
	- Fix lookup path so `BTC-USDC` and `ETH-USD` resolve IDs before subscribing.
	- Files: `src/server/server.cpp`, `src/dtc_test_client/dtc_gui_client.cpp`.

4) Unsubscribe UX/logging
	- Change GUI log label from "MarketDataResponse SUCCESS" to "Unsubscribe ACK" when ID==0.
	- Files: `src/dtc_test_client/dtc_gui_client.cpp`.

5) Mock DOM policy
	- Remove or guard the mock DOM output; only show when user explicitly toggles a "Use mock data" option.
	- Files: `src/dtc_test_client/dtc_gui_client.cpp`.

## Acceptance Criteria
- 146 updates show non-zero sizes matching Coinbase L2 quantities.
- L2 auth succeeds where available; GUI indicates L2 status per symbol.
- No "No symbol ID found" warnings for valid products after 502 retrieval.
- Unsubscribe logs labeled correctly; no ambiguity.
- No mock DOM output unless explicitly enabled.
- ⏳ Create DOM panel in `src/dtc_test_client/dtc_gui_client.*`
## 3. GUI Trade History
- ⏳ Consume DTC 107 trade updates
- ⏳ Maintain ring buffer of recent trades (configurable size)
- ⏳ Render list with time, side, price, size; add clear/filter controls

## 4. GUI Candles (Phase 1)
- ⏳ Aggregate client-side candles from trades: configurable intervals (1s/5s/1m)
- ⏳ Render basic OHLCV chart (lightweight GDI / Win32 drawing)

## 5. DTC Candles (Phase 2)
- ⏳ Server provides historical/snapshot OHLCV messages (DTC-compliant)
- ⏳ GUI prefers server-fed candles; fallback to client-side aggregation when unavailable

## 6. Performance & UX
- ⏳ Batch redraws; throttle UI updates; cap DOM levels
- ⏳ Use ring buffers for trades/candles
- ⏳ Keep console output concise per `copilot-instructions.md`

## 7. Testing & Docs
- ⏳ Build & run verification on Windows
- ⏳ Update `dev-info/TODO.md` progress markers (✅ / 🚧)
- ⏳ Add integration tests where feasible (depth/trade message handling)

## Notes (Compliance)
- DTC-only data flow: GUI must consume DTC protocol messages, never direct Coinbase APIs
- Namespaces: document any new namespaces in `dev-info/namespaces.md` before adding them
- No emojis in code/logs; use [SUCCESS], [ERROR], [WARNING]
- Credentials: prefer absolute paths or `CDP_CREDENTIALS_PATH` env var
