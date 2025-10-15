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

# Common sources
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")

list(FILTER SOURCES EXCLUDE REGEX ".*(_test|_benchmark)\\.cpp$")
list(FILTER SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

add_library(${PROJECT_NAME}_objs OBJECT ${SOURCES})

target_link_libraries(${PROJECT_NAME}_objs PUBLIC userver::core)

target_include_directories(${PROJECT_NAME}_objs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# The Service
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)

# Unit Tests
add_executable(${PROJECT_NAME}_unittest src/utils/generate_salt_test.cpp)

target_link_libraries(${PROJECT_NAME}_unittest PRIVATE ${PROJECT_NAME}_objs userver::utest)
add_google_tests(${PROJECT_NAME}_unittest)

# Benchmarks
add_executable(${PROJECT_NAME}_benchmark)
target_link_libraries(${PROJECT_NAME}_benchmark PRIVATE ${PROJECT_NAME}_objs userver::ubench)
add_google_benchmark_tests(${PROJECT_NAME}_benchmark)

# Functional Tests
userver_testsuite_add_simple(REQUIREMENTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/requirements.txt")
