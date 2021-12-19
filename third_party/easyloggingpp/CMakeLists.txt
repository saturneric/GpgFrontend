aux_source_directory(. EASY_LOGGING_CPP_SOURCE)

add_library(easy_logging_pp STATIC ${EASY_LOGGING_CPP_SOURCE})
target_link_libraries(easy_logging_pp stdc++)
if(XCODE_BUILD)
set_target_properties(easy_logging_pp 
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
endif()