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

add_library(${CASSANDRA_ROOT_PROJECT_NAME} STATIC ${SOURCES})

add_library(userver::cassandra ALIAS ${CASSANDRA_ROOT_PROJECT_NAME})

target_link_libraries(${CASSANDRA_ROOT_PROJECT_NAME} PUBLIC userver::core lz4)

target_include_directories(
  ${CASSANDRA_ROOT_PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                                        $<INSTALL_INTERFACE:include>
)
include_directories(src)

# Install
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(
  TARGETS ${CASSANDRA_ROOT_PROJECT_NAME}
  EXPORT ${CASSANDRA_ROOT_PROJECT_NAME}-targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(
  EXPORT ${CASSANDRA_ROOT_PROJECT_NAME}-targets
  FILE ${CASSANDRA_ROOT_PROJECT_NAME}Targets.cmake
  NAMESPACE userver::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${CASSANDRA_ROOT_PROJECT_NAME}
)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${CASSANDRA_ROOT_PROJECT_NAME}Config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${CASSANDRA_ROOT_PROJECT_NAME}Config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${CASSANDRA_ROOT_PROJECT_NAME}
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${CASSANDRA_ROOT_PROJECT_NAME}Config.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${CASSANDRA_ROOT_PROJECT_NAME}
)

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
