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

aux_source_directory(. MODULE_SDK_SOURCE)
aux_source_directory(private MODULE_SDK_SOURCE)

# define sdk
register_library(sdk LIBRARY_TARGET ${MODULE_SDK_SOURCE})
target_include_directories(${LIBRARY_TARGET} PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY_TARGET}_autogen/include)

# headers
target_include_directories(${LIBRARY_TARGET} PUBLIC sdk)

# link module system
target_link_libraries(${LIBRARY_TARGET} PRIVATE gf_core gf_ui)
# Proves the public SDK headers are consumable from C.
#
# Its own OBJECT target, compiled as C11, so it inherits neither the C++ PCH
# nor gf_sdk's include interface: a C++-only construct creeping into a public
# header fails the BUILD rather than being noticed in review. GF_SDK_EXPORT is
# stubbed empty, following modules/src/m_email/test/CMakeLists.txt.
add_library(gf_sdk_c_abi_check OBJECT
  ${CMAKE_CURRENT_SOURCE_DIR}/test/sdk_c_abi_check.c)

set_target_properties(gf_sdk_c_abi_check PROPERTIES
  C_STANDARD 11
  C_STANDARD_REQUIRED ON
  POSITION_INDEPENDENT_CODE ON)

target_compile_definitions(gf_sdk_c_abi_check PRIVATE GF_SDK_EXPORT=)

target_include_directories(gf_sdk_c_abi_check PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR})

# Built as part of the normal build, so the check cannot be skipped.
add_dependencies(${LIBRARY_TARGET} gf_sdk_c_abi_check)
