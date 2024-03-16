cmake_minimum_required(VERSION 3.25)
project(qr LANGUAGES CXX)
set(CMAKE_CURRENT_SOURCE_DIR src)

# Main executable
add_executable(qr-bin src/main.cc)
target_include_directories(qr-bin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps/utfcpp)

# Test executable
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(qr-tests src/tests.cc)
target_link_libraries(qr-tests GTest::gtest_main)
target_include_directories(qr-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps/utfcpp)

include(GoogleTest)
gtest_discover_tests(qr-tests)