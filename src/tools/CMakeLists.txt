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

# gf_core alone. This used to have to link gf_sdk too -- with --no-as-needed
# to stop the linker dropping it again -- because gf_core referenced four
# gf_sdk entry points and left them unresolved. It does not any more: those
# four arrive through ModuleSdkBridge, so gf_core is self-contained and this
# is an ordinary link.
target_link_libraries(gf_module_packager PRIVATE gf_core Qt::Core)

set_target_properties(gf_module_packager PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${GPGFRONTEND_HOST_TOOL_DIR})

# Not installed: a packager is a thing a build runs, not a thing a user gets.

