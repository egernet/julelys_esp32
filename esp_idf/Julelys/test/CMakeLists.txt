cmake_minimum_required(VERSION 3.10)
project(led_controller_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add mocks directory to include path (before system includes)
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/mocks)

# Add parent components directory for the actual source
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../components/led_controller)

# Test executable
add_executable(test_led_controller
    test_led_controller.cpp
)

# Enable testing
enable_testing()
add_test(NAME led_controller_tests COMMAND test_led_controller)
