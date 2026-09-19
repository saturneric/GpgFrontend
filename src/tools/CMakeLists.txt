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

# Host-side build tooling. These are not part of the application: they run on
# the machine doing the build, which for a cross-compiled target is not the
# machine the application will run on. Keeping their dependencies shallow is
# what would let a cross build supply a separately-built native copy without
# rebuilding the whole application for the host.

add_executable(gf_module_packager module_packager/main.cpp)

# gf_sdk as well as gf_core: gf_core's module loader calls GFGetHostApi(),
# which lives in the sdk, and a shared library leaves that unresolved until
# something links both.
#
# --no-as-needed is required rather than tidy. Nothing in the packager itself
# calls into the sdk, so a linker with --as-needed (the default on most Linux
# distributions) drops the gf_sdk dependency it was just given and then fails
# on the very symbol that dependency was there to supply.
if(UNIX AND NOT APPLE)
  target_link_libraries(gf_module_packager PRIVATE
    -Wl,--no-as-needed gf_sdk -Wl,--as-needed)
else()
  target_link_libraries(gf_module_packager PRIVATE gf_sdk)
endif()

target_link_libraries(gf_module_packager PRIVATE gf_core Qt::Core)

set_target_properties(gf_module_packager PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# Not installed: a packager is a thing a build runs, not a thing a user gets.

