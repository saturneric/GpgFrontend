#
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
#
#

# The SDK, in three pieces that must not be confused with one another.
#
#   gf_sdk_headers   the public C ABI. Types, the primitive tables, and the
#                    public SDK declarations. No code.
#   gf_sdk           the public SDK, implemented MODULE-side over GFHostApi.
#                    Stateless. Links Qt and nothing else from this tree.
#   gf_host_api      the primitive implementations. HOST-side, may include
#                    core/ and ui/, compiled into the application.
#
# The rule they exist to express:
#
#   module -> public SDK(ctx) -> GFHostApi primitive -> host internals
#
# A module links gf_sdk and gf_module_runtime. It cannot link gf_host_api,
# gf_core or gf_ui, and gf_add_module() refuses to build one that tries.

# ---------------------------------------------------------------------------
# gf_sdk_headers -- the public C ABI
# ---------------------------------------------------------------------------
#
# An INTERFACE target so both halves inherit the include directory from a
# common parent rather than from each other. It is what lets the module side
# be given the SDK headers WITHOUT the global src/ include directory, which is
# the earliest of the boundary checks: `#include "core/..."` in module-side
# code then fails to compile rather than linking against something it should
# never see.
add_library(gf_sdk_headers INTERFACE)
target_include_directories(gf_sdk_headers INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

add_subdirectory(api)

# ---------------------------------------------------------------------------
# gf_host_api -- the primitive implementations
# ---------------------------------------------------------------------------
#
# OBJECT rather than SHARED, and the difference matters. The bridge that lets
# gf_core call in used to be installed from a load-time initializer, justified
# by a module's DT_NEEDED dragging libgf_sdk into the process. Modules no
# longer link this at all, so the only thing keeping it there would be the
# application's link line -- and --as-needed can drop a library from which no
# symbol is referenced, taking the initializer with it. One explicit call from
# application startup is deterministic, and caller and implementation then sit
# in the same final link unit, so nothing here needs to be exported.
#
# Exactly ONE binary may link this: the registries and the context mint are
# process-wide state, and a second copy would mean a second registry.
aux_source_directory(host GF_HOST_API_SOURCE)
aux_source_directory(host/private GF_HOST_API_SOURCE)

add_library(gf_host_api OBJECT ${GF_HOST_API_SOURCE})

set_target_properties(gf_host_api PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON)

target_compile_features(gf_host_api PRIVATE cxx_std_20)
target_include_directories(gf_host_api PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/host)
target_link_libraries(gf_host_api PUBLIC gf_sdk_headers)
target_link_libraries(gf_host_api PRIVATE gf_core gf_ui)

# ---------------------------------------------------------------------------
# Proves the public SDK headers are consumable from C.
# ---------------------------------------------------------------------------
#
# Its own OBJECT target, compiled as C11, so it inherits neither a C++ PCH nor
# any include interface: a C++-only construct creeping into a public header
# fails the BUILD rather than being noticed in review.
add_library(gf_sdk_c_abi_check OBJECT
  ${CMAKE_CURRENT_SOURCE_DIR}/test/sdk_c_abi_check.c)

set_target_properties(gf_sdk_c_abi_check PROPERTIES
  C_STANDARD 11
  C_STANDARD_REQUIRED ON
  POSITION_INDEPENDENT_CODE ON)

target_include_directories(gf_sdk_c_abi_check PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR})

add_dependencies(gf_host_api gf_sdk_c_abi_check)
