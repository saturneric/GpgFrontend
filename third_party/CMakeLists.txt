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

if(BUILD_AND_STATIC_LINK_GPGME AND NOT MINGW AND NOT XCODE_BUILD)
  include(ExternalProject)

  set(DEPS_INSTALL_ROOT "${CMAKE_BINARY_DIR}/deps")

  ExternalProject_Add(
    EP_GpgError
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libgpg-error"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/libgpg-error"

    CONFIGURE_COMMAND
    bash -c
    "cd ${CMAKE_CURRENT_SOURCE_DIR}/libgpg-error && \
         ./autogen.sh"
    &&
    bash -c
    "cd ${CMAKE_CURRENT_BINARY_DIR}/libgpg-error && \
        CFLAGS='-fPIC' ${CMAKE_CURRENT_SOURCE_DIR}/libgpg-error/configure \
          --prefix=${DEPS_INSTALL_ROOT} \
          --enable-maintainer-mode \
          --enable-static \
          --disable-shared \
          --disable-rpath \
          --enable-silent-rules \
          --disable-dependency-tracking \
          --disable-tests \
          --enable-fast-install"

    BUILD_COMMAND ${CMAKE_COMMAND} -E chdir <BINARY_DIR> make
    INSTALL_COMMAND ${CMAKE_COMMAND} -E chdir <BINARY_DIR> make install
    BUILD_BYPRODUCTS "${DEPS_INSTALL_ROOT}/lib/libgpg-error.a"
  )

  ExternalProject_Add(
    EP_Assuan

    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libassuan"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/libassuan"

    CONFIGURE_COMMAND
    bash -c
    "cd ${CMAKE_CURRENT_SOURCE_DIR}/libassuan && ./autogen.sh"
    &&
    bash -c
    "cd ${CMAKE_CURRENT_BINARY_DIR}/libassuan && \
        CFLAGS='-fPIC' ${CMAKE_CURRENT_SOURCE_DIR}/libassuan/configure \
        --prefix=${DEPS_INSTALL_ROOT} \
        --with-libgpg-error-prefix=${DEPS_INSTALL_ROOT} \
        --enable-maintainer-mode \
        --enable-static \
        --disable-shared \
        --enable-silent-rules \
        --disable-dependency-tracking \
        --disable-doc \
        --enable-fast-install"

    DEPENDS EP_GpgError
    BUILD_BYPRODUCTS "${DEPS_INSTALL_ROOT}/lib/libassuan.a"
  )

  if(MINGW)
    set(EXTRA_W32_GLIB "--enable-w32-glib")
  else()
    set(EXTRA_W32_GLIB "")
  endif()

  ExternalProject_Add(
    EP_GpgME

    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/gpgme"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/gpgme"

    CONFIGURE_COMMAND
    bash -c
    "cd ${CMAKE_CURRENT_SOURCE_DIR}/gpgme && ./autogen.sh "
    &&
    bash -c
    "cd ${CMAKE_CURRENT_BINARY_DIR}/gpgme && \
        CFLAGS='-fPIC' ${CMAKE_CURRENT_SOURCE_DIR}/gpgme/configure \
        --prefix=${DEPS_INSTALL_ROOT} \
        --with-libgpg-error-prefix=${DEPS_INSTALL_ROOT} \
        --with-libassuan-prefix=${DEPS_INSTALL_ROOT} \
        --enable-maintainer-mode \
        --enable-static \
        --disable-shared \
        --enable-silent-rules \
        --disable-dependency-tracking \
        --enable-languages=cl \
        --disable-gpgconf-test \
        --disable-gpg-test \
        --disable-gpgsm-test \
        --disable-g13-test \
        ${EXTRA_W32_GLIB}"

    BUILD_COMMAND ${CMAKE_COMMAND} -E chdir <BINARY_DIR> make
    INSTALL_COMMAND ${CMAKE_COMMAND} -E chdir <BINARY_DIR> make install

    DEPENDS EP_GpgError EP_Assuan
    BUILD_BYPRODUCTS "${DEPS_INSTALL_ROOT}/lib/libgpgme.a"
  )

  # GpgError
  add_library(GpgError STATIC IMPORTED)
  set_target_properties(GpgError PROPERTIES
    IMPORTED_LOCATION "${DEPS_INSTALL_ROOT}/lib/libgpg-error.a"
  )

  # Assuan
  add_library(Assuan STATIC IMPORTED)
  set_target_properties(Assuan PROPERTIES
    IMPORTED_LOCATION "${DEPS_INSTALL_ROOT}/lib/libassuan.a"
    INTERFACE_LINK_LIBRARIES "GpgError"
  )

  # GpgME
  add_library(GpgME STATIC IMPORTED)
  set_target_properties(GpgME PROPERTIES
    IMPORTED_LOCATION "${DEPS_INSTALL_ROOT}/lib/libgpgme.a"
    INTERFACE_LINK_LIBRARIES "Assuan;GpgError"
  )

  add_library(GnuPGDeps INTERFACE)
  add_dependencies(GnuPGDeps EP_GpgError EP_Assuan EP_GpgME)
  target_include_directories(GnuPGDeps INTERFACE ${DEPS_INSTALL_ROOT}/include)
  target_link_libraries(GnuPGDeps INTERFACE GpgME Assuan GpgError)

  # mark this status as really built
  set(GNUPG_DEPS_BUILT TRUE CACHE BOOL "GnuPGDeps built status" FORCE)
