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

# tracking source files
aux_source_directory(. UI_SOURCE)
aux_source_directory(dialog/keypair_details UI_SOURCE)
aux_source_directory(widgets UI_SOURCE)
aux_source_directory(dialog/key_generate UI_SOURCE)
aux_source_directory(main_window UI_SOURCE)
aux_source_directory(dialog/help UI_SOURCE)
aux_source_directory(dialog/settings UI_SOURCE)
aux_source_directory(thread UI_SOURCE)
aux_source_directory(struct UI_SOURCE)
aux_source_directory(dialog/import_export UI_SOURCE)
aux_source_directory(dialog/controller UI_SOURCE)
aux_source_directory(dialog/profile UI_SOURCE)
aux_source_directory(dialog UI_SOURCE)
aux_source_directory(function UI_SOURCE)
aux_source_directory(model UI_SOURCE)
aux_source_directory(command UI_SOURCE)
aux_source_directory(lua UI_SOURCE)
aux_source_directory(html UI_SOURCE)

# register module
register_library(ui LIBRARY_TARGET ${UI_SOURCE})

# The relaunch belongs to this library rather than to the application target
# because this library calls it, and nothing linked against an executable can
# resolve a symbol the executable defines.
include(${CMAKE_SOURCE_DIR}/src/platform/PlatformSources.cmake)
target_sources(${LIBRARY_TARGET} PRIVATE ${GF_PLATFORM_UI_SOURCES})

# NSWorkspace, for the macOS relaunch
if(APPLE)
  find_library(APPKIT_FRAMEWORK AppKit REQUIRED)
  target_link_libraries(${LIBRARY_TARGET} ${APPKIT_FRAMEWORK})
endif()

# link qt
target_link_libraries(${LIBRARY_TARGET}
  Qt::Core Qt::Widgets Qt::Network Qt::PrintSupport)

# link gpgfrontend_core
target_link_libraries(${LIBRARY_TARGET} gf_core)

# The Lua interpreter behind module UI scripts. Private: nothing outside
# gf_ui sees a lua_State, and no module links Lua at all.
target_link_libraries(${LIBRARY_TARGET} gf_lua)

# set up pch
target_precompile_headers(${LIBRARY_TARGET} PUBLIC GpgFrontendUI.h)

# add ui generator include path
target_include_directories(${LIBRARY_TARGET} PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY_TARGET}_autogen/include)
