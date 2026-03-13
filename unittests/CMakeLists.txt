

# Unit Tests
add_executable(${PROJECT_NAME}_unittest io_test.cpp compress_test.cpp)

target_link_libraries(${PROJECT_NAME}_unittest PRIVATE userver::cassandra userver::utest)
target_include_directories(${PROJECT_NAME}_unittest PRIVATE ../include)
add_google_tests(${PROJECT_NAME}_unittest)


# Benchmarks
# add_executable(${PROJECT_NAME}_benchmark)
# target_link_libraries(${PROJECT_NAME}_benchmark PRIVATE ${PROJECT_NAME}_objs userver::ubench)
# add_google_benchmark_tests(${PROJECT_NAME}_benchmark)