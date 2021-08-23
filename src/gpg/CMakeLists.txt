aux_source_directory(./result_analyse GPG_SOURCE)
aux_source_directory(./gpg_context GPG_SOURCE)
aux_source_directory(. GPG_SOURCE)

add_library(gpg STATIC ${GPG_SOURCE})

set(UTILS_DIR ${CMAKE_SOURCE_DIR}/utils)

set(GPGME_LIB_DIR ${UTILS_DIR}/gpgme/lib)

if (MINGW)
    message(STATUS "Link GPG Static Library For MINGW")
    target_link_libraries(gpg
            gpgme gpg-error assuan
            Qt5::Network Qt5::PrintSupport Qt5::Widgets Qt5::Test Qt5::Core
            wsock32)
elseif(APPLE)
    message(STATUS "Link GPG Static Library For macOS")
    target_link_libraries(gpg
            /usr/local/lib/libgpgme.a /usr/local/lib/libgpg-error.a /usr/local/lib/libassuan.a
            Qt5::Network Qt5::PrintSupport Qt5::Widgets Qt5::Test Qt5::Core)
else()
        
    message(STATUS "Link GPG Static Library For Unix")
    target_link_libraries(gpg
            /usr/local/lib/libgpgme.a /usr/local/lib/libgpg-error.a /usr/local/lib/libassuan.a
            Qt5::Network Qt5::PrintSupport Qt5::Widgets Qt5::Test Qt5::Core)
endif()
