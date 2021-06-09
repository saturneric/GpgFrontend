cmake_minimum_required(VERSION 3.19)

project(GPGFrontend VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-O2")

if(APPLE)

    message("OS Platform MacOS")

    set(ENV{Qt5_DIR} /usr/local/opt/qt5/lib/cmake)

    include_directories(
        include
        /usr/local/include
    )
    link_directories(
        /usr/local/lib
    )
endif()

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

if(LINUX)

    message("OS Platform LINUX")

    include_directories(
        include
        /usr/include
        /usr/local/include
    )
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/)
    link_directories(
        /lib/
        /usr/lib/
        /usr/local/lib/
    )

endif()

find_package(Qt5 COMPONENTS Core Test Widgets PrintSupport Network REQUIRED)

add_subdirectory(src)



