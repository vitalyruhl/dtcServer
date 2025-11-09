# coinbase-dtc-core

> **⚠️ WARNING: DEVELOPMENT PROJECT - NOT READY FOR PRODUCTION USE**
>
> This project is currently under active development and is **NOT** ready for production use. 
> Many core features are still stubbed or incomplete. Use at your own risk.

Open-source DTC (Data and Trade Communication) market data feed implementation for Coinbase.

## Status

**🚧 UNDER CONSTRUCTION 🚧**

This is a **work-in-progress** project with the following limitations:

- **No actual Coinbase connection**: WebSocket/REST API implementation is stubbed
- **Incomplete DTC protocol**: Only basic message structure is implemented
- **No real data processing**: Feed parsing and message translation are not functional
- **Minimal testing**: Test coverage is incomplete
- **No production hardening**: Error handling, reconnection logic, and stability features are missing

**What currently works:**
- ✅ Project builds successfully with CMake
- ✅ Basic project structure and interfaces
- ✅ Simple server executable (prints startup message only)
- ✅ GitHub Actions CI/CD pipeline
- ✅ Docker containerization

**Do not use this in production environments.**

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

**Planned features and improvements:**

1. **Implement Coinbase WebSocket subscription handling** - Real-time market data connection
2. **Map Coinbase messages to internal model and DTC wire format** - Data transformation layer
3. **Add robust error handling and reconnection logic** - Production stability
4. **Integrate `nlohmann::json` for parsing** - JSON message processing
5. **Expand test coverage** - Protocol encoding/decoding, feed resilience
6. **Add configuration management** - Runtime configuration options
7. **Implement logging and monitoring** - Operational observability
8. **Performance optimization** - Low-latency data processing

**Contributions welcome!** Please see the project issues for specific tasks.

## License

Licensed under the Apache License, Version 2.0. See `LICENSE` for details.


<br>
<br>


## Donate

<table align="center" width="100%" border="0" bgcolor:=#3f3f3f>
<tr align="center">
<td align="center">  
if you prefer a one-time donation

[![donate-Paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://paypal.me/FamilieRuhl)

</td>

<td align="center">  
Become a patron, by simply clicking on this button (**very appreciated!**):

[![Become a patron](https://c5.patreon.com/external/logo/become_a_patron_button.png)](https://www.patreon.com/join/6555448/checkout?ru=undefined)

</td>
</tr>
</table>

<br>
<br>

---
