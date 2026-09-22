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
  VERSION 2.2.3
  DESCRIPTION "A modern, cross-platform OpenPGP tool with a unique dual-engine core."
  HOMEPAGE_URL "https://gpgfrontend.bktus.com"
  LANGUAGES CXX C)

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
option(GPGFRONTEND_ENABLE_ASAN "Enable ASAN for memory testing" OFF)
option(GPGFRONTEND_BUILD_APP_IMAGE "Deprecated; the AppImage job installs" OFF)
option(GPGFRONTEND_BUILD_MODULES "Build Modules" ON)
option(GPGFRONTEND_BUILD_STRIP_RPATH "Strip RPATH from binaries for packaging" OFF)
option(GPGFRONTEND_BUILD_STABLE "Build as a stable release (drops the \"Testing\" display-name suffix)" OFF)
option(GPGFRONTEND_BUILD_PORTABLE "Build as a portable release (data lives beside the application, not in the OS user-data location)" OFF)

# xcode build options
option(GPGFRONTEND_XCODE_TEAM_ID "GpgFrontend Apple Team ID" "NONE")
option(GPGFRONTEND_XCODE_CODE_SIGN_IDENTITY "GpgFrontend Signing Certificate" "NONE")
option(GPGFRONTEND_XCODE_PROVISIONING_PROFILE_UUID "GpgFrontend ProvisioningProfile UUID" "NONE")
option(GPGFRONTEND_XCODE_ENABLE_SANDBOX "Enable SandBox For Xcode Build" OFF)

# check options
option(GPGFRONTEND_LINK_GPGME_INTO_CORE "Statically link GpgME into the core library" OFF)

# Module native-binding policy, for INTEGRATED modules only.
#
# Integrated modules ship inside this distribution, so what protects them is
# whatever protects the distribution: Apple's signature and Library Validation
# on macOS, an immutable image on AppImage, the package manager elsewhere. The
# per-file cryptographic binding on top of that is therefore a policy choice,
# and it is not free -- binding exact ELF bytes is what makes linuxdeployqt,
# patchelf, dh_strip and rpmbuild all invalidate a descriptor.
#
# External modules are NOT governed by this. They always require a binding;
# see ModuleHostPolicy.h.
#
# OFF is the default because it is right for a developer tree and an ordinary
# local install. Official Windows and AppImage releases MUST set REQUIRED, and
# prove they did by passing --require-binding to verify-module-set.
set(GPGFRONTEND_INTEGRATED_MODULE_NATIVE_BINDING "OFF" CACHE STRING
  "Whether integrated module descriptors must bind their entry native: OFF or REQUIRED")
set_property(CACHE GPGFRONTEND_INTEGRATED_MODULE_NATIVE_BINDING
  PROPERTY STRINGS "OFF" "REQUIRED")

# The algorithm is an ASSERTION, not a choice.
#
# Which algorithm is usable is already fixed by the platform: the PE image
# digest is computed by a Windows-only OS API, and a full-file digest cannot
# survive Authenticode. AUTO means "the mode this platform mandates". Naming
# one explicitly states an intent that must match, so a release leg says what
# it expects and a silent platform-detection change becomes a configure error
# rather than a quietly weaker binding.
set(GPGFRONTEND_INTEGRATED_MODULE_BINDING_ALGORITHM "AUTO" CACHE STRING
  "Assert which binding algorithm integrated modules use: AUTO, FILE_SHA256 or PE_AUTHENTICODE_SHA256")
set_property(CACHE GPGFRONTEND_INTEGRATED_MODULE_BINDING_ALGORITHM
  PROPERTY STRINGS "AUTO" "FILE_SHA256" "PE_AUTHENTICODE_SHA256")

if(GPGFRONTEND_BUILD_APP_IMAGE)
  set(BUILD_APP_IMAGE 1)
endif()

if(GPGFRONTEND_BUILD_APP_FOR_PACKAGE)
  set(BUILD_APP_FOR_PACKAGE 1)
  set(BUILD_STRIP_RPATH 1) # suppose that it's a standard for deb or rpm packaging

  if(NOT DEFINED CMAKE_INSTALL_PREFIX OR CMAKE_INSTALL_PREFIX STREQUAL "")
    if(BUILD_FOR_PACKAGING)
      set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Normal packaging path" FORCE)
    else()
      set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Normal installing path" FORCE)
    endif()
  endif()

  set(PACKAGING_INSTALL_PREFIX "/usr" CACHE PATH "Prefix by packaging")
