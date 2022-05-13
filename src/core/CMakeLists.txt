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

aux_source_directory(./function/result_analyse GPG_SOURCE)
aux_source_directory(./function/gpg GPG_SOURCE)
aux_source_directory(./function/aes GPG_SOURCE)
aux_source_directory(./function GPG_SOURCE)
aux_source_directory(./thread GPG_SOURCE)
aux_source_directory(./model GPG_SOURCE)
aux_source_directory(./common GPG_SOURCE)
aux_source_directory(. GPG_SOURCE)

# define libgpgfrontend_core
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
add_library(gpgfrontend_core SHARED ${GPG_SOURCE})
set(_export_file "${CMAKE_CURRENT_SOURCE_DIR}/GpgFrontendCoreExport.h")
generate_export_header(gpgfrontend_core EXPORT_FILE_NAME "${_export_file}")

set(UTILS_DIR ${CMAKE_SOURCE_DIR}/utils)
set(GPGME_LIB_DIR ${UTILS_DIR}/gpgme/lib)

# link third-party libraries
target_link_libraries(gpgfrontend_core PUBLIC config++)
if (NOT LINUX)
    target_link_libraries(gpgfrontend_core PUBLIC config++ intl)
endif ()

# easyloggingpp
target_include_directories(gpgfrontend_core PUBLIC
        ${CMAKE_SOURCE_DIR}/third_party/easyloggingpp/src)
target_sources(gpgfrontend_core PUBLIC
        ${CMAKE_SOURCE_DIR}/third_party/easyloggingpp/src/easylogging++.cc)
# qt-aes
target_sources(gpgfrontend_core PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/qt-aes/qaesencryption.cpp)

# encoding detect library
aux_source_directory(${CMAKE_SOURCE_DIR}/third_party/encoding-detect ENCODING_DETECT_SOURCE_CODE)
target_sources(gpgfrontend_core PUBLIC ${ENCODING_DETECT_SOURCE_CODE})

# icu
if(APPLE)
    target_include_directories(gpgfrontend_core PRIVATE /usr/local/opt/icu4c/include)
    target_link_directories(gpgfrontend_core PRIVATE /usr/local/opt/icu4c/lib)
    target_link_libraries(gpgfrontend_core PRIVATE icui18n icuuc icudata)
else()
    find_package(ICU 60.0 REQUIRED COMPONENTS i18n)
    target_link_libraries(gpgfrontend_core PRIVATE ICU::i18n)
endif()

# link gnupg libraries
target_link_libraries(gpgfrontend_core PUBLIC gpgme assuan gpg-error)
# link openssl
target_link_libraries(gpgfrontend_core PUBLIC OpenSSL::SSL OpenSSL::Crypto)
# link boost libraries
target_link_libraries(gpgfrontend_core PUBLIC ${Boost_LIBRARIES})

# link libarchive
target_link_libraries(gpgfrontend_core PRIVATE archive)

# link json
target_link_libraries(gpgfrontend_core
        PUBLIC nlohmann_json::nlohmann_json)
# link Qt core
target_link_libraries(gpgfrontend_core PUBLIC Qt5::Core)

# set up pch
target_precompile_headers(gpgfrontend_core
        PUBLIC ${CMAKE_SOURCE_DIR}/src/GpgFrontend.h
        PUBLIC GpgFrontendCore.h)

# using std c++ 17
target_compile_features(gpgfrontend_core PUBLIC cxx_std_17)

# link for different platforms
if (MINGW)
    message(STATUS "Link GPG Static Library For MINGW")
    target_link_libraries(gpgfrontend_core PUBLIC wsock32)
elseif (APPLE)
    message(STATUS "Link GPG Static Library For macOS")
    target_link_libraries(gpgfrontend_core PUBLIC dl)
    if (XCODE_BUILD)
        set_target_properties(gpgfrontend_core
                PROPERTIES
                ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
                LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
                LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
                XCODE_ATTRIBUTE_SKIP_INSTALL "Yes"
                XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${GPGFRONTEND_XOCDE_CODE_SIGN_IDENTITY}")
    endif ()
else ()
    # linux
    message(STATUS "Link GPG Static Library For Unix")
    target_link_libraries(gpgfrontend_core PUBLIC pthread dl)
endif ()
