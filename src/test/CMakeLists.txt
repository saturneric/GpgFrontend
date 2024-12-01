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

# Set configure for test

aux_source_directory(./core TEST_SOURCE)
aux_source_directory(. TEST_SOURCE)

# define test library
add_library(gpgfrontend_test SHARED ${TEST_SOURCE})

# generate headers
set(_export_file "${CMAKE_CURRENT_SOURCE_DIR}/GpgFrontendTestExport.h")
generate_export_header(gpgfrontend_test EXPORT_FILE_NAME "${_export_file}")

# compile definitions
target_compile_definitions(gpgfrontend_test PRIVATE GF_TEST_PRIVATE)

# link options
target_link_libraries(gpgfrontend_test PRIVATE GTest::gtest)
target_link_libraries(gpgfrontend_test PRIVATE gpgfrontend_core)

if(XCODE_BUILD)
  set_target_properties(gpgfrontend_test
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    XCODE_ATTRIBUTE_SKIP_INSTALL "Yes"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${GPGFRONTEND_XOCDE_CODE_SIGN_IDENTITY}")
endif()

add_test(AllTestsInGpgFrontend gpgfrontend_test)
