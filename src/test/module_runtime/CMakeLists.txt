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

# Everything about this module -- id, version, name, description, author,

# gf_module_runtime and gf_sdk, tested against a host that only records what
# it was asked to do. This is possible because both reach the host only
# through the GFHostApi table handed to activate(): the stubs supply a fake
# table (ModuleRuntimeStubs.cpp), so every GFSDK* call runs the real wrapper
# code and lands in the recorder.

add_executable(gf_module_runtime_test
  ModuleRuntimeTest.cpp
  ModuleRuntimeStubs.cpp)

target_link_libraries(gf_module_runtime_test PRIVATE
  GTest::gtest GTest::gtest_main Qt::Core gf_module_runtime)

# Only the module export macro needs neutering: the SDK has no export macro
# any more, because the public SDK is not in a shared library.
target_compile_definitions(gf_module_runtime_test PRIVATE
  "GF_MODULE_EXPORT=")

set_target_properties(gf_module_runtime_test PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test-bin"
  AUTOMOC OFF)
