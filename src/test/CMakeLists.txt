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

# Where CMake actually writes the module key list, and how many modules it
# registered.
#
# The agreement test between gf_module_directory_key() and ModuleDirectoryKey()
# used to look beside the test binary, which is `artifacts/` on Ninja and
# `artifacts/bin`, `AppDir/usr/bin` or `build/<config>` everywhere else -- so on
# three of four layouts the only check pinning the two implementations together
# skipped itself and reported success. The path is a fact CMake knows; it is
# passed in rather than guessed at, and an absent file is now a failure instead
# of a skip.
list(LENGTH GPGFRONTEND_MODULE_DIRECTORY_KEYS GF_REGISTERED_MODULE_COUNT)
target_compile_definitions(${LIBRARY_TARGET} PRIVATE
  GF_BUILD_INFO_FILE="${CMAKE_BINARY_DIR}/artifacts/build-info.json"
  GF_REGISTERED_MODULE_COUNT=${GF_REGISTERED_MODULE_COUNT}
  GF_TEST_SENTINEL_FILE="$<TARGET_FILE:gf_mod_test_sentinel>")

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
  AUTORCC OFF)

# Pinned, not LIBRARY_OUTPUT_DIRECTORY alone.
#
# On Windows a SHARED library's .dll is a RUNTIME output -- LIBRARY_OUTPUT_
# DIRECTORY governs the import library there and nothing else -- so setting
# only that put this fixture in artifacts/bin beside the real libraries, where
# the Windows packaging step's `libgf_*.dll` glob handed it to windeployqt6.
# It is not a Qt executable, windeployqt6 said so, and the build stopped. On
# Linux the .so IS a LIBRARY output, which is why this only ever broke on one
# platform.
# Outside artifacts/, because on Windows artifacts/ IS the payload and this
# fixture is deliberately named so the module machinery treats it as a module
# -- which is exactly why the release gate reports it as a module-shaped
# library outside every namespace. It was being deleted again during staging;
# not putting it there is simpler than removing it afterwards.
gf_pin_output_directory(gf_mod_test_sentinel
  "${CMAKE_BINARY_DIR}/test-bin")

# Not installed: a test fixture is not something a user gets.

# The module-side runtime is only built when modules are, and it is tested
# against stubs rather than against the host -- so it gets its own target
# rather than linking into gf_test.
if(TARGET gf_module_runtime)
  add_subdirectory(module_runtime)
endif()
