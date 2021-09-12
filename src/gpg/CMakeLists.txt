aux_source_directory(./result_analyse GPG_SOURCE)
aux_source_directory(./function GPG_SOURCE)
aux_source_directory(./model GPG_SOURCE)
aux_source_directory(. GPG_SOURCE)

add_library(gpg_core STATIC ${GPG_SOURCE})

set(UTILS_DIR ${CMAKE_SOURCE_DIR}/utils)

set(GPGME_LIB_DIR ${UTILS_DIR}/gpgme/lib)

if(ESAY_LOGGING_PP)
   message(STATUS "Link ESAY_LOGGING_PP")
   set(THIRD_PARTY_LIBS easy_logging_pp config++)
endif()

set(BOOST_LIBS Boost::date_time Boost::filesystem)

message(STATUS "Third Party Libraries " ${THIRD_PARTY_LIBS})

if (MINGW)
    message(STATUS "Link GPG Static Library For MINGW")
    target_link_libraries(gpg_core ${THIRD_PARTY_LIBS}
            ${BOOST_LIBS}
            gpgme gpg-error assuan wsock32)
elseif(APPLE)
    message(STATUS "Link GPG Static Library For macOS")
    target_link_libraries(gpg_core ${THIRD_PARTY_LIBS}
            ${BOOST_LIBS}
            libgpgme.a libgpg-error.a libassuan.a dl)
else()
    message(STATUS "Link GPG Static Library For Unix")
    target_link_libraries(gpg_core ${THIRD_PARTY_LIBS}
            libgpgme.a libgpg-error.a libassuan.a
            ${BOOST_LIBS}
            pthread dl)
endif()
