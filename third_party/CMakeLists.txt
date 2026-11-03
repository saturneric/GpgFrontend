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

# On Android nothing comes from the system: libsodium and libarchive are built
# here for the target, pinned by version and hash, and offered under the names
# gf_core already links ("sodium", "archive").
if(ANDROID)
  include(ExternalProject)
  include(${CMAKE_SOURCE_DIR}/cmake/AndroidCrossBuild.cmake)

  set(GF_ANDROID_DEPS "${CMAKE_BINARY_DIR}/deps")
  file(MAKE_DIRECTORY "${GF_ANDROID_DEPS}/include")

  # The name gf_core links, for the static library @p ep installs as @p lib
  # (plus what it needs from the NDK).
  function(_gf_android_static_dep name ep lib)
    add_library(${name} INTERFACE)
    add_dependencies(${name} ${ep})
    target_include_directories(${name} INTERFACE "${GF_ANDROID_DEPS}/include")
    target_link_libraries(${name} INTERFACE "${GF_ANDROID_DEPS}/lib/${lib}"
      ${ARGN})
  endfunction()

  ExternalProject_Add(
    EP_Sodium
    URL "https://download.libsodium.org/libsodium/releases/libsodium-1.0.20.tar.gz"
    URL_HASH SHA256=ebb65ef6ca439333c2bb41a0c1990587288da07f6c7fd07cb3a18cc18d30ce19
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    CONFIGURE_COMMAND
    bash -c "${GF_ANDROID_AUTOTOOLS_ENV} CFLAGS='-O2 -fPIC' \
      <SOURCE_DIR>/configure --host=${GF_ANDROID_TRIPLE} \
        --prefix=${GF_ANDROID_DEPS} --enable-static --disable-shared \
        --with-pic --disable-dependency-tracking"
    BUILD_COMMAND make
    INSTALL_COMMAND make install
    BUILD_BYPRODUCTS "${GF_ANDROID_DEPS}/lib/libsodium.a")

  _gf_android_static_dep(sodium EP_Sodium libsodium.a)

  # Only what gf_core reads and writes: tar and zip, deflate from the NDK's
  # zlib. No command-line tools, crypto backends or other compressors.
  ExternalProject_Add(
    EP_Archive
    URL "https://github.com/libarchive/libarchive/releases/download/v3.8.1/libarchive-3.8.1.tar.xz"
    URL_HASH SHA256=19f917d42d530f98815ac824d90c7eaf648e9d9a50e4f309c812457ffa5496b5
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    CMAKE_ARGS
      -DCMAKE_TOOLCHAIN_FILE=${GF_ANDROID_NDK_TOOLCHAIN}
      -DANDROID_ABI=${CMAKE_ANDROID_ARCH_ABI}
      -DANDROID_PLATFORM=${ANDROID_PLATFORM}
      -DCMAKE_BUILD_TYPE=Release
      -DCMAKE_INSTALL_PREFIX=${GF_ANDROID_DEPS}
      -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_INSTALL_INCLUDEDIR=include
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON
      -DBUILD_SHARED_LIBS=OFF
      -DENABLE_TEST=OFF -DENABLE_TAR=OFF -DENABLE_CPIO=OFF -DENABLE_CAT=OFF
      -DENABLE_UNZIP=OFF -DENABLE_INSTALL=ON
      -DENABLE_ZLIB=ON -DENABLE_BZip2=OFF -DENABLE_LZMA=OFF -DENABLE_ZSTD=OFF
      -DENABLE_LZ4=OFF -DENABLE_LZO=OFF -DENABLE_LIBB2=OFF
      -DENABLE_OPENSSL=OFF -DENABLE_MBEDTLS=OFF -DENABLE_NETTLE=OFF
      -DENABLE_CNG=OFF -DENABLE_LIBXML2=OFF -DENABLE_EXPAT=OFF
      -DENABLE_ICONV=OFF -DENABLE_ACL=OFF -DENABLE_XATTR=OFF
      -DENABLE_PCREPOSIX=OFF -DENABLE_PCRE2POSIX=OFF
    BUILD_BYPRODUCTS "${GF_ANDROID_DEPS}/lib/libarchive.a")

  _gf_android_static_dep(archive EP_Archive libarchive.a z)
endif()

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

# The engine ships with the app and reports its version, so a release bumps
# one number. Cargo tracks the variable, so changing it rebuilds the crate.
corrosion_set_env_vars(gf_rust GFR_ENGINE_VERSION=${PROJECT_VERSION})

# On macOS Corrosion hard-codes the Command Line Tools SDK's usr/lib as a link
# directory of every Rust library, so it reaches whatever links gf_rust and
# -larchive, -lSystem and friends resolve there instead of in the SDK the
# compiler was given. Once the CLT SDK is newer than the active Xcode, its
# .tbd stubs name architectures that linker cannot parse ("unknown
# architecture"). The -isysroot SDK already provides every one of them.
if(APPLE)
  foreach(_gf_rust_lib gf_rust-static gf_rust-shared)
    if(TARGET ${_gf_rust_lib})
      set_property(TARGET ${_gf_rust_lib} PROPERTY INTERFACE_LINK_DIRECTORIES "")
    endif()
  endforeach()
endif()
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
