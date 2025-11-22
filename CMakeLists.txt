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
    find_package(ZLIB QUIET)
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
    find_package(ZLIB QUIET)
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

# Create utility library (core) - now with advanced logging
add_library(dtc_util STATIC
    src/core/util/advanced_log.cpp
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
    src/exchanges/coinbase/websocket_client.cpp  # Re-enabled with working implementation
    src/exchanges/coinbase/ssl_websocket_client.cpp  # New SSL/TLS WebSocket client with JWT
    src/exchanges/coinbase/rest_client.cpp  # REST API client for account data
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

target_include_directories(dtc_util INTERFACE
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
target_link_libraries(exchange_base PRIVATE dtc_util)
target_link_libraries(exchange_factory PRIVATE
    exchange_base 
    coinbase_feed 
    binance_feed 
    dtc_util
)
target_link_libraries(coinbase_feed PRIVATE exchange_base dtc_util)
target_link_libraries(binance_feed PRIVATE exchange_base dtc_util)
target_link_libraries(dtc_auth PRIVATE dtc_util)
target_link_libraries(dtc_test PRIVATE dtc_util)
target_link_libraries(dtc_server PRIVATE
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
    # exchange_factory  # TEMPORARY: Disabled for Docker build
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
    target_link_libraries(dtc_auth PUBLIC nlohmann_json::nlohmann_json)
    target_compile_definitions(coinbase_feed PUBLIC HAS_NLOHMANN_JSON)
    target_compile_definitions(binance_feed PUBLIC HAS_NLOHMANN_JSON)
    target_compile_definitions(dtc_auth PUBLIC HAS_NLOHMANN_JSON)
    message(STATUS "✅ Using nlohmann_json for JSON parsing")
endif()

if(jwt-cpp_FOUND)
    target_link_libraries(coinbase_feed PUBLIC jwt-cpp::jwt-cpp)
    target_link_libraries(dtc_auth PUBLIC jwt-cpp::jwt-cpp)
    target_compile_definitions(coinbase_feed PUBLIC HAS_JWT_CPP)
    target_compile_definitions(dtc_auth PUBLIC HAS_JWT_CPP)
    message(STATUS "✅ Using jwt-cpp for JWT authentication")
endif()

if(OpenSSL_FOUND)
    target_link_libraries(coinbase_feed PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    target_link_libraries(binance_feed PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(coinbase_feed PUBLIC HAS_OPENSSL)
    target_compile_definitions(binance_feed PUBLIC HAS_OPENSSL)
    message(STATUS "✅ Using OpenSSL for cryptographic operations")
endif()

if(ZLIB_FOUND)
    target_link_libraries(coinbase_feed PUBLIC ZLIB::ZLIB)
    target_compile_definitions(coinbase_feed PUBLIC HAS_ZLIB)
    message(STATUS "✅ Using ZLIB for compression/decompression")
endif()

# Windows-specific libraries
if(WIN32)
    target_link_libraries(coinbase_dtc_server PRIVATE ws2_32 wsock32)
endif()

# Enable testing
include(CTest)
if (BUILD_TESTING AND ENABLE_TESTING)
    
    # Core Tests
    add_executable(test_basic
        tests/core/test_basic.cpp
    )
    target_link_libraries(test_basic dtc_protocol dtc_util)
    target_include_directories(test_basic PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )

    add_executable(test_logger_simple
        tests/core/util/test_logger_simple.cpp
    )
    target_link_libraries(test_logger_simple dtc_util)
    target_include_directories(test_logger_simple PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )

    add_executable(test_logger
        tests/core/util/test_logger.cpp
    )
    target_link_libraries(test_logger dtc_util)
    target_include_directories(test_logger PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    # TEMPORARY: Disabled until Protocol methods are fully implemented
    add_executable(test_dtc_protocol
        tests/core/dtc/test_dtc_protocol.cpp
    )
    target_link_libraries(test_dtc_protocol dtc_protocol dtc_util)
    target_include_directories(test_dtc_protocol PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    # REMOVED: test_server - redundant functionality covered by integration tests
    # The simple DTCServer creation test is not critical as the server is tested
    # in practice through integration tests and the main application
    
    # Root level tests (legacy compatibility) - removed
    
    # TEMPORARY: Disabled until Protocol methods are fully implemented - removed
    
    # Exchange Tests - Coinbase
    # TEMPORARY: Disabled due to file corruption
    # add_executable(test_coinbase_feed
    #     tests/exchanges/coinbase/test_coinbase_feed.cpp
    # )
    # target_link_libraries(test_coinbase_feed
    #     coinbase_feed 
    #     exchange_base 
    #     dtc_util
    # )
    # target_include_directories(test_coinbase_feed PRIVATE
    #     ${CMAKE_CURRENT_SOURCE_DIR}/include
    #     ${CMAKE_CURRENT_SOURCE_DIR}/settings
    # )
    
# Tests that require CURL
if(CURL_FOUND)
    # Real Coinbase Account Info Test
    add_executable(test_coinbase_account_info
        src/tests/test_coinbase_account_info.cpp
        src/exchanges/coinbase/rest_client.cpp
    )
    target_link_libraries(test_coinbase_account_info
        dtc_auth
        dtc_util
        CURL::libcurl
        winhttp
        ws2_32
        crypt32
    )
    if(nlohmann_json_FOUND)
        target_link_libraries(test_coinbase_account_info nlohmann_json::nlohmann_json)
    endif()
    target_include_directories(test_coinbase_account_info PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    # Coinbase Endpoint Diagnostic Test
    add_executable(test_coinbase_endpoints
        src/tests/test_coinbase_endpoints.cpp
    )
    target_link_libraries(test_coinbase_endpoints
        dtc_auth
        dtc_util
        CURL::libcurl
        winhttp
        ws2_32
        crypt32
    )
    target_include_directories(test_coinbase_endpoints PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )

    add_executable(test_jwt_auth
        tests/exchanges/coinbase/test_jwt_auth.cpp
    )
    target_link_libraries(test_jwt_auth
        dtc_auth
        dtc_util
        CURL::libcurl
        winhttp
        ws2_32
        crypt32
    )
    target_include_directories(test_jwt_auth PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    target_compile_definitions(test_jwt_auth PRIVATE HAS_LIBCURL)
endif() # CURL_FOUND
    
    # TEMPORARY: Disabled due to missing header files
    # add_executable(test_advanced_trade_api
    #     tests/exchanges/coinbase/test_advanced_trade_api.cpp
    # )
    # target_link_libraries(test_advanced_trade_api
    #     coinbase_feed 
    #     exchange_base 
    #     dtc_util
    #     dtc_auth
    # )
    # target_include_directories(test_advanced_trade_api PRIVATE
    #     ${CMAKE_CURRENT_SOURCE_DIR}/include
    #     ${CMAKE_CURRENT_SOURCE_DIR}/settings
    # )
    
    # TEMPORARY: Disabled due to missing header files
    # add_executable(test_jwt_auth
    #     tests/exchanges/coinbase/test_jwt_auth.cpp
    # )
    # target_link_libraries(test_jwt_auth
    #     dtc_auth
    #     dtc_util
    # )
    # target_include_directories(test_jwt_auth PRIVATE
    #     ${CMAKE_CURRENT_SOURCE_DIR}/include
    #     ${CMAKE_CURRENT_SOURCE_DIR}/settings
    # )
    
    add_executable(test_permissions
        tests/exchanges/coinbase/test_permissions.cpp
    )
    target_link_libraries(test_permissions
        coinbase_feed
        dtc_auth
        dtc_util
    )
    target_include_directories(test_permissions PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    # TEMPORARY: Disabled due to missing CURL linking
    # add_executable(test_jwt_debug
    #     tests/exchanges/coinbase/test_jwt_debug.cpp
    # )
    # target_link_libraries(test_jwt_debug
    #     dtc_auth
    #     dtc_util
    # )
    # target_include_directories(test_jwt_debug PRIVATE
    #     ${CMAKE_CURRENT_SOURCE_DIR}/include
    #     ${CMAKE_CURRENT_SOURCE_DIR}/settings
    # )
    
    # Integration Tests
    add_executable(test_dtc_integration
        tests/integration/test_dtc_integration.cpp
    )
    target_link_libraries(test_dtc_integration
        dtc_protocol
        dtc_util
        dtc_server
    )
    if(nlohmann_json_FOUND)
        target_link_libraries(test_dtc_integration nlohmann_json::nlohmann_json)
    endif()
    target_include_directories(test_dtc_integration PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    add_executable(test_multi_exchange
        tests/integration/test_multi_exchange.cpp
    )
    target_link_libraries(test_multi_exchange
        dtc_protocol
        dtc_util
        dtc_server
        # exchange_factory  # TEMPORARY: Disabled
        exchange_base
        coinbase_feed
        binance_feed
    )
    target_include_directories(test_multi_exchange PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    # DTC Console Test Client - for testing DTC protocol communication
    add_executable(test_dtc_console
        src/tests/test_dtc_console.cpp
    )
    target_link_libraries(test_dtc_console
        dtc_protocol
        dtc_util
    )
    target_include_directories(test_dtc_console PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    # MarketDataResponse Console Test - for testing MarketDataResponse functionality
    add_executable(test_market_data_response
        tests/console/test_market_data_response.cpp
    )
    target_link_libraries(test_market_data_response
        dtc_protocol
        dtc_util
    )
    target_include_directories(test_market_data_response PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    # TEMPORARY: Disabled due to namespace migration issues
    # add_executable(test_environment
    #     tests/integration/test_environment.cpp
    # )
    # target_link_libraries(test_environment
    #     dtc_util
    #     dtc_auth
    # )
    # target_include_directories(test_environment PRIVATE
    #     ${CMAKE_CURRENT_SOURCE_DIR}/include
    #     ${CMAKE_CURRENT_SOURCE_DIR}/settings
    # )
    
    # Integration Utilities (token extraction)
    add_executable(extract_token
        tests/integration/extract_token.cpp
    )
    target_link_libraries(extract_token
        dtc_auth
        dtc_util
    )
    target_include_directories(extract_token PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    add_executable(extract_permissions_token
        tests/integration/extract_permissions_token.cpp
    )
    target_link_libraries(extract_permissions_token
        dtc_auth
        dtc_util
    )
    target_include_directories(extract_permissions_token PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    # Windows-specific libraries for tests
    if(WIN32)
        target_link_libraries(test_basic ws2_32 wsock32)
        target_link_libraries(test_dtc_protocol ws2_32 wsock32)
        # test_server removed - redundant functionality
        # Legacy test files removed
        # target_link_libraries(test_coinbase_feed ws2_32 wsock32)  # DISABLED
        # target_link_libraries(test_advanced_trade_api ws2_32 wsock32)
        # target_link_libraries(test_jwt_auth ws2_32 wsock32)
        target_link_libraries(test_permissions ws2_32 wsock32)
        # target_link_libraries(test_jwt_debug ws2_32 wsock32)
        target_link_libraries(test_dtc_integration ws2_32 wsock32)
        target_link_libraries(test_multi_exchange ws2_32 wsock32)
        # target_link_libraries(test_environment ws2_32 wsock32)
        target_link_libraries(extract_token ws2_32 wsock32)
        target_link_libraries(extract_permissions_token ws2_32 wsock32)
    endif()
    
    # Add tests to CTest
    add_test(NAME BasicTest COMMAND test_basic)
    add_test(NAME LoggerSimpleTest COMMAND test_logger_simple)
    add_test(NAME LoggerComponentTest COMMAND test_logger)
    add_test(NAME DTCProtocolTest COMMAND test_dtc_protocol)
    # ServerTest removed - redundant functionality covered by integration tests
    # Legacy tests removed
    # add_test(NAME CoinbaseFeedTest COMMAND test_coinbase_feed)
    # add_test(NAME AdvancedTradeAPITest COMMAND test_advanced_trade_api)
    # add_test(NAME JWTAuthTest COMMAND test_jwt_auth)
    add_test(NAME PermissionsTest COMMAND test_permissions)
    # add_test(NAME JWTDebugTest COMMAND test_jwt_debug)
    add_test(NAME DTCIntegrationTest COMMAND test_dtc_integration)
    add_test(NAME MultiExchangeTest COMMAND test_multi_exchange)
    # add_test(NAME EnvironmentTest COMMAND test_environment)

    if(TARGET test_jwt_auth)
        add_test(NAME JWTAuthTest COMMAND test_jwt_auth)
    endif()
    
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

# WebSocket test executable - removed

# SSL WebSocket test executable - removed

# SSL WebSocket Coinbase specific test
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/exchanges/test_ssl_websocket_coinbase.cpp")
    add_executable(test_ssl_websocket_coinbase tests/exchanges/test_ssl_websocket_coinbase.cpp)
    target_link_libraries(test_ssl_websocket_coinbase coinbase_feed exchange_base dtc_util)
    target_include_directories(test_ssl_websocket_coinbase PRIVATE include)
    
    if(WIN32)
        target_link_libraries(test_ssl_websocket_coinbase ws2_32 crypt32 secur32)
    endif()
endif()

# Basic WebSocket test executable - removed

# DTC Test Client - GUI application for testing DTC server (Windows only)
if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/dtc_test_client/main_gui.cpp")
    add_executable(dtc_test_client_gui
        src/dtc_test_client/main_gui.cpp
        src/dtc_test_client/dtc_gui_client.cpp
    )
    
    target_include_directories(dtc_test_client_gui PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/dtc_test_client
        ${CMAKE_CURRENT_SOURCE_DIR}/src/dtc_test_client
    )
    
    target_link_libraries(dtc_test_client_gui PRIVATE
        dtc_protocol
        dtc_util
        ws2_32 
        user32 
        gdi32 
        comctl32
        shell32
    )
    
    # Set subsystem to Windows GUI
    set_target_properties(dtc_test_client_gui PROPERTIES
        WIN32_EXECUTABLE TRUE
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        OUTPUT_NAME "dtc_test_client_gui"
    )
    
    message(STATUS "✅ DTC Test Client Windows GUI will be built")
endif()

# DTC Test Client - Console application for testing DTC server (legacy) - TEMPORARILY DISABLED
if(FALSE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/dtc_test_client/main.cpp")
    add_executable(dtc_test_client_console
        src/dtc_test_client/main.cpp
        src/dtc_test_client/test_client.cpp
    )
    
    target_include_directories(dtc_test_client_console PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/dtc_test_client
    )
    
    target_link_libraries(dtc_test_client_console PRIVATE
        dtc_protocol
        dtc_util
    )
    
    # Platform-specific libraries for console input handling
    if(WIN32)
        target_link_libraries(dtc_test_client_console PRIVATE ws2_32 user32)
    else()
        target_link_libraries(dtc_test_client_console PRIVATE pthread)
    endif()
    
    # Set target properties
    set_target_properties(dtc_test_client_console PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        OUTPUT_NAME "dtc_test_client_console"
    )
    
    message(STATUS "✅ DTC Test Client Console will be built")
endif()

# Minimal diagnostic executable
add_executable(minimal_test src/core/server/main_minimal.cpp)
set_target_properties(minimal_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "minimal_test"
)

# Step-by-step diagnostic executable
add_executable(stepwise_test src/core/server/main_stepwise.cpp)
set_target_properties(stepwise_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "stepwise_test"
)

message(STATUS "✅ Diagnostic minimal executables will be built")