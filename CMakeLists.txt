cmake_minimum_required(VERSION 3.16)

project(GpgFrontend VERSION 1.1.0 LANGUAGES CXX)

message(STATUS "GPGFrontend Build Configuration Started CMAKE Version ${CMAKE_VERSION}")

# C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Check Env Variables Before Configuring

if(CMAKE_BUILD_TYPE)
    message(STATUS "PreCheck CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}")
else()
    set(CMAKE_BUILD_TYPE "Release")
    message(STATUS "Set CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}")
endif()

if(EXECUTABLE_OUTPUT_PATH)
    message(STATUS "PreCheck EXECUTABLE_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH}")
else()
    set(EXECUTABLE_OUTPUT_PATH "./")
    message(STATUS "Set EXECUTABLE_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH}")
endif()

# Output Env Variables
message(STATUS "Define EXECUTABLE_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH}")
message(STATUS "Define CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}")

if(${CMAKE_BUILD_TYPE} STREQUAL  "Release")
    set(BUILD_FLAG 0)
    message(STATUS "Build Type RELEASE")
    set(CMAKE_CXX_FLAGS_RELEASE_INIT "-O2")
else()
    set(BUILD_FLAG 1)
    message(STATUS "Build Type DEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall")
endif()

# Get Git Information

set(GIT_COMMIT_HASH "")
set(GIT_BRANCH_NAME "")
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%H
            OUTPUT_VARIABLE GIT_COMMIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    execute_process(
            COMMAND ${GIT_EXECUTABLE} symbolic-ref --short -q HEAD
            OUTPUT_VARIABLE GIT_BRANCH_NAME
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()


set(BUILD_VERSION ${PROJECT_VERSION}_${CMAKE_SYSTEM}_${CMAKE_SYSTEM_PROCESSOR}_${CMAKE_BUILD_TYPE})
set(GIT_VERSION ${GIT_BRANCH_NAME}_${GIT_COMMIT_HASH})
set(AppName GpgFrontend)
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S")

message(STATUS "Build Timestamp ${BUILD_TIMESTAMP}")
message(STATUS "Build Version ${BUILD_VERSION}")
message(STATUS "Git Version ${GIT_VERSION}")

IF (MINGW)

	message(STATUS "Configuration For OS Platform Microsoft Windows")
	message(STATUS "Build Environment MINGW")

    set(OS_PLATFORM 0)

    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")

    include_directories(
        include
        /mingw64/include
    )
	link_directories(
        /mingw64/lib
    )

endif()

if(APPLE)

    message(STATUS "Configuration For OS Platform MacOS")

    set(OS_PLATFORM 1)

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

    message(STATUS "Configuration For OS Platform LINUX")
    set(OS_PLATFORM 2)

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

message(STATUS "OS_PLATFORM ${OS_PLATFORM}")

find_package(Qt5 COMPONENTS Core Test Widgets PrintSupport Network LinguistTools REQUIRED)

add_subdirectory(src)



