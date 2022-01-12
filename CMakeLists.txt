cmake_minimum_required(VERSION 3.16)

project(GpgFrontend VERSION 2.0.4 LANGUAGES CXX)

message(STATUS "GpgFrontend Build Configuration Started CMAKE Version ${CMAKE_VERSION}")

INCLUDE(CheckIncludeFiles)
INCLUDE(CheckIncludeFileCXX)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckSymbolExists)
INCLUDE(CheckTypeSize)
INCLUDE(CheckLibraryExists)
INCLUDE(CheckCXXSourceCompiles)


# options
option(GPGFRONTEND_BUILD_TYPE_TEST_CORE
        "Only compile the core and generate the unit test program" OFF)
option(GPGFRONTEND_BUILD_TYPE_TEST_CORE_AND_COVERAGE
        "Compile only the core and generate unit test programs that can evaluate test coverage" OFF)
option(GPGFRONTEND_BUILD_TYPE_TEST_UI
        "Only generate a graphical interface with basic functions" OFF)
option(GPGFRONTEND_BUILD_TYPE_TEST_ALL
        "Generate a graphical interface with all functions" OFF)
option(GPGFRONTEND_BUILD_TYPE_STABLE
        "Generate release version" ON)
option(GENERATE_LINUX_INSTALL_SOFTWARE "Generate an installable version" OFF)
option(GENERATE_APP_PACKAGE_DEB "Generate DEB package" OFF)
option(CONFIGURE_FOR_XCODE_BUILD "Generate a version that can be successfully compiled and packaged in Xcode" OFF)
option(USE_QT_DEFAULT_ENV "Build with the qt version provided by the qt-default package" OFF)

# analyse options
if (GPGFRONTEND_BUILD_TYPE_TEST_CORE)
    set(CMAKE_BUILD_TYPE "Debug")
    set(GPGFRONTEND_BUILD_CONFIG "test_core")
endif ()

if (GPGFRONTEND_BUILD_TYPE_TEST_CORE_AND_COVERAGE)
    set(CMAKE_BUILD_TYPE "Debug")
    set(GPGFRONTEND_BUILD_CONFIG "test_core_coverage")
endif ()

if (GPGFRONTEND_BUILD_TYPE_TEST_UI)
    set(CMAKE_BUILD_TYPE "Debug")
    set(GPGFRONTEND_BUILD_CONFIG "test_ui")
endif ()

if (GPGFRONTEND_BUILD_TYPE_TEST_ALL)
    set(CMAKE_BUILD_TYPE "Debug")
    set(GPGFRONTEND_BUILD_CONFIG "test_all")
endif ()

if (GPGFRONTEND_BUILD_TYPE_STABLE)

    set(GPGFRONTEND_BUILD_TYPE_TEST_CORE 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_CORE_AND_COVERAGE 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_UI 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_ALL 0)
    unset(GPGFRONTEND_BUILD_CONFIG)

    set(STABLE_APPLICATION_BUILD 1)
endif ()

if (GENERATE_LINUX_INSTALL_SOFTWARE)

    set(GPGFRONTEND_BUILD_TYPE_TEST_CORE 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_CORE_AND_COVERAGE 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_UI 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_ALL 0)
    unset(GPGFRONTEND_BUILD_CONFIG)

    set(LINUX_INSTALL_SOFTWARE 1)
    set(GENERATE_APP_PACKAGE_DEB 1)
endif ()

if (GENERATE_LINUX_INSTALL_SOFTWARE AND GENERATE_APP_PACKAGE_DEB)
    set(APP_PACKAGE_DEB 1)
endif ()

if (CONFIGURE_FOR_XCODE_BUILD)
    set(GENERATE_LINUX_INSTALL_SOFTWARE 0)
    set(GENERATE_APP_PACKAGE_DEB 0)
    set(LINUX_INSTALL_SOFTWARE 0)
    set(APP_PACKAGE_DEB 0)

    set(GPGFRONTEND_BUILD_TYPE_TEST_CORE 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_CORE_AND_COVERAGE 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_UI 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_ALL 0)

    unset(GPGFRONTEND_BUILD_CONFIG)
    set(STABLE_APPLICATION_BUILD 1)
endif ()

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

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif ()

# Specify different compilation modes
if (GPGFRONTEND_BUILD_CONFIG)
    # Test Build
    if (${GPGFRONTEND_BUILD_CONFIG} STREQUAL "test_core")
        message(STATUS "Switch Build Configure TEST_CORE_BUILD")
        set(TEST_CORE_BUILD 1)
        set(AppName GpgFrontendCoreTest)
        # Test Build With Coverage Test
    elseif (${GPGFRONTEND_BUILD_CONFIG} STREQUAL "test_core_coverage")
        message(STATUS "Switch Build Configure TEST_COVERAGE_BUILD")
        set(TEST_CORE_BUILD 1)
        if (USING_COMPILER_CLANG OR USING_COMPILER_GCC)
            set(TEST_COVERAGE_BUILD 1)
            set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)
        else ()
            message(WARNING "Branch testing is disabled")
            message(WARNING "Please use gcc or clang for branch coverage test.")
        endif ()
        set(AppName GpgFrontendTest)
        # Test Build with minimum UI
    elseif (${GPGFRONTEND_BUILD_CONFIG} STREQUAL "test_ui")
        message(STATUS "Switch Build Configure TEST_MINIMUM_UI_BUILD")
        set(MINIMUM_APPLICATION_BUILD 1)
        set(AppName GpgFrontend)
    elseif (${GPGFRONTEND_BUILD_CONFIG} STREQUAL "test_all")
        message(STATUS "Switch Build Configure FULL_APPLICATION_BUILD")
        set(FULL_APPLICATION_BUILD 1)
        set(AppName GpgFrontend)
    endif ()
