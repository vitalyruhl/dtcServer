cmake_minimum_required(VERSION 3.16)
project(coinbase-dtc-core VERSION 0.1.0 LANGUAGES CXX)

option(ENABLE_TESTING "Enable building tests" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(coinbase_dtc_core STATIC
    src/dtc_protocol/protocol.cpp
    src/coinbase_feed/feed.cpp
    src/server/server.cpp
    src/util/log.cpp
)

target_include_directories(coinbase_dtc_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
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
    add_test(NAME basic COMMAND coinbase_dtc_core_tests)
endif()
