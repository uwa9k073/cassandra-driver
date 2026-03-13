# Benchmarks
add_executable(${CASSANDRA_ROOT_PROJECT_NAME}_benchmark compress_bench.cpp)
target_link_libraries(${CASSANDRA_ROOT_PROJECT_NAME}_benchmark PRIVATE
                      userver::cassandra userver::ubench)
target_include_directories(${CASSANDRA_ROOT_PROJECT_NAME}_benchmark PRIVATE
                           ../include)
add_google_benchmark_tests(${CASSANDRA_ROOT_PROJECT_NAME}_benchmark)
