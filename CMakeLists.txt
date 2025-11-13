cmake_minimum_required(VERSION 3.16)
project(coinbase-dtc-core VERSION 0.2.0 LANGUAGES CXX)

option(ENABLE_TESTING "Enable building tests" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find packages (vcpkg integration for Windows, pkg-config for Linux)
find_package(PkgConfig QUIET)
if(WIN32)
    # Windows/vcpkg approach
    find_package(CURL QUIET)
    find_package(nlohmann_json QUIET)
    find_package(jwt-cpp CONFIG QUIET)
    find_package(OpenSSL QUIET)
else()
    # Linux approach - use pkg-config for libcurl
    if(PkgConfig_FOUND)
        pkg_check_modules(CURL QUIET libcurl)
        if(CURL_FOUND)
            # Create an imported target compatible with CURL::libcurl
            add_library(CURL::libcurl INTERFACE IMPORTED)
            target_link_libraries(CURL::libcurl INTERFACE ${CURL_LIBRARIES})
            target_include_directories(CURL::libcurl INTERFACE ${CURL_INCLUDE_DIRS})
            target_compile_options(CURL::libcurl INTERFACE ${CURL_CFLAGS_OTHER})
        endif()
    endif()
    find_package(nlohmann_json QUIET)
    find_package(jwt-cpp CONFIG QUIET)
    find_package(OpenSSL QUIET)
endif()

# Check if secrets file exists (for local development)
if(EXISTS "${CMAKE_SOURCE_DIR}/secrets/coinbase.h")
    message(STATUS "Found secrets file - enabling secrets support")
    add_compile_definitions(INCLUDE_SECRETS_FILE)
else()
    message(STATUS "No secrets file found - using environment variables only")
endif()

# Core Libraries

# Create DTC Protocol library (core)
add_library(dtc_protocol STATIC
    src/core/dtc/protocol.cpp
)

# Create utility library (core)
add_library(dtc_util STATIC
    src/core/util/log.cpp
)

# Create auth/credentials library (core)
add_library(dtc_auth STATIC
    src/core/auth/jwt_auth.cpp
    src/core/auth/credentials.cpp
)

# Create test utilities library (core)
add_library(dtc_test STATIC
    src/core/test/api_mock.cpp
)

# Create server library (core)
add_library(dtc_server STATIC
    src/core/server/server.cpp
    src/core/server/symbol_manager.cpp
)

# Exchange Libraries

# Create base exchange library
add_library(exchange_base STATIC
    src/exchanges/base/exchange_feed.cpp
)

# Create exchange factory library
add_library(exchange_factory STATIC
    src/exchanges/factory/exchange_factory.cpp
)

# Create Coinbase feed library
add_library(coinbase_feed STATIC
    src/exchanges/coinbase/coinbase_feed.cpp
)

# Create Binance feed library
add_library(binance_feed STATIC
    src/exchanges/binance/binance_feed.cpp
)

# Set include directories for all libraries
target_include_directories(dtc_protocol PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/settings>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(dtc_util PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(dtc_auth PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(dtc_test PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(dtc_server PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(exchange_base PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(exchange_factory PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(coinbase_feed PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(binance_feed PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link exchange dependencies
target_link_libraries(exchange_base dtc_util)
target_link_libraries(exchange_factory 
    exchange_base 
    coinbase_feed 
    binance_feed 
    dtc_util
)
target_link_libraries(coinbase_feed exchange_base dtc_util)
target_link_libraries(binance_feed exchange_base dtc_util)
target_link_libraries(dtc_auth dtc_util)
target_link_libraries(dtc_test dtc_util)
target_link_libraries(dtc_server 
    exchange_factory 
    exchange_base 
    dtc_protocol 
    dtc_auth
    dtc_util
)

# Main executable
add_executable(coinbase_dtc_server
    src/core/server/main.cpp
)

# Link libraries to main executable
target_link_libraries(coinbase_dtc_server PRIVATE
    dtc_protocol
    dtc_util
    dtc_auth
    dtc_test
    dtc_server
    exchange_factory
    exchange_base
    coinbase_feed
    binance_feed
)

# Link HTTP libraries if available (Windows/vcpkg + Linux)
message(STATUS "Checking for CURL... CURL_FOUND=${CURL_FOUND}")
if(CURL_FOUND)
    target_link_libraries(coinbase_feed PUBLIC CURL::libcurl)
    target_compile_definitions(coinbase_feed PUBLIC HAS_LIBCURL)
    message(STATUS "✅ Using libcurl for HTTP requests")
else()
    message(STATUS "❌ libcurl not found - will use system fallback")
endif()

if(nlohmann_json_FOUND)
    target_link_libraries(coinbase_feed PUBLIC nlohmann_json::nlohmann_json)
    target_link_libraries(binance_feed PUBLIC nlohmann_json::nlohmann_json)
    target_compile_definitions(coinbase_feed PUBLIC HAS_NLOHMANN_JSON)
    target_compile_definitions(binance_feed PUBLIC HAS_NLOHMANN_JSON)
    message(STATUS "✅ Using nlohmann_json for JSON parsing")
endif()

if(jwt-cpp_FOUND)
    target_link_libraries(coinbase_feed PUBLIC jwt-cpp::jwt-cpp)
    target_compile_definitions(coinbase_feed PUBLIC HAS_JWT_CPP)
    message(STATUS "✅ Using jwt-cpp for JWT authentication")
endif()

if(OpenSSL_FOUND)
    target_link_libraries(coinbase_feed PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    target_link_libraries(binance_feed PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(coinbase_feed PUBLIC HAS_OPENSSL)
    target_compile_definitions(binance_feed PUBLIC HAS_OPENSSL)
    message(STATUS "✅ Using OpenSSL for cryptographic operations")
endif()

# Windows-specific libraries
if(WIN32)
    target_link_libraries(coinbase_dtc_server PRIVATE ws2_32 wsock32)
endif()

# Enable testing
include(CTest)
if (BUILD_TESTING AND ENABLE_TESTING)
    
    # Core Tests
    add_executable(test_dtc_protocol
        tests/core/dtc/test_dtc_protocol.cpp
    )
        target_link_libraries(test_dtc_protocol dtc_protocol dtc_util)
    target_include_directories(test_dtc_protocol PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    add_executable(test_server
        tests/core/server/test_server.cpp
    )
    target_link_libraries(test_server
        dtc_server 
        dtc_protocol 
        dtc_util 
        exchange_factory
        exchange_base
    )
    target_include_directories(test_server PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    # Exchange Tests
    add_executable(test_coinbase
        tests/exchanges/coinbase/test_coinbase_feed.cpp
    )
    target_link_libraries(test_coinbase
        coinbase_feed 
        exchange_base 
        dtc_util
    )
    target_include_directories(test_coinbase PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    # Integration Tests
    add_executable(test_integration
        tests/integration/test_multi_exchange.cpp
    )
    target_link_libraries(test_integration
        dtc_protocol
        dtc_util
        dtc_server
        exchange_factory
        exchange_base
        coinbase_feed
        binance_feed
    )
    target_include_directories(test_integration PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    # DTC Integration Test (server + client communication)
    add_executable(test_dtc_integration
        tests/integration/test_dtc_integration.cpp
    )
    target_link_libraries(test_dtc_integration
        dtc_protocol
        dtc_util
    )
    target_include_directories(test_dtc_integration PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    # Windows-specific libraries for tests
    if(WIN32)
        target_link_libraries(test_dtc_protocol ws2_32 wsock32)
        target_link_libraries(test_server ws2_32 wsock32)
        target_link_libraries(test_coinbase ws2_32 wsock32)
        target_link_libraries(test_integration ws2_32 wsock32)
        target_link_libraries(test_dtc_integration ws2_32 wsock32)
    endif()
    
    # Add tests
    add_test(NAME DTCProtocolTest COMMAND test_dtc_protocol)
    add_test(NAME ServerTest COMMAND test_server)
    add_test(NAME CoinbaseTest COMMAND test_coinbase)
    add_test(NAME IntegrationTest COMMAND test_integration)
    
    # Add PowerShell server test (CI/CD friendly)
    if(WIN32)
        add_test(NAME ServerStartupTest 
                 COMMAND powershell -ExecutionPolicy Bypass -File "${CMAKE_CURRENT_SOURCE_DIR}/test-server.ps1"
                 WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
    
endif()

# Legacy compatibility - DTC Test Client executable
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_dtc_client.cpp")
    add_executable(test_dtc_client test_dtc_client.cpp)
    target_link_libraries(test_dtc_client dtc_protocol dtc_util)
    target_include_directories(test_dtc_client PRIVATE include)
    
    if(WIN32)
        target_link_libraries(test_dtc_client ws2_32)
    endif()
endif()

# Legacy compatibility - Debug sizes executable
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/debug_sizes.cpp")
    add_executable(debug_sizes debug_sizes.cpp)
    target_link_libraries(debug_sizes dtc_protocol dtc_util)
    target_include_directories(debug_sizes PRIVATE include)
endif()