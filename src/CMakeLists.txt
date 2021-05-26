add_subdirectory(gpg)
add_subdirectory(ui)

aux_source_directory(. BASE_SOURCE)

file(GLOB_RECURSE GPG4USB_HEADER_FILES RELACTIVE ../include/* *.h)
qt5_wrap_cpp(QT5_MOCS ${GPG4USB_HEADER_FILES} TARGET gpg4usb)

add_executable(gpg4usb ${BASE_SOURCE} ../gpg4usb.qrc ${QT5_MOCS})

target_link_libraries(gpg4usb
        qtui gpg
        Qt5::Network Qt5::PrintSupport Qt5::Widgets Qt5::Test Qt5::Core)
