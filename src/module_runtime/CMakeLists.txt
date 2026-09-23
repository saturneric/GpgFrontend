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

# The module-side framework, linked statically into every module.
#
# It owns the per-module runtime STATE: the SDK context, module identity and
# bootstrap facts, the hook table, event dispatch. It is not the SDK. The
# public GFSDK* functions live in gf_sdk, which is stateless and which this
# links like any other consumer.
#
# Deliberately NOT built with register_library(): that helper hardwires
# SHARED, a VERSION, generate_export_header() and a PUBLIC precompiled
# header, none of which a static archive consumed through plain headers
# wants, and it appends to the list of deployable libraries.

add_library(gf_module_runtime STATIC
  GFModuleRuntime.cpp
  GFModuleRuntimeBoot.cpp
  GFModuleRuntimeCommand.cpp
  GFModuleRuntimeDispatch.cpp
  GFModuleRuntimeI18n.cpp)

set_target_properties(gf_module_runtime PROPERTIES
  POSITION_INDEPENDENT_CODE ON   # it is linked into shared objects
  CXX_VISIBILITY_PRESET hidden   # nothing here is exported from a module
  VISIBILITY_INLINES_HIDDEN 1
  AUTOMOC OFF)                   # no QObject lives here; keep it that way

target_compile_features(gf_module_runtime PUBLIC cxx_std_17)

# Qt is a usage requirement, not a host dependency. The runtime's own sources
# use QString, QSet and QByteArray; they were compiling only because the
# global include_directories() in the top-level CMakeLists happens to put Qt
# on the path, which is an accident rather than a statement.
target_link_libraries(gf_module_runtime PUBLIC Qt::Core gf_sdk)

# The SDK headers arrive through gf_sdk's own interface, not by naming a
# path here: a module sees the public C ABI and Qt, and nothing else from
# this tree.
target_include_directories(gf_module_runtime PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
