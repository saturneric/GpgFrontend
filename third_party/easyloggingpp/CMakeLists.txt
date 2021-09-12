aux_source_directory(. EASY_LOGGING_CPP_SOURCE)

add_library(easy_logging_pp STATIC ${EASY_LOGGING_CPP_SOURCE})
target_link_libraries(easy_logging_pp stdc++)