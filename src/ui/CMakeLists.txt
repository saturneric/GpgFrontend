aux_source_directory(. QTUI_SOURCE)

add_library(qtui STATIC ${QTUI_SOURCE})

target_link_libraries(qtui
        Qt5::Network Qt5::PrintSupport Qt5::Widgets Qt5::Test Qt5::Core)