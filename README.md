# coinbase-dtc-core

Open-source DTC (Data and Trade Communication) market data feed implementation for Coinbase.

## Overview

This project provides foundational components to translate Coinbase market data into the DTC protocol format. It is structured as a reusable static library (`coinbase_dtc_core`) and a simple server executable (`coinbase_dtc_server`).

## Components

- **DTC Protocol (`src/dtc_protocol`)**: Basic stubs for encoding/decoding DTC messages.
- **Coinbase Feed (`src/coinbase_feed`)**: Stubs for connecting to and parsing Coinbase data (WebSocket/REST to be implemented).
- **Server (`src/server`)**: A minimal server harness that will orchestrate feed connection and DTC broadcasting.
- **Include (`include`)**: Public headers.
- **Tests (`tests`)**: Unit test stubs.

## Build Requirements

- C++17 compatible compiler (MSVC, Clang, or GCC)
- CMake >= 3.16
- (Optional) `nlohmann::json` for JSON parsing. Currently not required; will be integrated later.

## Building

```pwsh
# From repository root
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Artifacts:

- Static library: `build/coinbase_dtc_core.lib` (or platform equivalent)
- Executable: `build/coinbase_dtc_server`

## Running

```pwsh
./build/coinbase_dtc_server
```

(Currently prints a startup message only.)

## Testing

```pwsh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTING=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

## Roadmap

1. Implement Coinbase WebSocket subscription handling.
2. Map Coinbase messages to internal model and DTC wire format.
3. Add robust error handling and reconnection logic.
4. Integrate `nlohmann::json` for parsing.
5. Expand test coverage (protocol encoding/decoding, feed resilience).

## License

Licensed under the Apache License, Version 2.0. See `LICENSE` for details.
