
set(CMAKE_PROJECT_DESCRIPTION "abel c++ lib")
set(CMAKE_PROJECT_VERSION_MAJOR 0)
set(CMAKE_PROJECT_VERSION_MINOR 1)
set(CMAKE_PROJECT_VERSION_PATCH 0)
set(CMAKE_PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

option(ABEL_STATUS_PRINT "cmake toolchain print" ON)
option(ABEL_STATUS_DEBUG "cmake toolchain debug info" ON)

option(ENABLE_TESTING "enable unit test" ON)
option(ABEL_PACKAGE_GEN "enable package gen" ON)
option(ENABLE_BENCHMARK "enable benchmark" ON)


set(CMAKE_BUILD_TYPE Debug)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
