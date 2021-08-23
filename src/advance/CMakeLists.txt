aux_source_directory(. ADVANCE_SOURCE)

add_library(advance STATIC ${ADVANCE_SOURCE})

target_link_libraries(advance
        Qt5::Network Qt5::Widgets Qt5::Core)