else ()
    message(STATUS "Switch Build Configure STABLE_APPLICATION_BUILD")
    set(STABLE_APPLICATION_BUILD 1)
    set(AppName GpgFrontend)
endif ()

# Output Env Variables
message(STATUS "Define CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}")

if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    set(BUILD_FLAG 0)
    ADD_DEFINITIONS(-DRELEASE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
    # Release Version force to build using option STABLE_APPLICATION_BUILD
    set(FULL_APPLICATION_BUILD 0)
    set(STABLE_APPLICATION_BUILD 1)
else ()
    set(BUILD_FLAG 1)
    ADD_DEFINITIONS(-DDEBUG)
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

message(STATUS "GpgFrontend Build Timestamp ${BUILD_TIMESTAMP}")
message(STATUS "GpgFrontend Build Version ${BUILD_VERSION}")
message(STATUS "GpgFrontend Git Repo Version ${GIT_VERSION}")

IF (MINGW)
    message(STATUS "GpgFrontend Configuration For OS Platform Microsoft Windows")
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
    message(STATUS "GpgFrontend Configuration For OS Platform MacOS")

    set(OS_PLATFORM 1)
    ADD_DEFINITIONS(-DMACOS)

    set(ENV{Qt5_DIR} /usr/local/opt/qt5/lib/cmake)

    if (XCODE_BUILD)
        set(XCODE_CODE_SIGN_IDENTITY "\"${XCODE_CODE_SIGN_IDENTITY}\"")
        message(STATUS "XCODE_CODE_SIGN_IDENTITY ${XCODE_CODE_SIGN_IDENTITY}")
        if (APPLE_SANDBOX)
            add_compile_definitions(APPLE_SANDBOX)
        endif ()
    endif ()

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

    message(STATUS "GpgFrontend Configuration For OS Platform LINUX")

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

if (GPG_STANDALONE_MODE)
    add_compile_definitions(GPG_STANDALONE_MODE)
endif ()



if (LINUX_INSTALL_SOFTWARE)
    message(STATUS "Configure Linux Install ON")
    include(GNUInstallDirs)
    set(INSTALL_GPGFRONTEND_APP 1)
    set(APP_INSTALL_FLAG LINUX_INSTALL)
    set(USE_QT_DEFAULT_ENV ON)
    add_compile_definitions(LINUX_INSTALL_BUILD)
    set(Boost_USE_STATIC_LIBS ON)
else ()
    set(APP_INSTALL_FLAG BUNDLE)
    add_compile_definitions(BUNDLE_BUILD)
endif ()

# Basic ENV Configure
set(BASIC_ENV_CONFIG 1)
set(QT_MOC_CONFIG 1)

if (FULL_APPLICATION_BUILD)
    message(STATUS "Build Full Application")
    set(QT5_ENV_SUPPORT 1)
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
    set(GPG_CORE 1)
    set(UI_CORE 1)
    set(APPLICATION_BUILD 1)
    set(BASIC_ENV_CONFIG 1)
    # Disable APP_IMAGE_UPDATE because of too many issues
    #    if (LINUX)
    #        set(APP_IMAGE_UPDATE 1)
    #    endif ()
elseif (STABLE_APPLICATION_BUILD)
    set(QT5_ENV_SUPPORT 1)
    set(GPG_CORE 1)
    set(UI_CORE 1)
    set(APPLICATION_BUILD 1)
    set(BASIC_ENV_CONFIG 1)
    set(SMTP_SUPPORT 1)
    set(MULTI_LANG_SUPPORT 1)
elseif (TEST_CORE_BUILD)
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
    if (USE_QT_DEFAULT_ENV)
        find_package(Qt5 COMPONENTS Core Test Widgets PrintSupport Network REQUIRED)
    else ()
        find_package(Qt5 5.15 COMPONENTS Core Test Widgets PrintSupport Network REQUIRED)
    endif ()
    # find_package(Qt5 5.15.2 EXACT COMPONENTS Core Test Widgets PrintSupport Network LinguistTools REQUIRED)
    # Qt configuration
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)

    set(CMAKE_AUTORCC_OPTIONS "--compress;9")
    set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_AUTOUIC_SEARCH_PATHS} ${CMAKE_SOURCE_DIR}/ui)

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

# third_party
add_subdirectory(third_party)

# source code
add_subdirectory(src)

if (TEST_CORE_BUILD)
    include(CTest)
    enable_testing()
    add_subdirectory(test)
endif ()