endif()

if(GPGFRONTEND_BUILD_MODULES)
  set(BUILD_MODULES 1)
endif()

if(GPGFRONTEND_BUILD_STRIP_RPATH)
  set(BUILD_STRIP_RPATH 1)
else()
  set(BUILD_STRIP_RPATH 0)
endif()

if(GPGFRONTEND_LINK_GPGME_INTO_CORE)
  set(BUILD_AND_STATIC_LINK_GPGME 1)
else()
  set(BUILD_AND_STATIC_LINK_GPGME 0)
endif()

if(GPGFRONTEND_XCODE_ENABLE_SANDBOX)
  set(APPLE_SANDBOX 1)
endif()

# C++

# options for ccache
find_program(CCACHE_PROGRAM ccache)

if(CCACHE_PROGRAM)
  message(STATUS "Find ccache: ${CCACHE_PROGRAM}, which will be used to speed up compilation.")
  set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "" FORCE)
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "" FORCE)
endif()

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
  if(NOT(("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang") OR("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")))
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

  # for mingw, we will always use the system openssl, since it's easier to get
  # it working on windows and the openssl project provides official pre-built
  # binaries for windows. And in the msys2 environment, the openssl is also
  # available as a package, and it's often the latest version.
  set(USE_SYSTEM_OPENSSL 1)

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

    # For macOS sandbox, we need to define APPLE_SANDBOX for the source code to
    # conditionally compile the code related to sandbox, and we also need to
    # link the Security framework for the sandbox entitlements.
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
  find_package(Qt5 5.15 COMPONENTS Core Widgets PrintSupport Network LinguistTools Xml Sql REQUIRED)
else()
  # Support Qt version: 6.x
  find_package(Qt6 6 COMPONENTS Core Widgets PrintSupport Network LinguistTools Xml Sql REQUIRED)
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

# Decide the name of the app
set(GPGFRONTEND_APP_BASE_DISPLAY_NAME "GpgFrontend")

if(GPGFRONTEND_XCODE_ENABLE_SANDBOX)
  set(GPGFRONTEND_APP_BASE_DISPLAY_NAME "GpgFrontend Lite")
endif()

set(GPGFRONTEND_APP_DISPLAY_NAME "${GPGFRONTEND_APP_BASE_DISPLAY_NAME}")

# Non-stable builds (nightly / local development / any untagged trunk build) are
# marked "Testing" so users can tell them apart from official releases. Stable
# release builds set -DGPGFRONTEND_BUILD_STABLE=ON. This is decoupled from the
# git branch name on purpose: with a single-branch (trunk + release tags) model,
# `main` produces both nightlies and, via tags, releases, so the branch name can
# no longer distinguish the two.
if(NOT GPGFRONTEND_BUILD_STABLE)
  set(GPGFRONTEND_APP_DISPLAY_NAME
    "${GPGFRONTEND_APP_BASE_DISPLAY_NAME} Testing")
endif()

message(STATUS "App Display Name: ${GPGFRONTEND_APP_DISPLAY_NAME}")

# The profile name is the on-disk and registry identifier: it decides
# QCoreApplication::applicationName(), and through it the QSettings scope and
# the AppLocalData directory that holds secure/app.key and data_objs/. A
# non-stable build must not share those with an installed release -- running a
# nightly against a stable profile has silently rewritten and garbage-collected
# it. Deliberately spelled without a space, since it becomes a directory
# component and a Windows registry key.
if(GPGFRONTEND_BUILD_STABLE)
  set(GPGFRONTEND_APP_PROFILE_NAME "${CMAKE_PROJECT_NAME}")
  set(GPGFRONTEND_BUILD_STABLE_INT 1)
else()
  set(GPGFRONTEND_APP_PROFILE_NAME "${CMAKE_PROJECT_NAME}Testing")
  set(GPGFRONTEND_BUILD_STABLE_INT 0)
endif()

message(STATUS "App Profile Name: ${GPGFRONTEND_APP_PROFILE_NAME}")

