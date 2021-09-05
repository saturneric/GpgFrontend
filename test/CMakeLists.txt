enable_testing()

find_package(GTest REQUIRED)

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