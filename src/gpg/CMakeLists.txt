aux_source_directory(. GPG_SOURCE)

add_library(gpg STATIC ${GPG_SOURCE})

target_link_libraries(gpg gpg-error gpgme)

qt5_use_modules(gpg Network PrintSupport Widgets Test Core)