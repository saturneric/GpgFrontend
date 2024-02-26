# Copyright (C) 2021 Saturneric <eric@bktus.com>
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
# Saturneric <eric@bktus.com> starting on May 12, 2021.
#
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.16)

message(STATUS "Current Generator: ${CMAKE_GENERATOR}")
if(CMAKE_GENERATOR STREQUAL "Xcode")
    set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_SOURCE_DIR}/cmake/FlagsOverridesXcode.cmake")
else()
    set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_SOURCE_DIR}/cmake/FlagsOverrides.cmake")
endif()

# define project
project(GpgFrontend 
    VERSION 2.1.2 
    DESCRIPTION "GpgFrontend is a free, open-source, robust yet user-friendly, compact and cross-platform tool for OpenPGP encryption."
    HOMEPAGE_URL "https://gpgfrontend.bktus.com"
    LANGUAGES CXX)

# show cmake version
message(STATUS "GpgFrontend Build Configuration Started CMAKE Version ${CMAKE_VERSION}")

include(CheckIncludeFiles)
include(CheckIncludeFileCXX)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckTypeSize)
include(CheckLibraryExists)
include(CheckCXXSourceCompiles)

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
option(GPGFRONTEND_QT5_BUILD "Swith to Qt5 building mode" OFF)
option(GPGFRONTEND_GENERATE_LINUX_INSTALL_SOFTWARE "Generate an installable version" OFF)
option(GPGFRONTEND_CONFIGURE_FOR_XCODE_BUILD "Generate a version that can be successfully compiled and packaged in Xcode" OFF)
option(GPGFRONTEND_XCODE_TEAM_ID "GpgFrontend Apple Team ID" "NONE")
option(GPGFRONTEND_XOCDE_CODE_SIGN_IDENTITY "GpgFrontend Signing Certificate" "NONE")
option(GPGFRONTEND_XOCDE_APPID "GpgFrontend Apple AppID" "NONE")
option(GPGFRONTEND_XOCDE_PROVISIONING_PROFILE_UUID "GpgFrontend ProvisioningProfile UUID" "NONE")
option(GPGFRONTEND_XOCDE_ENABLE_SANDBOX "Enable SandBox For Xcode Build" OFF)

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

    set(STABLE_BUILD_APPLICATION 1)
endif ()

# linux package build
if (GPGFRONTEND_GENERATE_LINUX_INSTALL_SOFTWARE)

    set(GPGFRONTEND_BUILD_TYPE_TEST_CORE 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_CORE_AND_COVERAGE 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_UI 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_ALL 0)
    unset(GPGFRONTEND_BUILD_CONFIG)

    set(LINUX_INSTALL_SOFTWARE 1)
endif ()

# xcode archive build
if (GPGFRONTEND_CONFIGURE_FOR_XCODE_BUILD)
    set(GPGFRONTEND_GENERATE_LINUX_INSTALL_SOFTWARE 0)
    set(LINUX_INSTALL_SOFTWARE 0)
    set(XCODE_BUILD 1)

    set(GPGFRONTEND_BUILD_TYPE_TEST_CORE 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_CORE_AND_COVERAGE 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_UI 0)
    set(GPGFRONTEND_BUILD_TYPE_TEST_ALL 0)

    unset(GPGFRONTEND_BUILD_CONFIG)
    set(STABLE_BUILD_APPLICATION 1)
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
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        message(FATAL_ERROR "Clang version must be at least 9.0!")
    endif()
    message(STATUS "Build System Using Compiler CLANG, VERSION: ${CMAKE_CXX_COMPILER_VERSION}")
    set(USING_COMPILER_CLANG 1)

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # using gcc
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
        message(FATAL_ERROR "GCC version must be at least 8.0!")
    endif()
    message(STATUS "Build System Using Compiler GCC, VERSION: ${CMAKE_CXX_COMPILER_VERSION}")
    set(USING_COMPILER_GCC 1)

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # using Intel C++
    message(FATAL_ERROR "Intel C++ is not supported.")

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # using Visual Studio C++
    message(FATAL_ERROR "MSVC is not supported.")
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

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(RELEASE)
else()
    add_compile_definitions(DEBUG)