# Whether this artifact keeps its data beside itself rather than in the OS
# user-data location. A build option rather than anything discovered at run
# time: it decides which directory holds the user's keys, so it must not be
# switchable by dropping a file next to the binary, and a portable release is a
# separately packaged artifact anyway.
if(GPGFRONTEND_BUILD_PORTABLE)
  set(GPGFRONTEND_BUILD_PORTABLE_INT 1)
else()
  set(GPGFRONTEND_BUILD_PORTABLE_INT 0)
endif()

message(STATUS "Portable Build: ${GPGFRONTEND_BUILD_PORTABLE}")

# The module ABI generation, bumped BY HAND on any breaking change. It lives
# here rather than only in GFSDKBuildInfo.h.in because the packaging function
# has to stamp it into every manifest, and a number written down twice is a
# number that will eventually disagree with itself.
set(GF_SDK_ABI_VERSION 4)
set(GF_SDK_ABI_MIN_SUPPORTED 4)

# ---------------------------------------------------------------------------
# Product build identity
# ---------------------------------------------------------------------------
#
# A pure function of PRODUCT configuration. Deliberately NOT a function of who
# ran the build or when: no run id, no run attempt, no runner image, no
# timestamp, no nonce. A CI retry of the same commit must produce the same
# Host and the same descriptors, because nothing a user can observe differs.
#
# It is therefore NOT a unique instance identifier either. Two build trees with
# the same configuration share a build id and are told apart by their keys,
# which is the pair that actually identifies an instance: (build id, public
# key). Execution identity -- run id, runner image -- belongs in a provenance
# record, not in a compiled byte.
set(GPGFRONTEND_BUILD_REPOSITORY "saturneric/GpgFrontend"
  CACHE STRING "The repository this build claims to come from")
set(GPGFRONTEND_BUILD_REF "${GIT_BRANCH_NAME}"
  CACHE STRING "The ref this build was made from")
set(GPGFRONTEND_BUILD_FLAVOR "installed"
  CACHE STRING "installed | portable | sandbox")
set(GPGFRONTEND_BUILD_MODE "nightly"
  CACHE STRING "nightly | release")

if(WIN32)
  set(_gf_build_os "windows")
elseif(APPLE)
  set(_gf_build_os "macos")
else()
  set(_gf_build_os "linux")
endif()

if(GPGFRONTEND_QT5_BUILD)
  set(_gf_build_qt "${Qt5_VERSION}")
else()
  set(_gf_build_qt "${Qt6_VERSION}")
endif()

# Newline-joined and hashed, so adding an input later cannot silently collide
# with an existing key by concatenating differently.
string(JOIN "\n" _gf_build_identity
  "${GPGFRONTEND_BUILD_REPOSITORY}"
  "${GIT_COMMIT_HASH}"
  "${GPGFRONTEND_BUILD_REF}"
  "${_gf_build_os}"
  "${CMAKE_SYSTEM_PROCESSOR}"
  "${GPGFRONTEND_BUILD_FLAVOR}"
  "${GPGFRONTEND_BUILD_MODE}"
  "${CMAKE_BUILD_TYPE}"
  "${GF_SDK_ABI_VERSION}"
  "${_gf_build_qt}")
string(SHA256 _gf_build_identity_digest "${_gf_build_identity}")
string(SUBSTRING "${_gf_build_identity_digest}" 0 32 _gf_build_identity_short)
set(GPGFRONTEND_BUILD_ID "gfb1-${_gf_build_identity_short}")
message(STATUS "GpgFrontend Build ID: ${GPGFRONTEND_BUILD_ID}")

set(GF_MODULE_BUILD_KEY_DIR "${CMAKE_BINARY_DIR}/.module-build-key")
set(GF_MODULE_TRUST_ROOT_SOURCE
  "${CMAKE_BINARY_DIR}/generated/ModuleTrustRootGenerated.cpp")

# Validate and resolve the integrated binding policy, once, here -- so that
# every consumer (the generated constant, the packager command line, the CI
# gates) is reading one already-checked answer rather than re-deciding.
if(GPGFRONTEND_INTEGRATED_MODULE_NATIVE_BINDING STREQUAL "REQUIRED")
  set(GF_INTEGRATED_MODULE_BINDING_REQUIRED 1)
elseif(GPGFRONTEND_INTEGRATED_MODULE_NATIVE_BINDING STREQUAL "OFF")
  set(GF_INTEGRATED_MODULE_BINDING_REQUIRED 0)
