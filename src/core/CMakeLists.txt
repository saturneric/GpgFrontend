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

# gather source files
aux_source_directory(./function/result_analyse CORE_SOURCE)
aux_source_directory(./function/basic CORE_SOURCE)
aux_source_directory(./function/gpg CORE_SOURCE)
aux_source_directory(./function/secure_memory CORE_SOURCE)
aux_source_directory(./function CORE_SOURCE)
aux_source_directory(./thread CORE_SOURCE)
aux_source_directory(./model CORE_SOURCE)
aux_source_directory(./common CORE_SOURCE)
aux_source_directory(./module CORE_SOURCE)
aux_source_directory(./utils/aes CORE_SOURCE)
aux_source_directory(./utils CORE_SOURCE)
aux_source_directory(. CORE_SOURCE)

# register core
register_library(core LIBRARY_TARGET ${CORE_SOURCE})

# link gnupg libraries
if(BUILD_AND_STATIC_LINK_GPGME)
  # if we need to static link gpgme
  target_link_libraries(${LIBRARY_TARGET} PUBLIC GnuPGDeps)
else()
  target_link_libraries(${LIBRARY_TARGET} PUBLIC gpgme assuan gpg-error)
endif()

# link openssl
target_link_libraries(${LIBRARY_TARGET} PUBLIC OpenSSL::SSL OpenSSL::Crypto)

if(MINGW)
  # for uuid ability in mingw
  target_link_libraries(${LIBRARY_TARGET} PUBLIC bcrypt)
endif()

# configure libarchive
if(APPLE)
  if(EXISTS "/usr/local/opt/libarchive/include")
    set(LibArchive_INCLUDE_DIR "/usr/local/opt/libarchive/include")
  else()
    set(LibArchive_INCLUDE_DIR "/opt/homebrew/opt/libarchive/include")
  endif()
endif()

find_package(LibArchive REQUIRED)
target_include_directories(${LIBRARY_TARGET} PRIVATE ${LibArchive_INCLUDE_DIR})

# link libarchive
target_link_libraries(${LIBRARY_TARGET} PRIVATE archive)

# link qt
target_link_libraries(${LIBRARY_TARGET} PUBLIC Qt::Core Qt::Gui)

# set up pch
target_precompile_headers(${LIBRARY_TARGET}
  PUBLIC ${CMAKE_SOURCE_DIR}/src/GpgFrontend.h
  PUBLIC GpgFrontendCore.h
  PUBLIC GpgConstants.h)

# link for different platforms
if(MINGW)
  target_link_libraries(${LIBRARY_TARGET} PUBLIC wsock32)
  target_link_libraries(${LIBRARY_TARGET} PRIVATE Wintrust Crypt32)
elseif(APPLE)
else()
  target_link_libraries(${LIBRARY_TARGET} PUBLIC pthread dl)
endif()
