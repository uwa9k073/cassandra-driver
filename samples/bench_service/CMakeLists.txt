add_executable(${CASSANDRA_ROOT_PROJECT_NAME}_bench-service main.cpp)

target_link_libraries(
    ${CASSANDRA_ROOT_PROJECT_NAME}_bench-service
    PRIVATE userver::core userver::cassandra
)

# The Service
target_include_directories(${CASSANDRA_ROOT_PROJECT_NAME}_bench-service PRIVATE
                           ../../include)
