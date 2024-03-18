cmake_minimum_required(VERSION 3.25)
project(qr LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 20)
set(CMAKE_CURRENT_SOURCE_DIR src)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

# Main executable
add_executable(qr-bin src/main.cc src/encoder.cc)
target_include_directories(qr-bin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps/utfcpp)
if (CMAKE_COMPILER_IS_GNUCC)
  target_compile_options(main PRIVATE -Wall -Wextra -Wunintialized -pedantic)
endif()

# Test executable
enable_testing()
add_executable(qr-tests src/tests.cc src/encoder.cc)
target_link_libraries(qr-tests GTest::gtest_main)
target_include_directories(qr-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps/utfcpp)
gtest_discover_tests(qr-tests)