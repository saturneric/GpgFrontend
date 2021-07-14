aux_source_directory(. SMTP_MIME_SOURCE)

add_library(smtp STATIC ${SMTP_MIME_SOURCE})

target_link_libraries(smtp
        Qt5::Network Qt5::Core)