endif()


# Specify compilation mode
message(STATUS "Switch Build Configure STABLE_BUILD_APPLICATION")
set(STABLE_BUILD_APPLICATION 1)
set(AppName GpgFrontend)

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

# Convert BUILD_VERSION and GIT_VERSION to lowercase
string(TOLOWER "${BUILD_VERSION}" BUILD_VERSION)
string(TOLOWER "${GIT_VERSION}" GIT_VERSION)

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

    include_directories(
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party
            /mingw64/include
    )
    link_directories(
            ${CMAKE_SOURCE_DIR}/lib/mingw
            /mingw64/lib
    )

    # Http Request User Agent
    set(HTTP_REQUEST_USER_AGENT "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:123.0) Gecko/20100101 Firefox/123.0")

endif ()

# macOS
if (APPLE)
    message(STATUS "GpgFrontend Configuration For OS Platform MacOS")

    set(OS_PLATFORM 1)
    ADD_DEFINITIONS(-DMACOS)

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
            /opt/homebrew/include
    )
    link_directories(
            /usr/local/lib
            /opt/homebrew/lib
    )

    # Http Request User Agent
    set(HTTP_REQUEST_USER_AGENT "Mozilla/5.0 (Macintosh; Intel Mac OS X 14.3; rv:123.0) Gecko/20100101 Firefox/123.0")

endif ()

if (UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif ()

if (LINUX)

    message(STATUS "GpgFrontend Configuration For OS Platform Linux")

    set(OS_PLATFORM 2)
    add_compile_definitions(LINUX)

    # Get Env Info
    find_program(UNAME_PROGRAM uname)
    if(UNAME_PROGRAM)
        execute_process(COMMAND ${UNAME_PROGRAM} OUTPUT_VARIABLE SYSTEM_NAME OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()

    if(USING_COMPILER_CLANG)
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-stdlib=libstdc++>)
    endif()

    if(SYSTEM_NAME STREQUAL "FreeBSD")
        message(STATUS "FreeBSD BOX")
        add_compile_definitions(FREEBSD)
        set(FREEBSD TRUE)
    endif()

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

    # Http Request User Agent
    set(HTTP_REQUEST_USER_AGENT "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:123.0) Gecko/20100101 Firefox/123.0")

endif ()

if (LINUX_INSTALL_SOFTWARE)
    message(STATUS "Linux Install Traget ${CMAKE_INSTALL_PREFIX}")
    include(GNUInstallDirs)
    set(APP_INSTALL_FLAG LINUX_INSTALL)
    add_compile_definitions(LINUX_INSTALL_BUILD)
else ()
    set(APP_INSTALL_FLAG BUNDLE)
endif ()

if (GPGFRONTEND_QT5_BUILD)
    add_compile_definitions(QT5_BUILD)
else()
    add_compile_definitions(QT6_BUILD)
endif()

# Basic ENV Configure
set(BASIC_ENV_CONFIG 1)
set(QT_MOC_CONFIG 1)

if (STABLE_BUILD_APPLICATION)
    message("[+] Build Stable Application")
    set(BUILD_CORE 1)
    set(BUILD_UI 1)
    set(BUILD_MODULE 1)
    set(BUILD_TEST 1)
    set(BUILD_APPLICATION 1)
    set(BASIC_ENV_CONFIG 1)
    set(SUPPORT_MULTI_LANG 1)
endif ()

# For instance in order to select the highest version one
SET(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)
SET(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

if (SUPPORT_MULTI_LANG)
    add_compile_definitions(SUPPORT_MULTI_LANG)
endif ()

# Output Env Variables
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Build C Flags: ${CMAKE_C_FLAGS}")
message(STATUS "Build C++ Flags: ${CMAKE_CXX_FLAGS}")

# third_party
add_subdirectory(third_party)

# source code
add_subdirectory(src)
