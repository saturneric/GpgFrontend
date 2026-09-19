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

# The module-build keypair generator, in its own directory for one reason:
# gf_core compiles this tool's output, so this target has to exist before
# gf_core is declared. It is deliberately NOT linked against gf_core -- that
# would be the cycle -- and Qt::Core plus libsodium are the whole of what it
# needs.
add_executable(gf_module_keygen main.cpp)
target_link_libraries(gf_module_keygen PRIVATE Qt::Core sodium)

set_target_properties(gf_module_keygen PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${GPGFRONTEND_HOST_TOOL_DIR})

# Not installed: a key generator is a thing a build runs, not a thing a user
# gets.
