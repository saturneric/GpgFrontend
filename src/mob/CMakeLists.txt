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

# gf_mob, the phone application.
#
# Everything but main() is in gf_mob_lib, the target of the one QML module
# (GpgFrontend.Mobile): the application and its tests link the same library and
# import the same plugin, so they register the same types.

include(${CMAKE_SOURCE_DIR}/src/platform/PlatformSources.cmake)

qt_add_library(gf_mob_lib STATIC
  startup/InteractionBridge.cpp
  startup/MobileProfileLoaderDelegate.cpp
  ${GF_PLATFORM_APP_SOURCES}
  ${GF_PLATFORM_MOBILE_SOURCES})

qt_add_qml_module(gf_mob_lib
  URI GpgFrontend.Mobile
  VERSION 1.0
  # Where QQmlEngine looks by default (the QTP0001 layout).
  RESOURCE_PREFIX /qt/qml)

target_include_directories(gf_mob_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# The generated type registrations include each header by its bare name.
target_include_directories(gf_mob_lib PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/startup
  ${CMAKE_CURRENT_SOURCE_DIR}/lifecycle
  ${CMAKE_CURRENT_SOURCE_DIR}/model
  ${CMAKE_CURRENT_SOURCE_DIR}/crypto)
target_link_libraries(gf_mob_lib PUBLIC
  gf_boot gf_core gf_res Qt::Quick Qt::QuickControls2 Qt::Svg)
