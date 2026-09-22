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

# gf_sdk -- the public SDK, module-side and stateless.
#
# Its own directory, and that is load-bearing rather than tidy: the top-level
# CMakeLists calls include_directories(${CMAKE_SOURCE_DIR}/src), which every
# subdirectory inherits, and which would put core/ and ui/ headers on this
# target's include path. Clearing the directory property here means
# `#include "core/utils/CommonUtils.h"` in a file below fails to COMPILE,
# with "no such file", at the moment it is typed. That is the earliest and
# clearest of the boundary checks, and the only one that catches the mistake
# before it has been made.
set_property(DIRECTORY PROPERTY INCLUDE_DIRECTORIES "")

aux_source_directory(. GF_SDK_API_SOURCE)

add_library(gf_sdk STATIC ${GF_SDK_API_SOURCE})

set_target_properties(gf_sdk PROPERTIES
  POSITION_INDEPENDENT_CODE ON   # it is linked into shared objects
  CXX_VISIBILITY_PRESET hidden   # a module exports GFModuleGetApi and nothing else
  VISIBILITY_INLINES_HIDDEN ON
  AUTOMOC OFF)                   # no QObject lives here; keep it that way

target_compile_features(gf_sdk PUBLIC cxx_std_17)

# The public headers, and Qt for the handful of places a wrapper formats or
# converts. Qt is not a host dependency: it is the same library a module
# already links for itself.
target_link_libraries(gf_sdk PUBLIC gf_sdk_headers Qt::Core)
