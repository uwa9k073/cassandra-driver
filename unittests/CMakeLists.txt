# Unit Tests
add_executable(${CASSANDRA_ROOT_PROJECT_NAME}_unittest list_types_test.cpp
                                                       optional_values_test.cpp
                                                       scalar_types_test.cpp
                                                       compress_test.cpp
                                                       map_types_test.cpp
                                                       params_test.cpp
                                                       value_and_uuid_test.cpp)

target_link_libraries(${CASSANDRA_ROOT_PROJECT_NAME}_unittest PRIVATE userver::cassandra userver::utest)
target_include_directories(${CASSANDRA_ROOT_PROJECT_NAME}_unittest PRIVATE
                           ../include)
add_google_tests(${CASSANDRA_ROOT_PROJECT_NAME}_unittest)
