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
include(CTest)
enable_testing()
find_package(GTest REQUIRED)

aux_source_directory(./core TEST_SOURCE)
aux_source_directory(./core/gpg TEST_SOURCE)
aux_source_directory(./core/rpgp TEST_SOURCE)
aux_source_directory(./ui TEST_SOURCE)
aux_source_directory(. TEST_SOURCE)

# define test library
register_library(test LIBRARY_TARGET ${TEST_SOURCE})

# the libsecret candidate ordering is what makes an AppImage able to reach the
# system keychain, so it is covered by tests. Only this translation unit is
# pulled in, never LinuxSecretStore.cpp: that one can register a live backend
# and start writing to the developer's own keyring.
if(UNIX AND NOT APPLE)
  target_sources(${LIBRARY_TARGET} PRIVATE
    ${CMAKE_SOURCE_DIR}/src/platform/LibSecretSearchPath.cpp)
endif()

# set up pch
target_precompile_headers(${LIBRARY_TARGET} PUBLIC GpgFrontendTest.h)

# link options
target_link_libraries(${LIBRARY_TARGET} PRIVATE GTest::gtest)
target_link_libraries(${LIBRARY_TARGET} PRIVATE gf_core)

# the ui tests cover the settings page registry and its ordering rules; gf_ui
# already links gf_core, so this is purely additive
target_link_libraries(${LIBRARY_TARGET} PRIVATE gf_ui)

# the sdk tests cover the exported C ABI's string-ownership rules, which modules
# depend on and cannot check for themselves; gf_sdk already links gf_core and
# gf_ui, so this is purely additive
target_link_libraries(${LIBRARY_TARGET} PRIVATE gf_sdk)

add_test(AllTestsInGpgFrontend ${LIBRARY_TARGET})

# A shared library that writes a file when it is mapped.
#
# It backs the one assertion about module packages that cannot be made any
# other way: that a refused package never has its code executed. Asserting only
# that loading returned an error would pass even if the image had already been
# mapped and its initialisers run, so the test needs something whose loading is
# observable from outside the process that loaded it.
#
# Deliberately not a module: it exports no entry point, and the host refuses it
# as one. What is under test is whether its constructor ran.
add_library(gf_mod_test_sentinel SHARED sentinel_module/sentinel.cpp)

set_target_properties(gf_mod_test_sentinel PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  AUTOMOC OFF
  AUTOUIC OFF
  AUTORCC OFF
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/artifacts/test-modules)

# Not installed: a test fixture is not something a user gets.
