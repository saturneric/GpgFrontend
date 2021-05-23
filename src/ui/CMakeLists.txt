aux_source_directory(. QTUI_SOURCE)
aux_source_directory(./keypair_details QTUI_SOURCE)
aux_source_directory(./widgets QTUI_SOURCE)

add_library(qtui STATIC ${QTUI_SOURCE} ${QTUI_KEYPAIR_DETAILS_SOURCE})

target_link_libraries(qtui
        Qt5::Network Qt5::PrintSupport Qt5::Widgets Qt5::Test Qt5::Core)