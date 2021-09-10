aux_source_directory(./result_analyse GPG_SOURCE)
aux_source_directory(./function GPG_SOURCE)
aux_source_directory(./model GPG_SOURCE)
aux_source_directory(. GPG_SOURCE)

add_library(gpg_core STATIC ${GPG_SOURCE})

set(UTILS_DIR ${CMAKE_SOURCE_DIR}/utils)

set(GPGME_LIB_DIR ${UTILS_DIR}/gpgme/lib)

if(ESAY_LOGGING_CPP)
   message(STATUS "Link ESAY_LOGGING_CPP")
   set(THIRD_PARTY_LIBS easy_logging_cpp)
endif()

message(STATUS "Third Party Libraries " ${THIRD_PARTY_LIBS})

if (MINGW)
    message(STATUS "Link GPG Static Library For MINGW")
    target_link_libraries(gpg_core ${THIRD_PARTY_LIBS}
            Boost::date_time
            gpgme gpg-error assuan wsock32)
elseif(APPLE)
    message(STATUS "Link GPG Static Library For macOS")
    target_link_libraries(gpg_core ${THIRD_PARTY_LIBS}
            Boost::date_time
            libgpgme.a libgpg-error.a libassuan.a)
else()
    message(STATUS "Link GPG Static Library For Unix")
    target_link_libraries(gpg_core ${THIRD_PARTY_LIBS}
            libgpgme.a libgpg-error.a libassuan.a
            Boost::date_time
            pthread)
endif()
