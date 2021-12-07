cmake_minimum_required(VERSION 3.16)

project(GpgFrontend VERSION 2.0.1 LANGUAGES CXX)

message(STATUS "GpgFrontend Build Configuration Started CMAKE Version ${CMAKE_VERSION}")

# C++
#Detect Compiler
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # using Clang
    message(STATUS "Using Complier Clang")
    set(USING_COMPILER_CLANG 1)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # using GCC
    message(STATUS "Using Complier Gcc")
    set(USING_COMPILER_GCC 1)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # using Intel C++
    message(STATUS "Using Complier Intel")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # using Visual Studio C++
    message(STATUS "Using Complier Msvc")
endif ()

# Using Standard C++-17 (Consider compatibility)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# CMake
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Check Env Variables Before Configuring

if (CMAKE_BUILD_TYPE)
    message(STATUS "PreCheck CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}")
else ()
    set(CMAKE_BUILD_TYPE "Release")
    message(STATUS "Set CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}")
endif ()

# Specify different compilation modes
if (BUILD_CONFIG)
    # Test Build
    if (${BUILD_CONFIG} STREQUAL "test")
        message(STATUS "Switch TEST_BUILD")
        set(TEST_BUILD 1)
        set(AppName GpgFrontendTest)
        # Test Build With Coverage Test
    elseif (${BUILD_CONFIG} STREQUAL "test_coverage")
        message(STATUS "Switch TEST_COVERAGE_BUILD")
        set(TEST_BUILD 1)
        if (USING_COMPILER_CLANG OR USING_COMPILER_GCC)
            set(TEST_COVERAGE_BUILD 1)
            set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)
        else ()
            message(WARNING "Branch testing is disabled")
            message(WARNING "Please use gcc or clang for branch coverage test.")
        endif ()
        set(AppName GpgFrontendTest)
        # Test Build with minimum UI
    elseif (${BUILD_CONFIG} STREQUAL "test_ui")
        message(STATUS "Switch TEST_MINIMUM_UI_BUILD")
        set(MINIMUM_APPLICATION_BUILD 1)
        set(AppName GpgFrontend)
    elseif (${BUILD_CONFIG} STREQUAL "test_all")
        message(STATUS "Switch FULL_APPLICATION_BUILD")
        set(FULL_APPLICATION_BUILD 1)
        set(AppName GpgFrontend)
    endif ()
else ()
    message(STATUS "Switch Default STABLE_APPLICATION_BUILD")
    set(STABLE_APPLICATION_BUILD 1)
    set(AppName GpgFrontend)
endif ()

if (EXECUTABLE_OUTPUT_PATH)
    message(STATUS "PreCheck EXECUTABLE_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH}")
else ()
    set(EXECUTABLE_OUTPUT_PATH "./")
    message(STATUS "Set EXECUTABLE_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH}")
endif ()

# Output Env Variables
message(STATUS "Define EXECUTABLE_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH}")
message(STATUS "Define CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}")

