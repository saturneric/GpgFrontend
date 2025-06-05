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

set(DEPS_INSTALL_ROOT "${CMAKE_BINARY_DIR}/deps")
include(ExternalProject)

if(BUILD_GPGME)
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
          --enable-fast-install
      "

    BUILD_COMMAND make
    INSTALL_COMMAND make install
    BUILD_BYPRODUCTS "${DEPS_INSTALL_ROOT}/lib/libgpg-error.a"
  )

  ExternalProject_Add(
    EP_Assuan

    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libassuan"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/libassuan"

    CONFIGURE_COMMAND
    bash -c
    "cd ${CMAKE_CURRENT_SOURCE_DIR}/libassuan && \ 
        ./autogen.sh"
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
        --enable-fast-install
    "

    BUILD_COMMAND make
    INSTALL_COMMAND make install
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
    "cd ${CMAKE_CURRENT_SOURCE_DIR}/gpgme && \
        ./autogen.sh "
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
        ${EXTRA_W32_GLIB}
    "

    BUILD_COMMAND make
    INSTALL_COMMAND make install
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

  add_library(GpgDeps INTERFACE)
  add_dependencies(GpgDeps EP_GpgError EP_Assuan EP_GpgME)
  target_include_directories(GpgDeps INTERFACE ${DEPS_INSTALL_ROOT}/include)
  target_link_libraries(GpgDeps INTERFACE GpgME Assuan GpgError)
endif()