aux_source_directory(./result_analyse GPG_SOURCE)
aux_source_directory(./function GPG_SOURCE)
aux_source_directory(./model GPG_SOURCE)
aux_source_directory(. GPG_SOURCE)

add_library(gpg_core STATIC ${GPG_SOURCE})

set(UTILS_DIR ${CMAKE_SOURCE_DIR}/utils)

set(GPGME_LIB_DIR ${UTILS_DIR}/gpgme/lib)

if (EASY_LOGGING_PP)
    set(THIRD_PARTY_LIBS easy_logging_pp)
endif ()
set(THIRD_PARTY_LIBS "${THIRD_PARTY_LIBS} config++")

set(BOOST_LIBS Boost::date_time Boost::filesystem)

if (MINGW)
    message(STATUS "Link GPG Static Library For MINGW")
    target_link_libraries(gpg_core ${THIRD_PARTY_LIBS}
            ${BOOST_LIBS}
            gpgme gpg-error assuan wsock32)
    target_compile_features(gpg_core PUBLIC cxx_std_17)
elseif (APPLE)
    find_library(libgpgme NAMES libgpgme.a)
    find_library(libgpg-error NAMES libgpg-error.a)
    find_library(libassuan NAMES libassuan.a)
    message(STATUS "Link GPG Static Library For macOS")
    target_link_libraries(gpg_core ${THIRD_PARTY_LIBS}
            ${BOOST_LIBS}
            ${libgpgme} ${libgpg-error} ${libassuan}
            dl)
    if (XCODE_BUILD)
        set_target_properties(gpg_core
                PROPERTIES
                ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
                LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
                LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
    endif ()
else ()
    find_library(libgpgme NAMES libgpgme.a)
    find_library(libgpg-error NAMES libgpg-error.a)
    find_library(libassuan NAMES libassuan.a)
    message(STATUS "Link GPG Static Library For Unix")
    target_link_libraries(gpg_core ${THIRD_PARTY_LIBS}
            ${libgpgme} ${libgpg-error} ${libassuan}
            ${BOOST_LIBS}
            pthread dl)
endif ()
