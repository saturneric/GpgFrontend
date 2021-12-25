find_package(GTest REQUIRED)

# Set configure for test
file(COPY ${CMAKE_SOURCE_DIR}/test/conf DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/ FOLLOW_SYMLINK_CHAIN)
file(COPY ${CMAKE_SOURCE_DIR}/test/data DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/ FOLLOW_SYMLINK_CHAIN)

aux_source_directory(. TEST_SOURCE)

add_executable(
        ${AppName}
        ${TEST_SOURCE}
)

if (GPG_CORE)
    target_link_libraries(${AppName} gpg_core)
endif ()

if (GPG_STANDALONE_MODE)
    file(COPY ${CMAKE_SOURCE_DIR}/test/gpg DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/ FOLLOW_SYMLINK_CHAIN)
endif ()

target_link_libraries(${AppName} ${Boost_LIBRARIES} gtest gtest_main)
if (APPLE)
    target_link_libraries(${AppName} intl)
endif ()

add_test(AllTestsInGpgFrontend ${AppName})
