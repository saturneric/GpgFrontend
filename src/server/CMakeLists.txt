aux_source_directory(. SERVER_SOURCE)

add_library(server STATIC ${SERVER_SOURCE})

target_link_libraries(server
        Qt5::Network Qt5::Widgets Qt5::Core)
