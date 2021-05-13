aux_source_directory(. GPG_SOURCE)

add_library(gpg STATIC ${GPG_SOURCE})

set(UTILS_DIR ${CMAKE_SOURCE_DIR}/utils)

set(GPGME_LIB_DIR ${UTILS_DIR}/gpgme/lib)

target_link_libraries(gpg
       ${GPGME_LIB_DIR}/libgpgme.a ${GPGME_LIB_DIR}/libgpg-error.a ${GPGME_LIB_DIR}/libassuan.a
        Qt5::Network Qt5::PrintSupport Qt5::Widgets Qt5::Test Qt5::Core)