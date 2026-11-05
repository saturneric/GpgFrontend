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
aux_source_directory(./function/basic CORE_SOURCE)
# The GnuPG engine; an rPGP-only build leaves it out, and GpgME with it. Three
# files beside it use no GpgME and stay: the process runner the module SDK also
# uses, the photo-ID reader built on it, and the GnuPG home path rules the key
# database settings show.
if(GF_ENGINE_GNUPG)
  aux_source_directory(./function/gpg CORE_SOURCE)
else()
  list(APPEND CORE_SOURCE
    ./function/gpg/GpgCommandExecutor.cpp
    ./function/gpg/GpgAttributeHelper.cpp
    ./function/gpg/GnuPGHome.cpp)
endif()
aux_source_directory(./function/openpgp CORE_SOURCE)
aux_source_directory(./function/secure_memory CORE_SOURCE)
aux_source_directory(./function/rpgp CORE_SOURCE)
aux_source_directory(./function CORE_SOURCE)
aux_source_directory(./profile CORE_SOURCE)
aux_source_directory(./typedef CORE_SOURCE)
aux_source_directory(./thread CORE_SOURCE)
aux_source_directory(./model CORE_SOURCE)
aux_source_directory(./module CORE_SOURCE)
aux_source_directory(./utils CORE_SOURCE)
aux_source_directory(. CORE_SOURCE)

# register core
register_library(core LIBRARY_TARGET ${CORE_SOURCE})

# the operating-system half of core, picked per platform
include(${CMAKE_SOURCE_DIR}/src/plat/PlatformSources.cmake)
target_sources(${LIBRARY_TARGET} PRIVATE ${GF_PLATFORM_CORE_SOURCES})

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

# The flavor's settings policy, where it has one: defaults that differ from the
# catalog's (SettingItem.h, FlavorSettingDefault).
set(_gf_settings_policy
  "${CMAKE_SOURCE_DIR}/resource/settings_policy/${GPGFRONTEND_BUILD_FLAVOR}.json")
if(EXISTS "${_gf_settings_policy}")
  set_source_files_properties("${_gf_settings_policy}"
    PROPERTIES QT_RESOURCE_ALIAS "settings_policy.json")
  qt_add_resources(${LIBRARY_TARGET} "gf_settings_policy"
    PREFIX "/gf" FILES "${_gf_settings_policy}")
endif()

# Whether the GnuPG engine is compiled in; tested with `#if GF_ENGINE_GNUPG`.
target_compile_definitions(${LIBRARY_TARGET} PUBLIC
  GF_ENGINE_GNUPG=${GF_ENGINE_GNUPG})

# link gnupg libraries
if(BUILD_AND_STATIC_LINK_GPGME)
  # if we need to static link gpgme
  target_link_libraries(${LIBRARY_TARGET} PUBLIC GnuPGDeps)
elseif(GF_ENGINE_GNUPG)
  target_link_libraries(${LIBRARY_TARGET} PUBLIC gpgme assuan gpg-error)
endif()

# The generated module-build trust root: the public key this Host verifies
# module descriptors with, and the build id they must carry. Generated rather
# than checked in, because it is different for every build tree.
# A cross build has none (src/CMakeLists.txt).
if(GF_HAS_MODULE_BUILD_KEY)
  set_source_files_properties("${GF_MODULE_TRUST_ROOT_SOURCE}"
    PROPERTIES GENERATED TRUE SKIP_PRECOMPILE_HEADERS ON)
  target_sources(${LIBRARY_TARGET} PRIVATE "${GF_MODULE_TRUST_ROOT_SOURCE}")
  add_dependencies(${LIBRARY_TARGET} gf_module_trust_root)
else()
  target_compile_definitions(${LIBRARY_TARGET} PRIVATE GF_NO_MODULE_BUILD_KEY)
endif()

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

# Android builds it from source instead (third_party/CMakeLists.txt).
if(NOT TARGET archive)
  find_package(LibArchive REQUIRED)
  target_include_directories(${LIBRARY_TARGET} PRIVATE ${LibArchive_INCLUDE_DIR})
endif()

# link libarchive
target_link_libraries(${LIBRARY_TARGET} PRIVATE archive)

# link qt
target_link_libraries(${LIBRARY_TARGET} PUBLIC Qt::Core Qt::Gui Qt::Sql)

if(MINGW)
  # link bcrypt for windows bcrypt is required for the bcrypt implementation of
  # AES, which is used by rpgp engine for crypto operations.
  target_link_libraries(gf_rust INTERFACE bcrypt)
elseif(ANDROID)
  # The engine's log goes to logcat.
  target_link_libraries(gf_rust INTERFACE log)
endif()

# link rust support library
target_link_libraries(${LIBRARY_TARGET} PRIVATE gf_rust)
target_compile_definitions(${LIBRARY_TARGET} PRIVATE HAS_RUST_SUPPORT)

if(MINGW)
  # bcypt uuid generation
  target_link_libraries(${LIBRARY_TARGET} PRIVATE "-lbcrypt")
  # ImageGetDigestStream, for the Authenticode digest in
  # plat/windows/WinModule.cpp
  target_link_libraries(${LIBRARY_TARGET} PRIVATE imagehlp)
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
  # Android's libc carries the threads itself; there is no libpthread.
  if(ANDROID)
    target_link_libraries(${LIBRARY_TARGET} PUBLIC dl)
  else()
    target_link_libraries(${LIBRARY_TARGET} PUBLIC pthread dl)
  endif()

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
