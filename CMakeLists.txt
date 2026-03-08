cmake_minimum_required(VERSION 3.12)
project(userver-cql-driver CXX)

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
  ${PROJECT_NAME} STATIC ${SOURCES})

add_library(userver::cassandra ALIAS userver-cql-driver)

target_link_libraries(${PROJECT_NAME} PUBLIC userver::core)

target_include_directories(${PROJECT_NAME}
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

option(CASSANDRA_BUILD_SAMPLES "Build userver samples" OFF)

if(CASSANDRA_BUILD_SAMPLES)
    add_subdirectory(samples)
endif()