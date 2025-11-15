# CMake configuration for DTC Test Client
# This creates a simple test GUI for validating the DTC server functionality

# Add the test client executable
add_executable(dtc_test_client
    dtc_test_client/main.cpp
    dtc_test_client/test_client.cpp
)

# Set include directories
target_include_directories(dtc_test_client PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/dtc_test_client
)

# Link required libraries
target_link_libraries(dtc_test_client PRIVATE
    dtc_protocol
)

# Platform-specific libraries for console input handling
if(WIN32)
    # Windows-specific libraries
    target_link_libraries(dtc_test_client PRIVATE
        ws2_32
        user32
    )
else()
    # Unix/Linux libraries  
    target_link_libraries(dtc_test_client PRIVATE
        pthread
    )
endif()

# Set C++ standard
target_compile_features(dtc_test_client PRIVATE cxx_std_17)

# Compiler-specific options
if(MSVC)
    target_compile_options(dtc_test_client PRIVATE
        /W4
        /WX-  # Don't treat warnings as errors for test client
    )
else()
    target_compile_options(dtc_test_client PRIVATE
        -Wall
        -Wextra
        -Wno-unused-parameter
    )
endif()

# Set target properties
set_target_properties(dtc_test_client PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "dtc_test_client"
)

# Install the test client binary
install(TARGETS dtc_test_client
    RUNTIME DESTINATION bin
    COMPONENT TestTools
)