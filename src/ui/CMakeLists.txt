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

# tracking source files
aux_source_directory(. UI_SOURCE)
aux_source_directory(dialog/keypair_details UI_SOURCE)
aux_source_directory(widgets UI_SOURCE)
aux_source_directory(dialog/key_generate UI_SOURCE)
aux_source_directory(main_window UI_SOURCE)
aux_source_directory(dialog/help UI_SOURCE)
aux_source_directory(dialog/settings UI_SOURCE)
aux_source_directory(thread UI_SOURCE)
aux_source_directory(dialog/details UI_SOURCE)
aux_source_directory(struct UI_SOURCE)
aux_source_directory(dialog/import_export UI_SOURCE)
aux_source_directory(dialog UI_SOURCE)

# define libgpgfrontend_ui
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
add_library(gpgfrontend_ui SHARED ${UI_SOURCE})
set(_export_file "${CMAKE_CURRENT_SOURCE_DIR}/GpgFrontendUIExport.h")
generate_export_header(gpgfrontend_ui EXPORT_FILE_NAME "${_export_file}")

# link Qt
if(Qt6_DIR)
        target_link_libraries(gpgfrontend_ui
                Qt6::Network Qt6::PrintSupport Qt6::Widgets Qt6::Test Qt6::Core5Compat Qt6::Core)
else()
        target_link_libraries(gpgfrontend_ui
                Qt5::Network Qt5::PrintSupport Qt5::Widgets Qt5::Test Qt5::Core)
endif()

# link gpgfrontend_core
target_link_libraries(gpgfrontend_ui
        gpgfrontend_core)

# set up pch
target_precompile_headers(gpgfrontend_ui PUBLIC GpgFrontendUI.h)

# add ui generator include path
target_include_directories(gpgfrontend_ui PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/gpgfrontend_ui_autogen/include
        ${CMAKE_SOURCE_DIR}/third_party/spdlog/include)

# using std c++ 17
target_compile_features(gpgfrontend_ui PUBLIC cxx_std_17)

# for xcode archive build
if (XCODE_BUILD)
    set_target_properties(gpgfrontend_ui
            PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
            XCODE_ATTRIBUTE_SKIP_INSTALL "Yes"
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${GPGFRONTEND_XOCDE_CODE_SIGN_IDENTITY}")
endif ()
target_compile_features(gpgfrontend_ui PUBLIC cxx_std_17)