if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    set(BUILD_FLAG 0)
    ADD_DEFINITIONS(-DRELEASE)
    message(STATUS "Build Type RELEASE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
    # Release Version force to build using option STABLE_APPLICATION_BUILD
    set(FULL_APPLICATION_BUILD 0)
    set(STABLE_APPLICATION_BUILD 1)
else ()
    set(BUILD_FLAG 1)
    ADD_DEFINITIONS(-DDEBUG)
    message(STATUS "Build Type DEBUG")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g")
    # Generate branch coverage information using gcc
    if (TEST_COVERAGE_BUILD)
        if (USING_COMPILER_GCC)
            message(STATUS "Set branch coverage test parameters for Gcc")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
        elseif (USING_COMPILER_CLANG)
            message(STATUS "Set branch coverage test parameters for Clang")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
        endif ()
    endif ()
endif ()

message(STATUS "Build Flags " ${CMAKE_CXX_FLAGS})

# Get Git Information

set(GIT_COMMIT_HASH "")
set(GIT_BRANCH_NAME "")
find_package(Git QUIET)
if (GIT_FOUND)
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
endif ()

set(BUILD_VERSION ${PROJECT_VERSION}_${CMAKE_SYSTEM}_${CMAKE_SYSTEM_PROCESSOR}_${CMAKE_BUILD_TYPE})
set(GIT_VERSION ${GIT_BRANCH_NAME}_${GIT_COMMIT_HASH})
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S")

message(STATUS "Build Timestamp ${BUILD_TIMESTAMP}")
message(STATUS "Build Version ${BUILD_VERSION}")
message(STATUS "Git Version ${GIT_VERSION}")

IF (MINGW)
    message(STATUS "Configuration For OS Platform Microsoft Windows")
    message(STATUS "Build Environment MINGW")

    set(OS_PLATFORM 0)
    add_definitions(-DWINDOWS)

    set(Boost_USE_STATIC_LIBS ON)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")

    include_directories(
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party
            /mingw64/include
    )
    link_directories(
            /mingw64/lib
    )

endif ()

if (APPLE)

    message(STATUS "Configuration For OS Platform MacOS")

    set(OS_PLATFORM 1)
    ADD_DEFINITIONS(-DMACOS)

    set(ENV{Qt5_DIR} /usr/local/opt/qt5/lib/cmake)

    include_directories(
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party
            /usr/local/include
    )
    link_directories(
            /usr/local/lib
            /usr/local/opt/openssl/lib
    )
endif ()

if (UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif ()

if (LINUX)

    message(STATUS "Configuration For OS Platform LINUX")

    set(OS_PLATFORM 2)
    ADD_DEFINITIONS(-DLINUX)

    message(STATUS "GCC Version ${CMAKE_CXX_COMPILER_VERSION}")
    if (CMAKE_CXX_COMPILER_VERSION LESS 9)
        message(STATUS "GCC CXX_FLAGS add filesystem support manually")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++17 -lstdc++fs")
    endif ()

    include_directories(
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party
            /usr/include
            /usr/local/include
    )
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/)
    link_directories(
            /lib/
            /usr/lib/
            /usr/local/lib/
    )

endif ()

message(STATUS "OS_PLATFORM ${OS_PLATFORM}")

# Basic Envirnoment Configure
set(BASIC_ENV_CONFIG 1)
set(QT_MOC_CONFIG 1)
set(ESAY_LOGGING_PP 1)

if (FULL_APPLICATION_BUILD)
    message(STATUS "Build Full Application")
    set(QT5_ENV_SUPPORT 1)
    set(QT_MOC_CONFIG 1)
    set(GPG_CORE 1)
    set(UI_CORE 1)
    set(SMTP_SUPPORT 1)
    set(SERVER_SUPPORT 1)
    set(ADVANCE_SUPPORT 1)
    set(APPLICATION_BUILD 1)
    set(BASIC_ENV_CONFIG 1)
    set(MULTI_LANG_SUPPORT 1)
elseif (MINIMUM_APPLICATION_BUILD)
    set(QT5_ENV_SUPPORT 1)
    set(QT_MOC_CONFIG 1)
    set(GPG_CORE 1)
    set(UI_CORE 1)
    set(APPLICATION_BUILD 1)
    set(BASIC_ENV_CONFIG 1)
    set(MULTI_LANG_SUPPORT 1)
    # Disable APP_IMAGE_UPDATE because of too many issues
    #    if (LINUX)
    #        set(APP_IMAGE_UPDATE 1)
    #    endif ()
elseif (STABLE_APPLICATION_BUILD)
    set(QT5_ENV_SUPPORT 1)
    set(QT_MOC_CONFIG 1)
    set(GPG_CORE 1)
    set(UI_CORE 1)
    set(APPLICATION_BUILD 1)
    set(BASIC_ENV_CONFIG 1)
    # MULTI_LANG_SUPPORT now work only on Linux and macOS
    if (LINUX OR APPLE)
        set(MULTI_LANG_SUPPORT 1)
    endif ()
elseif (TEST_BUILD)
    message(STATUS "Build Test Cases")

    if (MODULES)
    else ()
        message(STATUS "Build Default Modules")
        set(GPG_CORE 1)
    endif ()

endif ()

# Introduce boost
find_package(Boost COMPONENTS date_time filesystem REQUIRED)

if (QT5_ENV_SUPPORT)
    # Support Qt version Both 5.12.x and 5.15.x
    find_package(Qt5 COMPONENTS Core Test Widgets PrintSupport Network REQUIRED)
    # find_package(Qt5 5.15.2 EXACT COMPONENTS Core Test Widgets PrintSupport Network LinguistTools REQUIRED)
    # Qt configuration
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)
endif ()


if (SMTP_SUPPORT)
    add_compile_definitions(SMTP_SUPPORT)
endif ()

if (SERVER_SUPPORT)
    add_compile_definitions(SERVER_SUPPORT)
endif ()

if (ADVANCE_SUPPORT)
    add_compile_definitions(ADVANCE_SUPPORT)
endif ()


if (MULTI_LANG_SUPPORT)
    add_compile_definitions(MULTI_LANG_SUPPORT)
endif ()


add_subdirectory(third_party)
add_subdirectory(src)

if (TEST_BUILD)
    include(CTest)
    enable_testing()
    add_subdirectory(test)
endif ()
