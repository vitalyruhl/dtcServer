cmake_minimum_required(VERSION 3.16)
project(coinbase-dtc-core VERSION 0.1.0 LANGUAGES CXX)

option(ENABLE_TESTING "Enable building tests" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Check if secrets file exists (for local development)
if(EXISTS "${CMAKE_SOURCE_DIR}/secrets/coinbase.h")
    message(STATUS "Found secrets file - enabling secrets support")
    add_compile_definitions(INCLUDE_SECRETS_FILE)
else()
    message(STATUS "No secrets file found - using environment variables only")
endif()

add_library(coinbase_dtc_core STATIC
    src/dtc_protocol/protocol.cpp
    src/coinbase_feed/feed.cpp
    src/server/server.cpp
    src/util/log.cpp
    src/test/api_mock.cpp
    src/credentials/cdp_credentials.cpp
)

target_include_directories(coinbase_dtc_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/settings>
    $<INSTALL_INTERFACE:include>
)

add_executable(coinbase_dtc_server
    src/server/main.cpp
)

target_link_libraries(coinbase_dtc_server PRIVATE coinbase_dtc_core)

include(CTest)
if (BUILD_TESTING AND ENABLE_TESTING)
    add_executable(coinbase_dtc_core_tests
        tests/test_basic.cpp
    )
    target_link_libraries(coinbase_dtc_core_tests PRIVATE coinbase_dtc_core)
    
    # Ensure tests can find all headers
    target_include_directories(coinbase_dtc_core_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    add_test(NAME basic COMMAND coinbase_dtc_core_tests)
endif()
