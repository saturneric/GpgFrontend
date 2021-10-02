aux_source_directory(. UI_SOURCE)
aux_source_directory(./keypair_details UI_SOURCE)
aux_source_directory(./widgets UI_SOURCE)
aux_source_directory(./keygen UI_SOURCE)
aux_source_directory(./main_window UI_SOURCE)
aux_source_directory(./help UI_SOURCE)
aux_source_directory(./settings UI_SOURCE)

add_library(gpgfrontend-ui STATIC ${UI_SOURCE} GpgFrontendUI.h)

target_link_libraries(gpgfrontend-ui
        Qt5::Network Qt5::PrintSupport Qt5::Widgets Qt5::Test Qt5::Core)