cmake_minimum_required(VERSION 3.16)
project(coinbase-dtc-core VERSION 0.1.0 LANGUAGES CXX)

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

add_library(coinbase_dtc_core STATIC
    src/dtc_protocol/protocol.cpp
    src/coinbase_feed/feed.cpp
    src/server/server.cpp
    src/util/log.cpp
    src/test/api_mock.cpp
    src/credentials/cdp_credentials.cpp
    src/auth/jwt_auth.cpp
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

# Link HTTP libraries if available (Windows/vcpkg + Linux)
message(STATUS "Checking for CURL... CURL_FOUND=${CURL_FOUND}")
if(CURL_FOUND)
    target_link_libraries(coinbase_dtc_core PUBLIC CURL::libcurl)
    target_compile_definitions(coinbase_dtc_core PUBLIC HAS_LIBCURL)
    message(STATUS "✅ Using libcurl for HTTP requests")
else()
    message(STATUS "❌ libcurl not found - will use system fallback")
endif()

if(nlohmann_json_FOUND)
    target_link_libraries(coinbase_dtc_core PUBLIC nlohmann_json::nlohmann_json)
    target_compile_definitions(coinbase_dtc_core PUBLIC HAS_NLOHMANN_JSON)
    message(STATUS "✅ Using nlohmann_json for JSON parsing")
endif()

if(jwt-cpp_FOUND)
    target_link_libraries(coinbase_dtc_core PUBLIC jwt-cpp::jwt-cpp)
    target_compile_definitions(coinbase_dtc_core PUBLIC HAS_JWT_CPP)
    message(STATUS "✅ Using jwt-cpp for JWT authentication")
endif()

if(OpenSSL_FOUND)
    target_link_libraries(coinbase_dtc_core PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(coinbase_dtc_core PUBLIC HAS_OPENSSL)
    message(STATUS "✅ Using OpenSSL for cryptographic operations")
endif()

include(CTest)
if (BUILD_TESTING AND ENABLE_TESTING)
    # Basic functionality tests
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
    
    # Advanced Trade API integration test
    add_executable(test_advanced_trade_api
        tests/test_advanced_trade_api.cpp
    )
    target_link_libraries(test_advanced_trade_api PRIVATE coinbase_dtc_core)
    
    target_include_directories(test_advanced_trade_api PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    # JWT Authentication test
    add_executable(test_jwt_auth
        tests/test_jwt_auth.cpp
    )
    target_link_libraries(test_jwt_auth PRIVATE coinbase_dtc_core)
    
    target_include_directories(test_jwt_auth PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    # API Permissions test
    add_executable(test_permissions
        tests/test_permissions.cpp
    )
    target_link_libraries(test_permissions PRIVATE coinbase_dtc_core)
    
    target_include_directories(test_permissions PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    # JWT Debug test  
    add_executable(test_jwt_debug
        tests/test_jwt_debug.cpp
    )
    target_link_libraries(test_jwt_debug PRIVATE coinbase_dtc_core)
    
    target_include_directories(test_jwt_debug PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    # View Permissions test  
    add_executable(test_view_permissions
        tests/test_view_permissions.cpp
    )
    target_link_libraries(test_view_permissions PRIVATE coinbase_dtc_core)
    
    target_include_directories(test_view_permissions PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    # Simple Auth test  
    add_executable(test_simple_auth
        tests/test_simple_auth.cpp
    )
    target_link_libraries(test_simple_auth PRIVATE coinbase_dtc_core)
    
    target_include_directories(test_simple_auth PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    # Environment test  
    add_executable(test_environment
        tests/test_environment.cpp
    )
    target_link_libraries(test_environment PRIVATE coinbase_dtc_core)
    
    target_include_directories(test_environment PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    # Token extractor  
    add_executable(extract_token
        tests/extract_token.cpp
    )
    target_link_libraries(extract_token PRIVATE coinbase_dtc_core)
    
    target_include_directories(extract_token PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    # JWT decoder  
    add_executable(decode_jwt
        tests/decode_jwt.cpp
    )
    target_link_libraries(decode_jwt PRIVATE coinbase_dtc_core)
    
    target_include_directories(decode_jwt PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    # Extract permissions token
    add_executable(extract_permissions_token
        tests/extract_permissions_token.cpp
    )
    target_link_libraries(extract_permissions_token PRIVATE coinbase_dtc_core)
    
    target_include_directories(extract_permissions_token PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
    )
    
    add_test(NAME advanced_trade_api COMMAND test_advanced_trade_api)
    add_test(NAME jwt_auth COMMAND test_jwt_auth)
    add_test(NAME permissions COMMAND test_permissions)
    add_test(NAME jwt_debug COMMAND test_jwt_debug)
endif()
