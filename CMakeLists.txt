set(CMAKE_POLICY_DEFAULT_CMP0079 NEW)

set(CASSANDRA_ROOT_PROJECT_NAME userver-cql-driver)

cmake_minimum_required(VERSION 3.12)
project(${CASSANDRA_ROOT_PROJECT_NAME} CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)

# Adding userver dependency
find_package(
  userver
  COMPONENTS core
  REQUIRED
)
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
list(FILTER SOURCES EXCLUDE REGEX ".*(_test|_benchmark)\\.cpp$")

add_library(
  ${CASSANDRA_ROOT_PROJECT_NAME} STATIC ${SOURCES})

add_library(userver::cassandra ALIAS ${CASSANDRA_ROOT_PROJECT_NAME})

target_link_libraries(${CASSANDRA_ROOT_PROJECT_NAME} PUBLIC userver::core lz4)

target_include_directories(${CASSANDRA_ROOT_PROJECT_NAME}
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(src)
option(CASSANDRA_BUILD_SAMPLES "Build userver samples" OFF)
option(CASSANDRA_BUILD_TESTS "Build userver tests" ON)

if(CASSANDRA_BUILD_SAMPLES)
    add_subdirectory(samples/example_service)
endif()


if(CASSANDRA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(unittests)
    add_subdirectory(benchmarks)
endif()
