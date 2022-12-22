#
# Copyright (C) 2021 Saturneric
#
# This file is part of GpgFrontend.
#
# GpgFrontend is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# GpgFrontend is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
#
# The initial version of the source code is inherited from
# the gpg4usb project, which is under GPL-3.0-or-later.
#
# All the source code of GpgFrontend was modified and released by
# Saturneric<eric@bktus.com> starting on May 12, 2021.
#
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.16)

# define project
project(GpgFrontend VERSION 2.0.9 LANGUAGES CXX)

# show cmake version
message(STATUS "GpgFrontend Build Configuration Started CMAKE Version ${CMAKE_VERSION}")

INCLUDE(CheckIncludeFiles)
INCLUDE(CheckIncludeFileCXX)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckSymbolExists)
INCLUDE(CheckTypeSize)
INCLUDE(CheckLibraryExists)
INCLUDE(CheckCXXSourceCompiles)

# generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
option(GPGFRONTEND_GENERATE_LINUX_INSTALL_SOFTWARE "Generate an installable version" OFF)
option(GPGFRONTEND_GENERATE_APP_PACKAGE_DEB "Generate DEB package" OFF)
option(GPGFRONTEND_GENERATE_APP_PACKAGE_RPM "Generate RPM package" OFF)
option(GPGFRONTEND_GENERATE_APP_PACKAGE_FREEBSD "Generate PKG package" OFF)
option(GPGFRONTEND_CONFIGURE_FOR_XCODE_BUILD "Generate a version that can be successfully compiled and packaged in Xcode" OFF)
option(GPGFRONTEND_XCODE_TEAM_ID "GpgFrontend Apple Team ID" "NONE")
option(GPGFRONTEND_XOCDE_CODE_SIGN_IDENTITY "GpgFrontend Signing Certificate" "NONE")
option(GPGFRONTEND_XOCDE_APPID "GpgFrontend Apple AppID" "NONE")
option(GPGFRONTEND_XOCDE_PROVISIONING_PROFILE_UUID "GpgFrontend ProvisioningProfile UUID" "NONE")

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

# stable features
if (GPGFRONTEND_BUILD_TYPE_STABLE)

    set(GPGFRONTEND_BUILD_TYPE_TEST_CORE 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_CORE_AND_COVERAGE 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_UI 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_ALL 0)
    unset(GPGFRONTEND_BUILD_CONFIG)

    set(STABLE_APPLICATION_BUILD 1)
endif ()

# linux package build
if (GPGFRONTEND_GENERATE_LINUX_INSTALL_SOFTWARE)

    set(GPGFRONTEND_BUILD_TYPE_TEST_CORE 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_CORE_AND_COVERAGE 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_UI 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_ALL 0)
    unset(GPGFRONTEND_BUILD_CONFIG)

    set(LINUX_INSTALL_SOFTWARE 1)
    set(GPGFRONTEND_GENERATE_APP_PACKAGE_DEB 1)
endif ()

# linux package build deb
if (GPGFRONTEND_GENERATE_LINUX_INSTALL_SOFTWARE AND GPGFRONTEND_GENERATE_APP_PACKAGE_DEB)
    set(APP_PACKAGE_DEB 1)
endif ()

# linux package build rpm
if (GPGFRONTEND_GENERATE_LINUX_INSTALL_SOFTWARE AND GPGFRONTEND_GENERATE_APP_PACKAGE_RPM)
    set(APP_PACKAGE_RPM 1)
endif ()

# linux package build pkg
if (GPGFRONTEND_GENERATE_LINUX_INSTALL_SOFTWARE AND GPGFRONTEND_GENERATE_APP_PACKAGE_FREEBSD)
    set(APP_PACKAGE_FREEBSD 1)
endif ()