endif()

if(BUILD_AND_STATIC_LINK_GPGME AND MINGW)
  find_library(
    GPG_ERROR_STATIC_LIB
    NAMES libgpg-error.a
    REQUIRED
  )

  find_library(
    ASSUAN_STATIC_LIB
    NAMES libassuan.a
    REQUIRED
  )

  find_library(
    GPGME_STATIC_LIB
    NAMES libgpgme.a
    REQUIRED
  )

  add_library(GnuPGDeps INTERFACE)
  target_link_libraries(GnuPGDeps INTERFACE ${GPGME_STATIC_LIB} ${ASSUAN_STATIC_LIB} ${GPG_ERROR_STATIC_LIB})
  target_link_libraries(GnuPGDeps INTERFACE ws2_32)
endif()

if(BUILD_AND_STATIC_LINK_GPGME AND XCODE_BUILD)
  find_library(
    GPG_ERROR_STATIC_LIB
    NAMES libgpg-error.a
    REQUIRED
  )

  find_library(
    ASSUAN_STATIC_LIB
    NAMES libassuan.a
    REQUIRED
  )

  find_library(
    GPGME_STATIC_LIB
    NAMES libgpgme.a
    REQUIRED
  )

  add_library(GnuPGDeps INTERFACE)
  target_link_libraries(GnuPGDeps INTERFACE intl)
  target_link_libraries(GnuPGDeps INTERFACE ${GPGME_STATIC_LIB} ${ASSUAN_STATIC_LIB} ${GPG_ERROR_STATIC_LIB})
endif()

# Introduce Rust support
add_subdirectory(corrosion)

# Import targets defined in a package or workspace manifest `Cargo.toml` file
corrosion_import_crate(MANIFEST_PATH ${CMAKE_SOURCE_DIR}/rust/Cargo.toml)
# ---------------------------------------------------------------------------
# gf_lua -- the Lua interpreter behind module UI scripts. Host-only.
# ---------------------------------------------------------------------------
#
# Compiled as C, with Lua's own setjmp/longjmp error handling. The Host never
# lets a Lua error jump across a C++ frame that owns an object: every entry
# into Lua is a lua_pcall, and every binding raises only from a frame with
# nothing to destroy (src/ui/lua/LuaBinding.h). That invariant is checked by
# tests, and holds the same way on every compiler -- which building Lua as C++
# and relying on exceptions to unwind through it would not.
#
# Only the libraries the sandbox opens are built. There is no io, os,
# package, debug or coroutine library in the binary at all, so no mistake in
# the sandbox's setup could open one.
set(GF_LUA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lua")
set(GF_LUA_SOURCES
  lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c lmem.c
  lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c ltm.c lundump.c
  lvm.c lzio.c
  lauxlib.c lbaselib.c lmathlib.c lstrlib.c ltablib.c lutf8lib.c)
list(TRANSFORM GF_LUA_SOURCES PREPEND "${GF_LUA_DIR}/")

add_library(gf_lua STATIC ${GF_LUA_SOURCES})
set_source_files_properties(${GF_LUA_SOURCES} PROPERTIES LANGUAGE C)
set_target_properties(gf_lua PROPERTIES
  C_STANDARD 99
  POSITION_INDEPENDENT_CODE ON   # linked into libgf_ui
  C_VISIBILITY_PRESET hidden     # nothing of Lua is exported from the Host
  AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
target_include_directories(gf_lua PUBLIC "${GF_LUA_DIR}")
if(UNIX)
  target_link_libraries(gf_lua PRIVATE m)
endif()
