# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
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

cmake_minimum_required(VERSION 3.24)

if(CMAKE_GENERATOR STREQUAL "Xcode")
  set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_SOURCE_DIR}/cmake/FlagsOverridesXcode.cmake")
  set(XCODE_BUILD 1)
else()
  set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_SOURCE_DIR}/cmake/FlagsOverrides.cmake")
endif()

# define project
project(GpgFrontend
  VERSION 2.1.8
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

# general build options
option(GPGFRONTEND_QT5_BUILD "Swith to Qt5 building mode" OFF)
option(GPGFRONTEND_BUILD_APP_FOR_PACKAGE "Allow generating an installable package(DEB or RPM)." OFF)
option(GPGFRONTEND_ENABLE_ASAN "Enable ASAN" OFF)
option(GPGFRONTEND_BUILD_APP_IMAGE "Build AppImage" OFF)
option(GPGFRONTEND_BUILD_MODULES "Build Modules" ON)

# xcode build options
option(GPGFRONTEND_XCODE_TEAM_ID "GpgFrontend Apple Team ID" "NONE")
option(GPGFRONTEND_XCODE_CODE_SIGN_IDENTITY "GpgFrontend Signing Certificate" "NONE")
option(GPGFRONTEND_XCODE_APPID "GpgFrontend Apple AppID" "NONE")
option(GPGFRONTEND_XCODE_PROVISIONING_PROFILE_UUID "GpgFrontend ProvisioningProfile UUID" "NONE")
option(GPGFRONTEND_XCODE_ENABLE_SANDBOX "Enable SandBox For Xcode Build" OFF)

# check options
option(GPGFRONTEND_CHECK_RELEASE_COMMIT_HASH "Enable checking on commit hash from the GitHub release" OFF)

if(GPGFRONTEND_BUILD_APP_IMAGE)
  set(BUILD_APP_IMAGE 1)
endif()

if(GPGFRONTEND_BUILD_APP_FOR_PACKAGE)
  set(BUILD_APP_FOR_PACKAGE 1)

  if(NOT DEFINED CMAKE_INSTALL_PREFIX OR CMAKE_INSTALL_PREFIX STREQUAL "")
    if(BUILD_FOR_PACKAGING)
      set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Normal packaging path" FORCE)
    else()
      set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Normal installing path" FORCE)
    endif()
  endif()

  set(PACKAGING_INSTALL_PREFIX "/usr" CACHE PATH "Prefix by packaging")
  set(CPACK_PACKAGING_INSTALL_PREFIX "${PACKAGING_INSTALL_PREFIX}")
endif()

if(GPGFRONTEND_BUILD_MODULES)
  set(BUILD_MODULES 1)
endif()

if(GPGFRONTEND_CHECK_RELEASE_COMMIT_HASH)
  set(CHECK_RELEASE_COMMIT_HASH 1)
else()
  set(CHECK_RELEASE_COMMIT_HASH 0)
endif()

# C++

# options for ccache
find_program(CCACHE_FOUND ccache)

if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

# detect compiler
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
  # using clang
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    message(FATAL_ERROR "Clang version must be at least 9.0!")
  endif()

  message(STATUS "Build System Using Compiler CLANG, VERSION: ${CMAKE_CXX_COMPILER_VERSION}")
  set(USING_COMPILER_CLANG 1)

elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # using gcc
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
    message(FATAL_ERROR "GCC version must be at least 8.0!")
  endif()

  message(STATUS "Build System Using Compiler GCC, VERSION: ${CMAKE_CXX_COMPILER_VERSION}")
  set(USING_COMPILER_GCC 1)

elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # using Intel C++
  message(FATAL_ERROR "Intel C++ is not supported.")

elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
  message(FATAL_ERROR "MSVC is not supported.")
else()
  # using a unknown compiler
  message(FATAL_ERROR "Compiler: ${CMAKE_CXX_COMPILER_ID} is not supported.")
endif()

# Using Standard C++-17 (Consider compatibility)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# CMake
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Check Env Variables Before Configuring
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_definitions(RELEASE)
else()
  add_compile_definitions(DEBUG)
endif()

# use xcode archive build at macos release at default
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND APPLE)
  set(GPGFRONTEND_GENERATE_LINUX_INSTALL_SOFTWARE 0)
  set(LINUX_INSTALL_SOFTWARE 0)
endif()

# if enable ASAN
if(GPGFRONTEND_ENABLE_ASAN)
  # check compiler
  if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    message(FATAL_ERROR "Use GPGFRONTEND_ENABLE_ASAN only when using the clang compiler.")
  endif()

  add_compile_options(-fsanitize=address -fsanitize-recover=address)
  add_link_options(-fsanitize=address -fsanitize-recover=address)

  set(ENABLE_ASAN 1)
endif()

if(LINUX)
  string(TOLOWER "${CMAKE_PROJECT_NAME}" APP_NAME)
else()
  set(APP_NAME "${CMAKE_PROJECT_NAME}")
endif()

string(TOLOWER "${APP_NAME}" APP_NAME_LOWER)

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
string(TIMESTAMP BUILD_TIMESTAMP UTC)

# Convert BUILD_VERSION and GIT_VERSION to lowercase
string(TOLOWER "${BUILD_VERSION}" BUILD_VERSION)
string(TOLOWER "${GIT_VERSION}" GIT_VERSION)

