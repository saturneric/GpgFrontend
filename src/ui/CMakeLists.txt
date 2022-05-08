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
aux_source_directory(keypair_details UI_SOURCE)
aux_source_directory(widgets UI_SOURCE)
aux_source_directory(key_generate UI_SOURCE)
aux_source_directory(main_window UI_SOURCE)
aux_source_directory(help UI_SOURCE)
aux_source_directory(settings UI_SOURCE)
aux_source_directory(thread UI_SOURCE)
aux_source_directory(details UI_SOURCE)
aux_source_directory(struct UI_SOURCE)
aux_source_directory(import_export UI_SOURCE)
aux_source_directory(dialog UI_SOURCE)

# define libgpgfrontend_ui
add_library(gpgfrontend_ui SHARED ${UI_SOURCE})

# link Qt
target_link_libraries(gpgfrontend_ui
        Qt5::Network Qt5::PrintSupport Qt5::Widgets Qt5::Test Qt5::Core)

# link gpgfrontend_core
target_link_libraries(gpgfrontend_ui
        gpgfrontend_core)

# set up pch
target_precompile_headers(gpgfrontend_ui PUBLIC GpgFrontendUI.h)

# add ui generator include path
target_include_directories(gpgfrontend_ui PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/gpgfrontend_ui_autogen/include)

# using std c++ 17
target_compile_features(gpgfrontend_ui PUBLIC cxx_std_17)

# for xcode archive build
if (XCODE_BUILD)
    set_target_properties(gpgfrontend_ui
            PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
endif ()
target_compile_features(gpgfrontend_ui PUBLIC cxx_std_17)