else()
  message(FATAL_ERROR
    "GPGFRONTEND_INTEGRATED_MODULE_NATIVE_BINDING must be OFF or REQUIRED, "
    "not \"${GPGFRONTEND_INTEGRATED_MODULE_NATIVE_BINDING}\"")
endif()

# The one mode this target platform mandates. macOS has none: its modules are
# signed by Apple with the application's own identity and enforced at map time,
# so the descriptor makes no claim about their bytes at all.
if(WIN32)
  set(GF_INTEGRATED_MODULE_BINDING_ALGORITHM "PE_AUTHENTICODE_SHA256")
elseif(APPLE)
  set(GF_INTEGRATED_MODULE_BINDING_ALGORITHM "NONE")
else()
  set(GF_INTEGRATED_MODULE_BINDING_ALGORITHM "FILE_SHA256")
endif()

if(GF_INTEGRATED_MODULE_BINDING_REQUIRED
   AND GF_INTEGRATED_MODULE_BINDING_ALGORITHM STREQUAL "NONE")
  message(FATAL_ERROR
    "GPGFRONTEND_INTEGRATED_MODULE_NATIVE_BINDING=REQUIRED cannot be honoured "
    "on macOS: there is no entry binding mode on this platform. Apple code "
    "signing and Library Validation provide executable integrity instead.")
endif()

if(NOT GPGFRONTEND_INTEGRATED_MODULE_BINDING_ALGORITHM STREQUAL "AUTO"
   AND NOT GPGFRONTEND_INTEGRATED_MODULE_BINDING_ALGORITHM
       STREQUAL GF_INTEGRATED_MODULE_BINDING_ALGORITHM)
  message(FATAL_ERROR
    "GPGFRONTEND_INTEGRATED_MODULE_BINDING_ALGORITHM asserts "
    "\"${GPGFRONTEND_INTEGRATED_MODULE_BINDING_ALGORITHM}\", but this platform "
    "mandates \"${GF_INTEGRATED_MODULE_BINDING_ALGORITHM}\". The algorithm "
    "follows the platform; this setting exists to state an expectation, not "
    "to choose one.")
endif()

message(STATUS
  "Integrated module native binding: "
  "${GPGFRONTEND_INTEGRATED_MODULE_NATIVE_BINDING} "
  "(${GF_INTEGRATED_MODULE_BINDING_ALGORITHM})")

set(GF_MODULE_HOST_POLICY_SOURCE
  "${CMAKE_BINARY_DIR}/generated/ModuleHostPolicyGenerated.cpp")
configure_file(
  "${CMAKE_SOURCE_DIR}/cmake/ModuleHostPolicyGenerated.cpp.in"
  "${GF_MODULE_HOST_POLICY_SOURCE}" @ONLY)


# Set build information
configure_file(${CMAKE_SOURCE_DIR}/src/GpgFrontend.h.in ${CMAKE_SOURCE_DIR}/src/GpgFrontend.h @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/GpgFrontendBuildInfo.h.in ${CMAKE_SOURCE_DIR}/src/GpgFrontendBuildInfo.h @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/sdk/GFSDKBuildInfo.h.in ${CMAKE_SOURCE_DIR}/src/sdk/GFSDKBuildInfo.h @ONLY)

# configure plist for macOS bundle
if(APPLE)
  if(APPLE_SANDBOX)
    configure_file(${CMAKE_SOURCE_DIR}/resource/plist/ExportOptionsSandbox.plist.in ${CMAKE_BINARY_DIR}/ExportOptions.plist @ONLY)
  else()
    configure_file(${CMAKE_SOURCE_DIR}/resource/plist/ExportOptions.plist.in ${CMAKE_BINARY_DIR}/ExportOptions.plist @ONLY)
  endif()
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
else()
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/artifacts)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/artifacts)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/artifacts)
endif()

