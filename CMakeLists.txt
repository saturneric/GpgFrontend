cmake_minimum_required(VERSION 3.19)

project(gpg4usb VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 COMPONENTS Core Test Widgets PrintSupport Network REQUIRED)

message("${Qt5Widgets_INCLUDE_DIRS}")

include_directories(
        include
        ${CMAKE_SOURCE_DIR}/utils/gpgme/include
)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/)
link_directories(/usr/local/lib/)
link_directories(/lib/)

add_subdirectory(src)



