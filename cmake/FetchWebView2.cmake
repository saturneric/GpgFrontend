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

# The WebView2 SDK, for the built-in Windows HTML view renderer.
#
# Microsoft publishes it as a NuGet package: a plain COM header and
# WebView2Loader.dll, which finds the Evergreen runtime on the machine at run
# time. MSYS2 packages it for the ucrt64 and clang64 environments but not for
# mingw64, which this project builds in, so the pinned package is fetched at
# configure time and verified against its hash. The loader is a Microsoft
# redistributable and ships beside the binaries; the renderer resolves it
# with LoadLibrary, so a build without it still runs and simply has no
# built-in renderer.
#
# Sets:
#   WEBVIEW2_FOUND         whether the header and the loader are usable
#   WEBVIEW2_INCLUDE_DIR   where WebView2.h is
#   WEBVIEW2_LOADER_DLL    the x64 loader, to place beside the binaries
#
# For a build without network access, GPGFRONTEND_WEBVIEW2_SDK_DIR names an
# already-extracted package (the package's own layout: build/native/...).

set(_gf_webview2_version "1.0.3650.58")
set(_gf_webview2_sha256
  "911a472128c82ac8baa0c486c23342cc9dd6e7dc50d754e676726642ca065c60")

set(GPGFRONTEND_WEBVIEW2_SDK_DIR "" CACHE PATH
  "An extracted Microsoft.Web.WebView2 NuGet package, for builds without network access")

if(GPGFRONTEND_WEBVIEW2_SDK_DIR)
  set(_gf_webview2_root "${GPGFRONTEND_WEBVIEW2_SDK_DIR}")
else()
  set(_gf_webview2_root "${CMAKE_BINARY_DIR}/webview2-sdk/${_gf_webview2_version}")
  if(NOT EXISTS "${_gf_webview2_root}/build/native/include/WebView2.h")
    set(_gf_webview2_nupkg
      "${CMAKE_BINARY_DIR}/webview2-sdk/microsoft.web.webview2.${_gf_webview2_version}.nupkg")
    message(STATUS "Fetching the WebView2 SDK ${_gf_webview2_version}")
    file(DOWNLOAD
      "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/${_gf_webview2_version}"
      "${_gf_webview2_nupkg}"
      EXPECTED_HASH SHA256=${_gf_webview2_sha256}
      STATUS _gf_webview2_status)
    list(GET _gf_webview2_status 0 _gf_webview2_error)
    if(_gf_webview2_error EQUAL 0)
      file(ARCHIVE_EXTRACT
        INPUT "${_gf_webview2_nupkg}"
        DESTINATION "${_gf_webview2_root}"
        PATTERNS
          "build/native/include/WebView2.h"
          "build/native/x64/WebView2Loader.dll"
          "LICENSE.txt")
    else()
      list(GET _gf_webview2_status 1 _gf_webview2_error_text)
      file(REMOVE "${_gf_webview2_nupkg}")
      message(WARNING
        "The WebView2 SDK could not be fetched (${_gf_webview2_error_text}); "
        "set GPGFRONTEND_WEBVIEW2_SDK_DIR to an extracted package to build "
        "the HTML view renderer offline")
    endif()
  endif()
endif()

set(WEBVIEW2_INCLUDE_DIR "${_gf_webview2_root}/build/native/include")
set(WEBVIEW2_LOADER_DLL "${_gf_webview2_root}/build/native/x64/WebView2Loader.dll")
if(EXISTS "${WEBVIEW2_INCLUDE_DIR}/WebView2.h" AND EXISTS "${WEBVIEW2_LOADER_DLL}")
  set(WEBVIEW2_FOUND TRUE)
else()
  set(WEBVIEW2_FOUND FALSE)
endif()
