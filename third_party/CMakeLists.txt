# Copyright (C) 2021 Saturneric <eric@bktus.com>
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

# json
set(JSON_BuildTests OFF CACHE INTERNAL "")

if(MINGW)
  add_subdirectory(libarchive EXCLUDE_FROM_ALL)
endif()

if(NOT APPLE)
  set(MI_SECURE ON)

  # fix the segment fault issue on M1 chip platform
  # refer to https://github.com/microsoft/mimalloc/issues/343
  # if(APPLE)
  # set(MI_OSX_ZONE ON)
  # set(MI_OSX_INTERPOSE OFF)
  # endif()

  # ASAN checking
  if(${CMAKE_BUILD_TYPE} STREQUAL "Debug" AND ENABLE_ASAN)
    # if(APPLE)
    # set(MI_OVERRIDE OFF)
    # endif()

    # set(MI_TRACK_VALGRIND ON)
    set(MI_TRACK_ASAN ON)
  endif()

  add_subdirectory(mimalloc EXCLUDE_FROM_ALL)
endif()

set(INSTALL_GTEST OFF)
add_subdirectory(googletest EXCLUDE_FROM_ALL)