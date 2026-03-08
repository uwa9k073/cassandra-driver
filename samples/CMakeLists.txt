project(cassandra-samples)

add_custom_target(${PROJECT_NAME})
# Imitate an installed userver in samples.
file(WRITE "${CMAKE_BINARY_DIR}/package_stubs/userverConfig.cmake" )
add_subdirectory(example_service)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-example_service)
