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

# The phone application's own tests: its controllers and models on the core
# (gf_mob_tests), and its pages (gf_mob_qml_tests).
find_package(Qt6 COMPONENTS Test QuickTest REQUIRED)

add_executable(gf_mob_tests
  main.cpp
  AppLifecycleTest.cpp
  ContentKindTest.cpp
  FileStoreTest.cpp
  FileWorkspaceTest.cpp
  InteropTest.cpp
  InteractionBridgeTest.cpp
  KeyActionsTest.cpp
  KeyModelsTest.cpp
  MobileProfileLoaderDelegateTest.cpp
  PlatformTest.cpp
  QmlRegistrationTest.cpp
  SettingsModelTest.cpp
  StartupTest.cpp
  TextCryptoTest.cpp)
target_include_directories(gf_mob_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/mob)
target_link_libraries(gf_mob_tests PRIVATE
  gf_mob_lib gf_mob_libplugin GTest::gtest Qt::Test)

add_executable(gf_mob_qml_tests QmlTestMain.cpp)
target_include_directories(gf_mob_qml_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src/mob)
target_link_libraries(gf_mob_qml_tests PRIVATE
  gf_mob_lib gf_mob_libplugin Qt::QuickTest)
target_compile_definitions(gf_mob_qml_tests PRIVATE
  QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/qml")
