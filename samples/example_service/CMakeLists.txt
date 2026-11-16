add_executable(${CASSANDRA_ROOT_PROJECT_NAME}_example-service main.cpp)

target_link_libraries(
    ${CASSANDRA_ROOT_PROJECT_NAME}_example-service
    PRIVATE userver::core userver::cassandra
)

# The Service
target_include_directories(${CASSANDRA_ROOT_PROJECT_NAME}_example-service PRIVATE
                           ../../include)


userver_testsuite_add(SERVICE_TARGET
                      ${CASSANDRA_ROOT_PROJECT_NAME}_example-service
                      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
                      PYTHONPATH ${userver-cql-driver_TESTSUITE_DIRS}
        # CMake автоматически установит пакеты из этого файла в venv тестсьюта
                      REQUIREMENTS ${userver-cql-driver_TESTSUITE_REQUIREMENTS}
                      PYTEST_ARGS
                      --service-config=${CMAKE_CURRENT_SOURCE_DIR}/configs/static_config.yaml
                      --service-config-vars=${CMAKE_CURRENT_SOURCE_DIR}/configs/config_vars.testing.yaml
                      --service-source-dir=${CMAKE_CURRENT_SOURCE_DIR}
                      --service-binary=${CMAKE_CURRENT_BINARY_DIR}/${CASSANDRA_ROOT_PROJECT_NAME}_example-service)
