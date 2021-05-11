cmake_minimum_required(VERSION 3.19)

project(gpg4usb VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OM)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Test REQUIRED)

file(GLOB GPG4USB_HEADER_FILES include/* "*.h")

aux_source_directory(src SOURCES)

include_directories(include)
include_directories(/usr/local/include)

link_directories(/usr/local/lib)
message(STATUS "${GPG4USB_HEADER_FILES}")

qt5_wrap_cpp(QT5_MOCS ${GPG4USB_HEADER_FILES} TARGET gpg4usb)

add_executable(gpg4usb ${SOURCES}  gpg4usb.qrc ${QT5_MOCS})

target_link_libraries(gpg4usb gpg-error gpgme)
qt5_use_modules(gpg4usb Network PrintSupport Widgets Test Core)