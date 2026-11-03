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

# What an autotools ./configure needs to build for Android with the NDK the
# CMake toolchain uses: the GNU host triple, and the compiler and binutils as
# shell assignments to put in front of the configure call.
#
#   GF_ANDROID_TRIPLE         aarch64-linux-android (from the ABI)
#   GF_ANDROID_AUTOTOOLS_ENV  CC='...' AR='...' RANLIB='...' STRIP='...'

if(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
  set(GF_ANDROID_TRIPLE "aarch64-linux-android")
elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
  set(GF_ANDROID_TRIPLE "x86_64-linux-android")
else()
  message(FATAL_ERROR "Android ABI \"${CMAKE_ANDROID_ARCH_ABI}\" is not "
    "supported: use arm64-v8a, or x86_64 for the emulator")
endif()

if(DEFINED ANDROID_NDK)
  set(_gf_ndk "${ANDROID_NDK}")
else()
  set(_gf_ndk "${CMAKE_ANDROID_NDK}")
endif()

if(DEFINED ANDROID_PLATFORM_LEVEL)
  set(_gf_api "${ANDROID_PLATFORM_LEVEL}")
else()
  set(_gf_api "${CMAKE_SYSTEM_VERSION}")
endif()

set(_gf_bin "${_gf_ndk}/toolchains/llvm/prebuilt/linux-x86_64/bin")
if(NOT EXISTS "${_gf_bin}/${GF_ANDROID_TRIPLE}${_gf_api}-clang")
  message(FATAL_ERROR "No NDK compiler for ${GF_ANDROID_TRIPLE} API "
    "${_gf_api} in ${_gf_bin}")
endif()

set(GF_ANDROID_AUTOTOOLS_ENV
  "CC='${_gf_bin}/${GF_ANDROID_TRIPLE}${_gf_api}-clang' \
AR='${_gf_bin}/llvm-ar' RANLIB='${_gf_bin}/llvm-ranlib' \
STRIP='${_gf_bin}/llvm-strip'")

# The NDK's own toolchain file, for a CMake subproject; Qt's toolchain file
# only chain-loads it.
set(GF_ANDROID_NDK_TOOLCHAIN "${_gf_ndk}/build/cmake/android.toolchain.cmake")

message(STATUS "Android: ${GF_ANDROID_TRIPLE}, API ${_gf_api}, NDK ${_gf_ndk}")
