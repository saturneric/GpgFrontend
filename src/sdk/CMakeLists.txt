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

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# define libgpgfrontend_module_sdk
aux_source_directory(. MODULE_SDK_SOURCE)
aux_source_directory(private MODULE_SDK_SOURCE)

add_library(gpgfrontend_module_sdk SHARED ${MODULE_SDK_SOURCE})
set(_export_file_sdk "${CMAKE_CURRENT_SOURCE_DIR}/GFSDKExport.h")
generate_export_header(gpgfrontend_module_sdk EXPORT_FILE_NAME "${_export_file_sdk}")
target_include_directories(gpgfrontend_module_sdk PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/gpgfrontend_module_sdk_autogen/include)

target_include_directories(gpgfrontend_module_sdk PUBLIC sdk)

# link module system
target_link_libraries(gpgfrontend_module_sdk PRIVATE gpgfrontend_core gpgfrontend_ui)

# sdk export headers
file(GLOB _headerPath "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
set_target_properties(gpgfrontend_module_sdk PROPERTIES PUBLIC_HEADER "${_headerPath}")

if(XCODE_BUILD)
  set_target_properties(gpgfrontend_module_sdk
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    XCODE_ATTRIBUTE_SKIP_INSTALL "Yes"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${GPGFRONTEND_XOCDE_CODE_SIGN_IDENTITY}")
endif()