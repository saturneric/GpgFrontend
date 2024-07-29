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

# gather source files
aux_source_directory(./function/result_analyse CORE_SOURCE)
aux_source_directory(./function/basic CORE_SOURCE)
aux_source_directory(./function/gpg CORE_SOURCE)
aux_source_directory(./function/secure_memory CORE_SOURCE)
aux_source_directory(./function CORE_SOURCE)
aux_source_directory(./thread CORE_SOURCE)
aux_source_directory(./model CORE_SOURCE)
aux_source_directory(./common CORE_SOURCE)
aux_source_directory(./module CORE_SOURCE)
aux_source_directory(./utils/aes CORE_SOURCE)
aux_source_directory(./utils CORE_SOURCE)
aux_source_directory(. CORE_SOURCE)

# define libgpgfrontend_core
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

add_library(gpgfrontend_core SHARED ${CORE_SOURCE})

# generate headers
set(_export_file "${CMAKE_CURRENT_SOURCE_DIR}/GpgFrontendCoreExport.h")
generate_export_header(gpgfrontend_core EXPORT_FILE_NAME "${_export_file}")

# compile definitions
target_compile_definitions(gpgfrontend_core PUBLIC GF_CORE)

# mimalloc (except apple macos)
if(NOT APPLE)
  target_link_libraries(gpgfrontend_core PUBLIC mimalloc)
endif()

# qt-aes
target_sources(gpgfrontend_core PRIVATE
  ${CMAKE_SOURCE_DIR}/third_party/qt-aes/qaesencryption.cpp)

# encoding detect library
aux_source_directory(${CMAKE_SOURCE_DIR}/third_party/encoding-detect ENCODING_DETECT_SOURCE_CODE)
target_sources(gpgfrontend_core PUBLIC ${ENCODING_DETECT_SOURCE_CODE})

# link gnupg libraries
target_link_libraries(gpgfrontend_core PUBLIC gpgme assuan gpg-error)

# link openssl
target_link_libraries(gpgfrontend_core PUBLIC OpenSSL::SSL OpenSSL::Crypto)

if(MINGW)
  # for uuid ability in mingw
  target_link_libraries(gpgfrontend_core PUBLIC bcrypt)
endif()

# configure libarchive
if(APPLE)
  if(EXISTS "/usr/local/opt/libarchive/include")
    set(LibArchive_INCLUDE_DIR "/usr/local/opt/libarchive/include")
  else()
    set(LibArchive_INCLUDE_DIR "/opt/homebrew/opt/libarchive/include")
  endif()
endif()

find_package(LibArchive REQUIRED)
target_include_directories(gpgfrontend_core PRIVATE ${LibArchive_INCLUDE_DIR})

# link libarchive
target_link_libraries(gpgfrontend_core PRIVATE archive)

if(GPGFRONTEND_QT5_BUILD)
  # link Qt core
  target_link_libraries(gpgfrontend_core PUBLIC Qt5::Core)
else()
  # link Qt core
  target_link_libraries(gpgfrontend_core PUBLIC Qt6::Core)
endif()

# set up pch
target_precompile_headers(gpgfrontend_core
  PUBLIC ${CMAKE_SOURCE_DIR}/src/GpgFrontend.h
  PUBLIC GpgFrontendCore.h
  PUBLIC GpgConstants.h)

# using std c++ 17
target_compile_features(gpgfrontend_core PUBLIC cxx_std_17)

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  # lib output path
  set_target_properties(gpgfrontend_core PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lib)
endif()

# link for different platforms
if(MINGW)
  message(STATUS "Link GPG Static Library For MINGW")
  target_link_libraries(gpgfrontend_core PUBLIC wsock32)
elseif(APPLE)
  message(STATUS "Link GPG Static Library For macOS")

  if(XCODE_BUILD)
    set_target_properties(gpgfrontend_core
      PROPERTIES
      ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
      XCODE_ATTRIBUTE_SKIP_INSTALL "Yes"
      XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${GPGFRONTEND_XOCDE_CODE_SIGN_IDENTITY}")
  endif()
else()
  # linux
  message(STATUS "Link GPG Static Library For Unix")
  target_link_libraries(gpgfrontend_core PUBLIC pthread dl)
endif()
