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
set(_export_file "${CMAKE_CURRENT_SOURCE_DIR}/GpgFrontendCoreExport.h")
generate_export_header(gpgfrontend_core EXPORT_FILE_NAME "${_export_file}")

target_link_libraries(gpgfrontend_core PUBLIC mimalloc)

# link third-party libraries
target_link_libraries(gpgfrontend_core PUBLIC config++)
if (NOT LINUX)
    target_link_libraries(gpgfrontend_core PUBLIC config++ intl)
endif ()

# qt-aes
target_sources(gpgfrontend_core PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/qt-aes/qaesencryption.cpp)

# encoding detect library
aux_source_directory(${CMAKE_SOURCE_DIR}/third_party/encoding-detect ENCODING_DETECT_SOURCE_CODE)
target_sources(gpgfrontend_core PUBLIC ${ENCODING_DETECT_SOURCE_CODE})

# icu
if (APPLE)
    target_include_directories(gpgfrontend_core PRIVATE /usr/local/opt/icu4c/include)
    target_link_directories(gpgfrontend_core PRIVATE /usr/local/opt/icu4c/lib)
    target_link_libraries(gpgfrontend_core PRIVATE icui18n icuuc icudata)
else ()
    find_package(ICU REQUIRED COMPONENTS i18n uc data)
    target_include_directories(gpgfrontend_core PRIVATE ${ICU_INCLUDE_DIRS})
    target_link_libraries(gpgfrontend_core PRIVATE ${ICU_LIBRARIES})
endif ()

# link gnupg libraries
target_link_libraries(gpgfrontend_core PRIVATE gpgme assuan gpg-error)
# link openssl
target_link_libraries(gpgfrontend_core PUBLIC OpenSSL::SSL OpenSSL::Crypto)
# link boost libraries
target_link_libraries(gpgfrontend_core PUBLIC ${Boost_LIBRARIES})
if (MINGW)
    # for uuid ability in mingw
    target_link_libraries(gpgfrontend_core PUBLIC bcrypt)
endif ()

# spdlog
target_link_libraries(gpgfrontend_core PRIVATE spdlog)

# link libarchive
if(APPLE)
    set(LibArchive_INCLUDE_DIR "/usr/local/opt/libarchive/include")
endif()
find_package(LibArchive REQUIRED)
target_include_directories(gpgfrontend_core PRIVATE ${LibArchive_INCLUDE_DIR})
target_link_libraries(gpgfrontend_core PRIVATE archive)

# link json
target_link_libraries(gpgfrontend_core
        PUBLIC nlohmann_json::nlohmann_json)
        
# link Qt core
if(Qt6_DIR)
    target_link_libraries(gpgfrontend_core PUBLIC Qt6::Core)
else()
    target_link_libraries(gpgfrontend_core PUBLIC Qt5::Core)
endif()

# set up pch
target_precompile_headers(gpgfrontend_core
        PUBLIC ${CMAKE_SOURCE_DIR}/src/GpgFrontend.h
        PUBLIC GpgFrontendCore.h
        PUBLIC GpgConstants.h)

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
