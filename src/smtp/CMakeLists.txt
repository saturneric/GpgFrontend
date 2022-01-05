aux_source_directory(. SMTP_MIME_SOURCE)

add_library(smtp STATIC ${SMTP_MIME_SOURCE})

target_link_libraries(smtp
        Qt5::Network Qt5::Core)

if(XCODE_BUILD)
set_target_properties(smtp 
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
endif()