find_package(GTest REQUIRED)

# Set configure for test
file(COPY ${CMAKE_SOURCE_DIR}/test/conf DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/ FOLLOW_SYMLINK_CHAIN)
file(COPY ${CMAKE_SOURCE_DIR}/test/data DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/ FOLLOW_SYMLINK_CHAIN)

aux_source_directory(. TEST_SOURCE)

add_executable(
        ${AppName}
        ${TEST_SOURCE}
)

if(GPG_CORE)
    target_link_libraries(${AppName} gpg_core)
endif()

target_link_libraries(${AppName} gtest gtest_main)

add_test(AllTestsInGpgFrontend ${AppName})
