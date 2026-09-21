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
aux_source_directory(./function/openpgp CORE_SOURCE)
aux_source_directory(./function/secure_memory CORE_SOURCE)
aux_source_directory(./function/rpgp CORE_SOURCE)
aux_source_directory(./function CORE_SOURCE)
aux_source_directory(./profile CORE_SOURCE)
aux_source_directory(./typedef CORE_SOURCE)
aux_source_directory(./thread CORE_SOURCE)
aux_source_directory(./model CORE_SOURCE)
aux_source_directory(./common CORE_SOURCE)
aux_source_directory(./module CORE_SOURCE)
aux_source_directory(./utils/aes CORE_SOURCE)
aux_source_directory(./utils CORE_SOURCE)
aux_source_directory(. CORE_SOURCE)

# register core
register_library(core LIBRARY_TARGET ${CORE_SOURCE})

target_compile_definitions(${LIBRARY_TARGET} PUBLIC
  $<$<CONFIG:Debug>:GF_BUILD_DEBUG=1>
  $<$<CONFIG:RelWithDebInfo>:GF_BUILD_RELWITHDEBINFO=1>
  $<$<CONFIG:Release>:GF_BUILD_RELEASE=1>
  $<$<CONFIG:MinSizeRel>:GF_BUILD_MINSIZEREL=1>
)

if(APPLE_SANDBOX)
  message(STATUS "Enabling macOS sandbox for Xcode build")
  target_link_libraries(${LIBRARY_TARGET} PRIVATE "-framework Security")
endif()

# link gnupg libraries
if(BUILD_AND_STATIC_LINK_GPGME)
  # if we need to static link gpgme
  target_link_libraries(${LIBRARY_TARGET} PUBLIC GnuPGDeps)
else()
  target_link_libraries(${LIBRARY_TARGET} PUBLIC gpgme assuan gpg-error)
endif()

# The generated module-build trust root: the public key this Host verifies
# module descriptors with, and the build id they must carry. Generated rather
# than checked in, because it is different for every build tree.
set_source_files_properties("${GF_MODULE_TRUST_ROOT_SOURCE}"
  PROPERTIES GENERATED TRUE SKIP_PRECOMPILE_HEADERS ON)
target_sources(${LIBRARY_TARGET} PRIVATE "${GF_MODULE_TRUST_ROOT_SOURCE}")
add_dependencies(${LIBRARY_TARGET} gf_module_trust_root)

# The integrated module native-binding policy this build was configured with.
# Generated for the same reason the trust root is: one definition, compiled in,
# so gf_core and gf_module_packager cannot hold different answers.
set_source_files_properties("${GF_MODULE_HOST_POLICY_SOURCE}"
  PROPERTIES GENERATED TRUE SKIP_PRECOMPILE_HEADERS ON)
target_sources(${LIBRARY_TARGET} PRIVATE "${GF_MODULE_HOST_POLICY_SOURCE}")

# link libsodium
target_link_libraries(${LIBRARY_TARGET} PUBLIC sodium)

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
target_link_libraries(${LIBRARY_TARGET} PUBLIC Qt::Core Qt::Gui Qt::Sql)

if(MINGW)
  # link bcrypt for windows bcrypt is required for the bcrypt implementation of
  # AES, which is used by rpgp engine for crypto operations.
  target_link_libraries(gf_rust INTERFACE bcrypt)
endif()

# link rust support library
target_link_libraries(${LIBRARY_TARGET} PRIVATE gf_rust)
target_compile_definitions(${LIBRARY_TARGET} PRIVATE HAS_RUST_SUPPORT)

if(MINGW)
  # bcypt uuid generation
  target_link_libraries(${LIBRARY_TARGET} PRIVATE "-lbcrypt")
endif()

# set up pch
target_precompile_headers(${LIBRARY_TARGET}
  PUBLIC ${CMAKE_SOURCE_DIR}/src/GpgFrontend.h
  PUBLIC GpgFrontendCore.h
  PUBLIC GFConstants.h)

# link for different platforms
if(MINGW)
  target_link_libraries(${LIBRARY_TARGET} PUBLIC wsock32)
elseif(APPLE)
else()
  target_link_libraries(${LIBRARY_TARGET} PUBLIC pthread dl)

  # Refuse to produce a gf_core with unresolved symbols.
  #
  # This is not tidiness. A MinGW DLL and a Mach-O dylib BOTH require every
  # symbol to resolve at link time, while ELF shared libraries do not -- so an
  # undefined symbol here is a Linux build that succeeds and a Windows and
  # macOS build that fails, which is the worst possible place to find out. It
  # cost two broken platforms once already: gf_core acquired references to
  # four gf_sdk entry points, Linux linked them happily and resolved them
  # lazily once a module dragged libgf_sdk.so in, and both other platforms
  # stopped linking entirely.
  #
  # With this, that failure happens at the developer's desk, in the form the
  # other two platforms would have reported it.
  target_link_options(${LIBRARY_TARGET} PRIVATE "LINKER:--no-undefined")
endif()
