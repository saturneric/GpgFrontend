# tracking source files
aux_source_directory(. UI_SOURCE)
aux_source_directory(./keypair_details UI_SOURCE)
aux_source_directory(./widgets UI_SOURCE)
aux_source_directory(./keygen UI_SOURCE)
aux_source_directory(./main_window UI_SOURCE)
aux_source_directory(./help UI_SOURCE)
aux_source_directory(./settings UI_SOURCE)
aux_source_directory(./thread UI_SOURCE)
aux_source_directory(./details UI_SOURCE)
aux_source_directory(./data_struct UI_SOURCE)
aux_source_directory(./encoding UI_SOURCE)

if (SMTP_SUPPORT)
    aux_source_directory(./smtp UI_SOURCE)
endif ()

add_library(gpgfrontend_ui STATIC ${UI_SOURCE})
set(GPGFRONTEND_UI_LIB_NAME gpgfrontend_ui)

# link smtp-mime
if (SMTP_SUPPORT)
    target_link_libraries(${GPGFRONTEND_UI_LIB_NAME}
            smtp-mime)
endif ()
# link json
target_link_libraries(${GPGFRONTEND_UI_LIB_NAME}
        nlohmann_json::nlohmann_json)
# Qt AES
target_link_libraries(${GPGFRONTEND_UI_LIB_NAME}
        QtAES)
# link Qt
target_link_libraries(${GPGFRONTEND_UI_LIB_NAME}
        Qt5::Network Qt5::PrintSupport Qt5::Widgets Qt5::Test Qt5::Core)
# link vmime
target_link_libraries(${GPGFRONTEND_UI_LIB_NAME}
        vmime-static anl ssl crypto)
# link easyloggingpp
target_link_libraries(${GPGFRONTEND_UI_LIB_NAME}
        easyloggingpp)

# link gpgfrontend_core
target_link_libraries(${GPGFRONTEND_UI_LIB_NAME}
        gpgfrontend_core)

target_include_directories(gpgfrontend_ui PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/${GPGFRONTEND_UI_LIB_NAME}_autogen/include)

if (XCODE_BUILD)
    set_target_properties(${GPGFRONTEND_UI_LIB_NAME}
            PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
endif ()
target_compile_features(gpgfrontend_ui PUBLIC cxx_std_17)