# xcode archive build
if (GPGFRONTEND_CONFIGURE_FOR_XCODE_BUILD)
    set(GPGFRONTEND_GENERATE_LINUX_INSTALL_SOFTWARE 0)
    set(GPGFRONTEND_GENERATE_APP_PACKAGE_DEB 0)
    set(LINUX_INSTALL_SOFTWARE 0)
    set(APP_PACKAGE_DEB 0)
    set(XCODE_BUILD 1)

    set(GPGFRONTEND_BUILD_TYPE_TEST_CORE 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_CORE_AND_COVERAGE 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_UI 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_ALL 0)

    unset(GPGFRONTEND_BUILD_CONFIG)
    set(STABLE_APPLICATION_BUILD 1)
endif ()

# C++

# options for ccache
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

# detect compiler
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # using clang
    message(STATUS "Using Compiler clang")
    message(STATUS "clang version ${CMAKE_CXX_COMPILER_VERSION}")
    set(USING_COMPILER_CLANG 1)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # using gcc
    message(STATUS "Using Compiler gcc")
    message(STATUS "gcc version ${CMAKE_CXX_COMPILER_VERSION}")
    set(USING_COMPILER_GCC 1)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # using Intel C++
    message(STATUS "Using Compiler Intel")
    message(WARNING "Compilation with this compiler is untested, which may create some problems that need to be solved by the user.")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # using Visual Studio C++
    message(STATUS "Using Compiler MSVC")
    message(WARNING "Compilation with this compiler is untested, which may create some problems that need to be solved by the user.")
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
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    # Release Version force to build using option STABLE_APPLICATION_BUILD
    set(FULL_APPLICATION_BUILD 0)
    set(STABLE_APPLICATION_BUILD 1)
else ()
    set(BUILD_FLAG 1)
    ADD_DEFINITIONS(-DDEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
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

# support for dymatic libraries
include (GenerateExportHeader)

# Windows
IF (MINGW)
    message(STATUS "GpgFrontend Configuration For OS Platform Microsoft Windows")
    message(STATUS "Build Environment MINGW")

    set(OS_PLATFORM 0)
    add_definitions(-DWINDOWS)

    set(Boost_USE_STATIC_LIBS OFF)
    set(Boost_USE_MULTITHREADED ON)
    set(Boost_USE_STATIC_RUNTIME OFF)

    include_directories(
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party
            /mingw64/include
    )
    link_directories(
            ${CMAKE_SOURCE_DIR}/lib/mingw
            /mingw64/lib
    )
endif ()

# macOS
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
    )
endif ()

if (UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif ()

if (LINUX)

    message(STATUS "GpgFrontend Configuration For OS Platform LINUX")

    set(OS_PLATFORM 2)
    ADD_DEFINITIONS(-DLINUX)

    # Get Env Info
    exec_program(uname OUTPUT_VARIABLE SYSTEM_NAME)
    set(SYSTEM_NAME "${SYSTEM_NAME}" CACHE INTERNAL "")

    if(SYSTEM_NAME STREQUAL "FreeBSD")
        message(STATUS "FreeBSD BOX")
        ADD_DEFINITIONS(-DFREEBSD)
        set(FREEBSD TRUE)
    endif()

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
    message(STATUS "Build Minimum Application")
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
    message(STATUS "Build Stable Application")
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

# For instance in order to select the highest version one
SET(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)
SET(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)

# disable myeasylog.log
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DELPP_NO_DEFAULT_LOG_FILE")

# Introduce boost
find_package(Boost COMPONENTS date_time system REQUIRED)

# Introduce OpenSSL
if(APPLE)
    set(OPENSSL_ROOT_DIR /usr/local/opt/openssl@1.1)
endif()
find_package(OpenSSL REQUIRED)

# Introduce Qt
if (QT5_ENV_SUPPORT)
    # Support Qt version Both 5.12.x and 5.15.x
    find_package(Qt5 5.9 COMPONENTS Core Test Widgets PrintSupport Network REQUIRED)
    
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

# build to test gpgfrontend core
if (TEST_CORE_BUILD)
    include(CTest)
    enable_testing()
    add_subdirectory(test)
endif ()
