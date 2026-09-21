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
# Deliberately NOT built with register_library(): that helper hardwires SHARED,
# a VERSION, generate_export_header() and a PUBLIC precompiled header, none of
# which a static archive consumed through plain headers wants. It is also
# deliberately not linked against gf_sdk -- it DECLARES the SDK symbols it
# calls and leaves them undefined, so the module resolves them, exactly as
# module code does today. That is what keeps the runtime linkable into a test
# binary with hand-written SDK stubs.

add_library(gf_module_runtime STATIC
  GFModuleRuntime.cpp
  GFModuleRuntimeBoot.cpp
  GFModuleRuntimeDispatch.cpp
  GFModuleRuntimeI18n.cpp
  # The module-side SDK. These DEFINE the public GFSDK* functions, over the
  # capability groups the host granted; a module links this archive instead of
  # gf_sdk, which is what makes the boundary enforceable at link time.
  sdk/GFSdkApp.cpp
  sdk/GFSdkBuffer.cpp
  sdk/GFSdkCommon.cpp
  sdk/GFSdkEvent.cpp
  sdk/GFSdkGpg.cpp
  sdk/GFSdkLog.cpp
  sdk/GFSdkProcess.cpp
  sdk/GFSdkStorage.cpp
  sdk/GFSdkUi.cpp)

set_target_properties(gf_module_runtime PROPERTIES
  POSITION_INDEPENDENT_CODE ON   # it is linked into shared objects
  CXX_VISIBILITY_PRESET hidden   # nothing here is exported from a module
  VISIBILITY_INLINES_HIDDEN 1
  AUTOMOC OFF)                   # no QObject lives here; keep it that way

target_compile_features(gf_module_runtime PUBLIC cxx_std_17)
F

target_include_directories(gf_module_runtime PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/sdk>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src/sdk>)
