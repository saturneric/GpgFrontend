add_subdirectory(gpg)
add_subdirectory(ui)

aux_source_directory(. BASE_SOURCE)

file(GLOB_RECURSE GPG4USB_HEADER_FILES RELACTIVE ../include/* *.h)
qt5_wrap_cpp(QT5_MOCS ${GPG4USB_HEADER_FILES} TARGET gpgfrontend)

# Set Build Information
configure_file(${CMAKE_SOURCE_DIR}/include/GpgFrontend.h.in ${CMAKE_SOURCE_DIR}/include/GpgFrontend.h @ONLY)

# Copy Resource File
file(COPY ${CMAKE_SOURCE_DIR}/resource/ DESTINATION ${EXECUTABLE_OUTPUT_PATH}/ FOLLOW_SYMLINK_CHAIN)

add_executable(gpgfrontend ${BASE_SOURCE} ${CMAKE_SOURCE_DIR}/gpgfrontend.qrc ${QT5_MOCS})

IF (MINGW)
    message(STATUS "Link Application Static Library For MINGW")
    target_link_libraries(gpgfrontend
            qtui gpg
            Qt5::Network Qt5::PrintSupport Qt5::Widgets Qt5::Test Qt5::Core)


else()
    message(STATUS "Link Application Static Library For UNIX")
    target_link_libraries(gpgfrontend
            qtui gpg
            Qt5::Network Qt5::PrintSupport Qt5::Widgets Qt5::Test Qt5::Core)
endif()

