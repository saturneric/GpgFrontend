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

# com.bktus.gpgfrontend.module.integrated.version-checking

aux_source_directory(. INTEGRATED_MODULE_SOURCE)

# define libgpgfrontend_module
add_library(gpgfrontend_integrated_module_version_checking SHARED ${INTEGRATED_MODULE_SOURCE})
set(_export_file "${CMAKE_CURRENT_SOURCE_DIR}/GpgFrontendModuleExport.h")
generate_export_header(gpgfrontend_integrated_module_version_checking EXPORT_FILE_NAME "${_export_file}")

if (XCODE_BUILD)
  set_target_properties(gpgfrontend_integrated_module_version_checking
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    XCODE_ATTRIBUTE_SKIP_INSTALL "Yes"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${GPGFRONTEND_XOCDE_CODE_SIGN_IDENTITY}")
endif ()

# link sdk
target_link_libraries(gpgfrontend_integrated_module_version_checking PRIVATE
        gpgfrontend_module_sdk)

# link Qt
target_link_libraries(gpgfrontend_integrated_module_version_checking PRIVATE Qt6::Network)

# property
set_property(TARGET gpgfrontend_integrated_module_version_checking PROPERTY AUTOMOC ON)

# using std c++ 17
target_compile_features(gpgfrontend_integrated_module_version_checking PRIVATE cxx_std_17)