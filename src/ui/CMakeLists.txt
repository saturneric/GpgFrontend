aux_source_directory(. UI_SOURCE)
aux_source_directory(./aes UI_SOURCE)
aux_source_directory(./keypair_details UI_SOURCE)
aux_source_directory(./widgets UI_SOURCE)
aux_source_directory(./keygen UI_SOURCE)
aux_source_directory(./main_window UI_SOURCE)
aux_source_directory(./help UI_SOURCE)
aux_source_directory(./settings UI_SOURCE)
aux_source_directory(./function UI_SOURCE)
aux_source_directory(./details UI_SOURCE)
aux_source_directory(./data_struct UI_SOURCE)

if (SMTP_SUPPORT)
    aux_source_directory(./smtp UI_SOURCE)
endif ()

add_library(gpgfrontend-ui STATIC ${UI_SOURCE})
set(GPGFRONTEND_UI_LIB_NAME gpgfrontend-ui)
target_link_libraries(${GPGFRONTEND_UI_LIB_NAME}
        Qt5::Network Qt5::PrintSupport Qt5::Widgets Qt5::Test Qt5::Core)
target_include_directories(gpgfrontend-ui PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/${GPGFRONTEND_UI_LIB_NAME}_autogen/include)

if (XCODE_BUILD)
    set_target_properties(gpgfrontend-ui
            PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
endif ()
target_compile_features(gpgfrontend-ui PUBLIC cxx_std_17)