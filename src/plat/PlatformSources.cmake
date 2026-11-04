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
  ${GF_PLATFORM_DIR}/common/DetachedRelaunch.cpp
  ${GF_PLATFORM_DIR}/common/PlatformHtmlView.cpp
  ${GF_PLATFORM_DIR}/common/PlatformWebEditor.cpp)

# Compiles to nothing on platforms that ship a real backend below.
set(GF_PLATFORM_APP_SOURCES
  ${GF_PLATFORM_DIR}/common/StubSecretStore.cpp)

# The mobile application's platform interfaces: sharing, sensitive clipboard
# copies, keeping a window out of screenshots, the system font scale, what
# other applications share with it. The
# stubs compile to nothing where a backend below exists.
set(GF_PLATFORM_MOBILE_SOURCES
  ${GF_PLATFORM_DIR}/common/PlatformShare.cpp
  ${GF_PLATFORM_DIR}/common/PlatformClipboard.cpp
  ${GF_PLATFORM_DIR}/common/PlatformPrivacy.cpp
  ${GF_PLATFORM_DIR}/common/StubShare.cpp
  ${GF_PLATFORM_DIR}/common/StubClipboard.cpp
  ${GF_PLATFORM_DIR}/common/StubPrivacy.cpp
  ${GF_PLATFORM_DIR}/common/StubDisplay.cpp
  ${GF_PLATFORM_DIR}/common/StubInbox.cpp
  ${GF_PLATFORM_DIR}/common/StubFolder.cpp
  ${GF_PLATFORM_DIR}/common/StubRestart.cpp)

if(MINGW OR WIN32)
  list(APPEND GF_PLATFORM_CORE_SOURCES
    ${GF_PLATFORM_DIR}/windows/WinFs.cpp
    ${GF_PLATFORM_DIR}/windows/WinModule.cpp)
  # Compile to nothing unless the UI target found the WebView2 SDK and
  # defined GF_HAS_WEBVIEW2_SDK; the common stubs answer then.
  list(APPEND GF_PLATFORM_UI_SOURCES
    ${GF_PLATFORM_DIR}/windows/WinWebView2.cpp
    ${GF_PLATFORM_DIR}/windows/WinHtmlView.cpp
    ${GF_PLATFORM_DIR}/windows/WinWebEditor.cpp)
  list(APPEND GF_PLATFORM_APP_SOURCES
    ${GF_PLATFORM_DIR}/windows/WinSecretStore.cpp)
elseif(APPLE)
  list(APPEND GF_PLATFORM_CORE_SOURCES
    ${GF_PLATFORM_DIR}/unix/UnixFs.cpp
    ${GF_PLATFORM_DIR}/macos/MacFs.cpp
    ${GF_PLATFORM_DIR}/macos/MacAppSandbox.cpp)
  # Relaunching the bundle goes through NSWorkspace, HTML views and web
  # editors through WKWebView, so all three are Objective-C++.
  list(APPEND GF_PLATFORM_UI_SOURCES
    ${GF_PLATFORM_DIR}/macos/MacRelaunch.mm
    ${GF_PLATFORM_DIR}/macos/MacHtmlView.mm
    ${GF_PLATFORM_DIR}/macos/MacWebEditor.mm)
  list(APPEND GF_PLATFORM_APP_SOURCES
    ${GF_PLATFORM_DIR}/macos/MacSecretStore.cpp)
elseif(ANDROID)
  # The Linux filesystem helpers hold on Android's kernel; fscrypt, libsecret
  # and the desktop credential store do not apply. The secret store stays the
  # stub until a Keystore backend exists.
  list(APPEND GF_PLATFORM_CORE_SOURCES
    ${GF_PLATFORM_DIR}/unix/UnixFs.cpp
    ${GF_PLATFORM_DIR}/linux/LinuxFs.cpp)
  list(APPEND GF_PLATFORM_MOBILE_SOURCES
    ${GF_PLATFORM_DIR}/android/AndroidPlatform.cpp)
else()
  list(APPEND GF_PLATFORM_CORE_SOURCES
    ${GF_PLATFORM_DIR}/unix/UnixFs.cpp
    ${GF_PLATFORM_DIR}/linux/LinuxFs.cpp
    ${GF_PLATFORM_DIR}/linux/LinuxFscryptStorage.cpp)
  list(APPEND GF_PLATFORM_APP_SOURCES
    ${GF_PLATFORM_DIR}/linux/LinuxSecretStore.cpp
    ${GF_PLATFORM_DIR}/linux/LibSecretSearchPath.cpp)
endif()

# Only the Objective-C++ sources need the PCH skipped. The HTML view and the
# web editor hold WebKit objects in C++ containers, which only ARC keeps alive
# correctly.
if(APPLE)
  set_source_files_properties(${GF_PLATFORM_DIR}/macos/MacRelaunch.mm
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
  set_source_files_properties(${GF_PLATFORM_DIR}/macos/MacHtmlView.mm
    ${GF_PLATFORM_DIR}/macos/MacWebEditor.mm
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON COMPILE_OPTIONS "-fobjc-arc")
endif()