# Where host build tools go, and deliberately NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY.
#
# That variable follows the layout: on an AppImage build it is the AppDir's
# usr/bin, on MinGW it is artifacts/bin. Letting the tools follow it had two
# consequences, both bad. gf_module_packager and gf_module_keygen ended up
# INSIDE the AppDir, so a build tool would have shipped in the AppImage; and
# every caller outside CMake -- the CI audit, seal and finalize steps -- had to
# know which layout was in effect to find the tool at all, which is exactly the
# kind of thing that is discovered in CI rather than at the desk.
#
# One fixed path instead. These are host tools: they run on the machine doing
# the build, and where the application's files go is none of their business.
#
# Windows is the exception, and not by preference. gf_module_packager links
# gf_core, and Windows has no rpath: a DLL is found in the executable's own
# directory or on PATH, and nothing else. A tool in its own directory simply
# cannot start -- `0xc0000135`, STATUS_DLL_NOT_FOUND, from a ninja edge that
# says nothing about which DLL. ELF and Mach-O both carry a build rpath, so
# everywhere else the tools can live wherever is tidiest.
if(MINGW)
  set(GPGFRONTEND_HOST_TOOL_DIR "${CMAKE_BINARY_DIR}/artifacts/bin")
else()
  set(GPGFRONTEND_HOST_TOOL_DIR "${CMAKE_BINARY_DIR}/artifacts/tools")
endif()
message(STATUS "Host build tools: ${GPGFRONTEND_HOST_TOOL_DIR}")

# Published in build-info.json, with everything else a CI step needs.

