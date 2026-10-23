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

# The one place that decides which platform sources go into which binary.
#
# The tree is split by operating system, but gf_core, gf_ui and the
# application are separate binaries and a symbol has to be compiled into the
# one that calls it: a library cannot resolve a symbol only the executable
# defines. So every list below is per target, and each target's CMakeLists
# include()s this file and adds its own list.
#
# common/ holds the portable interfaces and the portable answers; every file
# in it is built everywhere and guards itself. unix/ is the POSIX code Linux,
# FreeBSD and macOS share.

set(GF_PLATFORM_DIR ${CMAKE_CURRENT_LIST_DIR})

set(GF_PLATFORM_CORE_SOURCES
  ${GF_PLATFORM_DIR}/common/FscryptStorage.cpp
  ${GF_PLATFORM_DIR}/common/PlatformEnv.cpp
  ${GF_PLATFORM_DIR}/common/PlatformFs.cpp
  ${GF_PLATFORM_DIR}/common/PlatformModule.cpp)

set(GF_PLATFORM_UI_SOURCES
  ${GF_PLATFORM_DIR}/common/DetachedRelaunch.cpp)

# Compiles to nothing on platforms that ship a real backend below.
set(GF_PLATFORM_APP_SOURCES
  ${GF_PLATFORM_DIR}/common/StubSecretStore.cpp)

if(MINGW OR WIN32)
  list(APPEND GF_PLATFORM_CORE_SOURCES
    ${GF_PLATFORM_DIR}/windows/WinFs.cpp
    ${GF_PLATFORM_DIR}/windows/WinModule.cpp)
  list(APPEND GF_PLATFORM_APP_SOURCES
    ${GF_PLATFORM_DIR}/windows/WinSecretStore.cpp)
elseif(APPLE)
  list(APPEND GF_PLATFORM_CORE_SOURCES
    ${GF_PLATFORM_DIR}/unix/UnixFs.cpp
    ${GF_PLATFORM_DIR}/macos/MacFs.cpp
    ${GF_PLATFORM_DIR}/macos/MacAppSandbox.cpp)
  # Relaunching the bundle goes through NSWorkspace, so it is Objective-C++.
  list(APPEND GF_PLATFORM_UI_SOURCES
    ${GF_PLATFORM_DIR}/macos/MacRelaunch.mm)
  list(APPEND GF_PLATFORM_APP_SOURCES
    ${GF_PLATFORM_DIR}/macos/MacSecretStore.cpp)
else()
  list(APPEND GF_PLATFORM_CORE_SOURCES
    ${GF_PLATFORM_DIR}/unix/UnixFs.cpp
    ${GF_PLATFORM_DIR}/linux/LinuxFs.cpp
    ${GF_PLATFORM_DIR}/linux/LinuxFscryptStorage.cpp)
  list(APPEND GF_PLATFORM_APP_SOURCES
    ${GF_PLATFORM_DIR}/linux/LinuxSecretStore.cpp
    ${GF_PLATFORM_DIR}/linux/LibSecretSearchPath.cpp)
endif()

# Only the Objective-C++ source needs the PCH skipped.
if(APPLE)
  set_source_files_properties(${GF_PLATFORM_DIR}/macos/MacRelaunch.mm
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
endif()