message(STATUS "GpgFrontend Build Timestamp ${BUILD_TIMESTAMP}")
message(STATUS "GpgFrontend Build Version ${BUILD_VERSION}")
message(STATUS "GpgFrontend Git Repo Version ${GIT_VERSION}")

# support for dymatic libraries
include(GenerateExportHeader)

# Windows
if(MINGW)
  message(STATUS "GpgFrontend Configuration For OS Platform Microsoft Windows")
  message(STATUS "Build Environment MINGW")

  set(OS_PLATFORM 0)

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
endif()

# macOS
if(APPLE)
  message(STATUS "GpgFrontend Configuration For OS Platform MacOS")

  set(OS_PLATFORM 1)

  if(XCODE_BUILD)
    set(XCODE_CODE_SIGN_IDENTITY "\"${XCODE_CODE_SIGN_IDENTITY}\"")
    message(STATUS "XCODE_CODE_SIGN_IDENTITY ${XCODE_CODE_SIGN_IDENTITY}")

    if(APPLE_SANDBOX)
      add_compile_definitions(APPLE_SANDBOX)
    endif()
  endif()

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
endif()

if(UNIX AND NOT APPLE)
  set(LINUX TRUE)
endif()

if(LINUX)
  message(STATUS "GpgFrontend Configuration For OS Platform Linux")

  set(OS_PLATFORM 2)

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
endif()

if(GPGFRONTEND_QT5_BUILD)
  # Support Qt version: 5.15.x
  find_package(Qt5 5.15 COMPONENTS Core Widgets PrintSupport Network LinguistTools REQUIRED)
else()
  # Support Qt version: 6.x
  find_package(Qt6 6 COMPONENTS Core Widgets PrintSupport Network LinguistTools REQUIRED)
endif()

# Qt configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_AUTOUIC_SEARCH_PATHS} ${CMAKE_SOURCE_DIR}/ui)

# For instance in order to select the highest version one
SET(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)
SET(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Output Env Variables
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Build C Flags: ${CMAKE_C_FLAGS}")
message(STATUS "Build C++ Flags: ${CMAKE_CXX_FLAGS}")

# third_party
add_subdirectory(third_party) # source code

# Set build information
configure_file(${CMAKE_SOURCE_DIR}/src/GpgFrontend.h.in ${CMAKE_SOURCE_DIR}/src/GpgFrontend.h @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/GpgFrontendBuildInfo.h.in ${CMAKE_SOURCE_DIR}/src/GpgFrontendBuildInfo.h @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/sdk/GFSDKBuildInfo.h.in ${CMAKE_SOURCE_DIR}/src/sdk/GFSDKBuildInfo.h @ONLY)

if(APPLE)
  configure_file(${CMAKE_SOURCE_DIR}/resource/plist/ExportOptions.plist.in ${CMAKE_BINARY_DIR}/ExportOptions.plist @ONLY)
endif()

# binary and libraries output path
if(XCODE_BUILD)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
elseif(MINGW)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/artifacts/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/artifacts/bin)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/artifacts/bin)
elseif(BUILD_APP_IMAGE)
  set(APPDIR "${CMAKE_BINARY_DIR}/artifacts/AppDir")

  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${APPDIR}/usr/bin")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${APPDIR}/usr/lib")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${APPDIR}/usr/lib")

  file(MAKE_DIRECTORY "${APPDIR}/usr/share/icons")
  file(COPY "${CMAKE_SOURCE_DIR}/resource/lfs/hicolor" DESTINATION "${APPDIR}/usr/share/icons")

  file(MAKE_DIRECTORY "${APPDIR}/usr/share/metainfo")
  file(COPY "${CMAKE_SOURCE_DIR}/resource/appstream/com.bktus.gpgfrontend.metainfo.xml"
    DESTINATION "${APPDIR}/usr/share/metainfo" FOLLOW_SYMLINK_CHAIN)

  file(MAKE_DIRECTORY "${APPDIR}/usr/share/applications")
  file(COPY "${CMAKE_SOURCE_DIR}/resource/appstream/com.bktus.gpgfrontend.desktop"
    DESTINATION "${APPDIR}/usr/share/applications" FOLLOW_SYMLINK_CHAIN)

  file(COPY "${CMAKE_SOURCE_DIR}/resource/lfs/pixmaps/com.bktus.gpgfrontend.png"
    DESTINATION "${APPDIR}" FOLLOW_SYMLINK_CHAIN)

  message(STATUS "AppImage mode enabled. All outputs will go to: ${APPDIR}")
else()
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/artifacts)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/artifacts)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/artifacts)
endif()

# rpath
if(XCODE_BUILD)
  set(CMAKE_MACOSX_RPATH TRUE)
  set(CMAKE_INSTALL_RPATH "@executable_path/../Frameworks")
else()
  set(CMAKE_SKIP_INSTALL_RPATH FALSE)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

# modules are only for qt6
if(BUILD_MODULES AND NOT GPGFRONTEND_QT5_BUILD)
  include(${CMAKE_SOURCE_DIR}/cmake/ModuleRegistry.cmake)
  add_subdirectory(modules)
endif()

add_subdirectory(src)