# Put a target's output exactly where it is told, on every generator.
#
# A multi-config generator -- Xcode, Visual Studio -- appends the
# configuration name to RUNTIME_OUTPUT_DIRECTORY unless the per-config
# property is set too. So `.../native` quietly becomes
# `.../native/RelWithDebInfo`, which breaks two things that are not obviously
# related: the CI steps that look for a host tool at a fixed path, and module
# loading itself, because the Host requires a module's entry native to be a
# DIRECT child of its namespace's native directory.
#
# Single-config generators leave CMAKE_CONFIGURATION_TYPES empty and the loop
# does nothing, so this is free on Ninja and Makefiles.
function(gf_pin_output_directory target dir)
  # ARCHIVE as well as RUNTIME and LIBRARY. On MinGW the import library
  # (`*.dll.a`) is an ARCHIVE output, so leaving it unpinned scattered import
  # libraries into the old flat `artifacts/bin/modules/` while the DLLs went to
  # their namespaces -- files nothing cleans up and nothing needs.
  set_target_properties(${target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${dir}"
    LIBRARY_OUTPUT_DIRECTORY "${dir}"
    ARCHIVE_OUTPUT_DIRECTORY "${dir}")
  foreach(config IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER "${config}" config_upper)
    set_target_properties(${target} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${dir}"
      LIBRARY_OUTPUT_DIRECTORY_${config_upper} "${dir}"
      ARCHIVE_OUTPUT_DIRECTORY_${config_upper} "${dir}")
  endforeach()
endfunction()

# The module namespace root: one `<key>/{module.gfmodule,native/}` per module.
#
# Every layout but the AppImage uses the build tree's own artifacts directory,
# which is also what a development run discovers -- the dev tree IS the
# deployment tree, so a developer exercises the same resolution a user does.
if(NOT DEFINED GPGFRONTEND_MODULE_NAMESPACE_ROOT)
  set(GPGFRONTEND_MODULE_NAMESPACE_ROOT "${CMAKE_BINARY_DIR}/artifacts/modules")
endif()
message(STATUS "Module namespace root: ${GPGFRONTEND_MODULE_NAMESPACE_ROOT}")

# rpath
if(BUILD_STRIP_RPATH)
  set(CMAKE_SKIP_INSTALL_RPATH TRUE)
  unset(CMAKE_INSTALL_RPATH CACHE)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
else()
  if(XCODE_BUILD)
    set(CMAKE_MACOSX_RPATH TRUE)
    set(CMAKE_INSTALL_RPATH "@executable_path/../Frameworks")
  else()
    set(CMAKE_SKIP_INSTALL_RPATH FALSE)
    # Relative to the binary, not to the prefix that happened to be configured.
    #
    # This was "${CMAKE_INSTALL_PREFIX}/lib", which is frozen at configure
    # time -- so `cmake --install --prefix /somewhere/else` produced a tree
    # whose own binary looked for its libraries under the ORIGINAL prefix and
    # could not start. The module verification at the end of the install still
    # passed, because it runs the build tree's tool rather than the installed
    # application, so the failure only appeared when someone ran it.
    #
    # $ORIGIN/../lib covers <prefix>/bin -> <prefix>/lib, and the absolute
    # libdir stays for a multilib install where that is not the same place.
    set(CMAKE_INSTALL_RPATH
      "$ORIGIN/../lib;$ORIGIN/../${CMAKE_INSTALL_LIBDIR};${CMAKE_INSTALL_PREFIX}/lib")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
  endif()
endif()

# Before any subdirectory, because modules/ is configured ahead of src/ and its
# install rules need CMAKE_INSTALL_LIBDIR. Including it only in src/ meant the
# module install destination resolved against an empty variable.
include(GNUInstallDirs)

add_subdirectory(third_party)

# single source of truth for the UI locales shared by the app and all modules
include(${CMAKE_SOURCE_DIR}/cmake/Translations.cmake)

# modules are only for qt6
if(BUILD_MODULES AND NOT GPGFRONTEND_QT5_BUILD)
  # Before modules/, because every module links it.
  add_subdirectory(src/module_runtime)
  include(${CMAKE_SOURCE_DIR}/cmake/ModuleRegistry.cmake)
  add_subdirectory(modules)
endif()

# register_library() accumulates into this cache variable and nothing ever
# clears it, so a library that is removed or renamed lingers in an existing
# build directory and reappears in install(TARGETS ...) as a target that no
# longer exists. Reset it here, once, before anything registers itself.
set(GPGFRONTEND_LIBRARY_TARGETS "" CACHE INTERNAL "All libraries" FORCE)

add_subdirectory(src)
# ---------------------------------------------------------------------------
# One file that says what this build is and where it put things.
#
# It replaces four generated files and, more to the point, the places that
# ignored them: the module count was re-derived in eleven workflow shells, the
# host tool path in eight (each appending ".exe" by hand), the module root was
# hardcoded in seventeen, and the project version was scraped out of this very
# file with a regular expression.
#
# Deliberately dumb and internal. It is ephemeral build metadata -- not a
# runtime input, not signed, not a versioned public schema, not a compatibility
# surface. Nothing at runtime reads it and nothing should start.
#
# There is deliberately NO architecture here. CMake says `aarch64` where the
# project says `arm64`, and normalising it in CMake would be a second
# implementation of NormalizeManifestArch() that could drift from the first.
# `gf_module_packager host-info` already answers it, from that one
# implementation, and the two workflow steps that need it keep asking it.
#
# file(GENERATE) rather than file(WRITE): the library names are generator
# expressions, and only the generate step can resolve them.
set(gf_deployable_library_files)
foreach(gf_library_target IN LISTS GPGFRONTEND_LIBRARY_TARGETS)
  list(APPEND gf_deployable_library_files
    "\"$<TARGET_FILE_NAME:${gf_library_target}>\"")
endforeach()

set(gf_module_entries)
foreach(gf_pair IN LISTS GPGFRONTEND_MODULE_DIRECTORY_KEYS)
  string(FIND "${gf_pair}" "=" gf_at)
  string(SUBSTRING "${gf_pair}" 0 ${gf_at} gf_id)
  math(EXPR gf_at "${gf_at} + 1")
  string(SUBSTRING "${gf_pair}" ${gf_at} -1 gf_key)
  list(APPEND gf_module_entries "    \"${gf_id}\": \"${gf_key}\"")
endforeach()
list(LENGTH gf_module_entries GPGFRONTEND_MODULE_COUNT)
string(REPLACE ";" ",\n" gf_module_json "${gf_module_entries}")

file(GENERATE
  OUTPUT "${CMAKE_BINARY_DIR}/artifacts/build-info.json"
  CONTENT "{
  \"project_version\": \"${PROJECT_VERSION}\",
  \"build_id\": \"${GPGFRONTEND_BUILD_ID}\",
  \"host_os\": \"${_gf_build_os}\",
  \"host_tool_dir\": \"${GPGFRONTEND_HOST_TOOL_DIR}\",
  \"module_packager\": \"${GPGFRONTEND_HOST_TOOL_DIR}/gf_module_packager${CMAKE_EXECUTABLE_SUFFIX}\",
  \"module_namespace_root\": \"${GPGFRONTEND_MODULE_NAMESPACE_ROOT}\",
  \"module_count\": ${GPGFRONTEND_MODULE_COUNT},
  \"modules\": {
${gf_module_json}
  },
  \"deployable_libraries\": [$<JOIN:${gf_deployable_library_files},, >]
}
")
