#
# Copyright (C) 2021 Saturneric
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
# Saturneric<eric@bktus.com> starting on May 12, 2021.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# easyloggingpp
set(build_static_lib 1 CACHE BOOL "" FORCE)
add_subdirectory(easyloggingpp)
target_include_directories(easyloggingpp PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/easyloggingpp/src)

# json
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(json)

# qt-aes
include(GenerateExportHeader)
add_subdirectory(qt-aes)

# vmime
set(VMIME_HAVE_MESSAGING_PROTO_SENDMAIL OFF CACHE BOOL "" FORCE)
set(VMIME_HAVE_SASL_SUPPORT OFF CACHE BOOL "" FORCE)
set(VMIME_BUILD_STATIC_LIBRARY ON CACHE BOOL "" FORCE)
set(VMIME_BUILD_SHARED_LIBRARY OFF CACHE BOOL "" FORCE)
set(VMIME_CHARSETCONV_LIB "iconv" CACHE STRING "" FORCE)
set(VMIME_TLS_SUPPORT_LIB "openssl" CACHE STRING "" FORCE)

set(VMIME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vmime)
set(VMIME_BIN ${CMAKE_CURRENT_BINARY_DIR}/vmime)

set(VMIME_INCLUDES ${VMIME_DIR}/src ${CMAKE_BINARY_DIR}/src/vmime/ ${CMAKE_BINARY_DIR}/src/)

set(VMIME_BIN_STATIC_LIB ${VMIME_BIN}/build/lib/libvmime.a)

add_subdirectory(vmime)

add_library(gpgfrontend_vmime STATIC IMPORTED GLOBAL)
add_dependencies(gpgfrontend_vmime vmime-static vmime-static-config)
set_target_properties(gpgfrontend_vmime PROPERTIES IMPORTED_LOCATION ${VMIME_BIN_STATIC_LIB})
set_target_properties(gpgfrontend_vmime PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${VMIME_INCLUDES}")

# gnupg

include(ExternalProject)

# libgpg-error


if (NOT MINGW)
    set(GPG_ERROR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libgpg-error)
    set(GPG_ERROR_BIN ${CMAKE_CURRENT_BINARY_DIR}/libgpg-error)
    set(GPG_ERROR_STATIC_LIB ${GPG_ERROR_BIN}/lib/libgpg-error.a)
    set(GPG_ERROR_INCLUDES ${GPG_ERROR_BIN}/include)
    set(GPG_ERROR_LIBS ${GPG_ERROR_BIN}/lib)
    file(MAKE_DIRECTORY ${GPG_ERROR_INCLUDES})
else ()
    find_library(GPG_ERROR_STATIC_LIB libgpg-error.a)
endif ()

if (NOT MINGW)

    ExternalProject_Add(libgpg-error
            PREFIX ${GPG_ERROR_BIN}
            SOURCE_DIR ${GPG_ERROR_DIR}
            CONFIGURE_COMMAND ${GPG_ERROR_DIR}/configure --enable-maintainer-mode --prefix=${GPG_ERROR_BIN} --enable-static=yes
            BUILD_COMMAND make
            INSTALL_COMMAND make install
            BUILD_BYPRODUCTS ${GPG_ERROR_STATIC_LIB})

    ExternalProject_Add_Step(libgpg-error autogen
            WORKING_DIRECTORY ${GPG_ERROR_DIR}
            COMMAND ${GPG_ERROR_DIR}/autogen.sh
            DEPENDERS configure)

endif ()

add_library(gpg-error STATIC IMPORTED GLOBAL)
if (NOT MINGW)
    add_dependencies(gpg-error libgpg-error)
    set_target_properties(gpg-error PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${GPG_ERROR_INCLUDES})
endif ()
set_target_properties(gpg-error PROPERTIES IMPORTED_LOCATION ${GPG_ERROR_STATIC_LIB})

# libassuan

if (NOT MINGW)
    set(ASSUAN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libassuan)
    set(ASSUAN_BIN ${CMAKE_CURRENT_BINARY_DIR}/libassuan)
    set(ASSUAN_STATIC_LIB ${ASSUAN_BIN}/lib/libassuan.a)
    set(ASSUAN_INCLUDES ${ASSUAN_BIN}/include)
    set(ASSUAN_LIBS ${ASSUAN_BIN}/lib)
    file(MAKE_DIRECTORY ${ASSUAN_INCLUDES})
else ()
    find_library(ASSUAN_STATIC_LIB libassuan.a)
endif ()

if (NOT MINGW)

    ExternalProject_Add(libassuan
            PREFIX ${ASSUAN_BIN}
            SOURCE_DIR ${ASSUAN_DIR}
            CONFIGURE_COMMAND ${ASSUAN_DIR}/configure --enable-maintainer-mode --prefix=${ASSUAN_BIN} --enable-static=yes
            --with-libgpg-error-prefix=${GPG_ERROR_BIN}
            BUILD_COMMAND make
            INSTALL_COMMAND make install
            BUILD_BYPRODUCTS ${ASSUAN_STATIC_LIB})

    ExternalProject_Add_Step(libassuan autogen
            WORKING_DIRECTORY ${ASSUAN_DIR}
            COMMAND ${ASSUAN_DIR}/autogen.sh
            DEPENDERS configure)

    ExternalProject_Add_StepDependencies(libassuan autogen libgpg-error)

endif ()

add_library(assuan STATIC IMPORTED GLOBAL)
if (NOT MINGW)
    add_dependencies(assuan libassuan gpg-error)
    set_target_properties(assuan PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${ASSUAN_INCLUDES})
endif ()
set_target_properties(assuan PROPERTIES IMPORTED_LOCATION ${ASSUAN_STATIC_LIB})

# gpgme

if (NOT MINGW)
    set(GPGME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gpgme)
    set(GPGME_BIN ${CMAKE_CURRENT_BINARY_DIR}/gpgme)
    set(GPGME_STATIC_LIB ${GPGME_BIN}/lib/libgpgme.a)
    set(GPGME_INCLUDES ${GPGME_BIN}/include)
    file(MAKE_DIRECTORY ${GPGME_INCLUDES})
else ()
    find_library(GPGME_STATIC_LIB libgpgme.a)
endif ()

if (NOT MINGW)

    ExternalProject_Add(libgpgme
            PREFIX ${GPGME_BIN}
            SOURCE_DIR ${GPGME_DIR}
            CONFIGURE_COMMAND ${GPGME_DIR}/configure --enable-maintainer-mode --prefix=${GPGME_BIN} --enable-shared=no --enable-static=yes --disable-gpg-test --enable-languages=cpp
            --with-libgpg-error-prefix=${GPG_ERROR_BIN}
            --with-libassuan-prefix=${ASSUAN_BIN}
            "LIB=-lassuan -lgpg-error"
            "LDFLAGS=-L${ASSUAN_LIBS} -L${GPG_ERROR_LIBS} "
            BUILD_COMMAND make
            INSTALL_COMMAND make install
            BUILD_BYPRODUCTS ${GPGME_STATIC_LIB})

    ExternalProject_Add_Step(libgpgme autogen
            WORKING_DIRECTORY ${GPGME_DIR}
            COMMAND ${GPGME_DIR}/autogen.sh
            DEPENDERS configure)

    ExternalProject_Add_StepDependencies(libgpgme autogen libassuan libgpg-error)

endif ()

add_library(gpgme STATIC IMPORTED GLOBAL)
if (NOT MINGW)
    add_dependencies(gpgme libgpgme assuan gpg-error)
    set_target_properties(gpgme PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${GPGME_INCLUDES})
endif ()
set_target_properties(gpgme PROPERTIES IMPORTED_LOCATION ${GPGME_STATIC_LIB})

if (SMTP_SUPPORT)
    message(STATUS "Build SMTP Support")
    aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/smtp-mime/src SMTP_MIME_SOURCE)
    add_library(smtp-mime STATIC ${SMTP_MIME_SOURCE})
    target_link_libraries(smtp-mime
            Qt5::Network Qt5::Core)
    target_include_directories(smtp-mime PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/smtp-mime/src)
    if (XCODE_BUILD)
        set_target_properties(smtp-mime
                PROPERTIES
                ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
                LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
                LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
    endif ()
endif ()

# encoding detect library
aux_source_directory(./encoding-detect ENCODING_DETECT_SOURCE_CODE)
add_library(encoding_detect STATIC ${ENCODING_DETECT_SOURCE_CODE})

# libarchive
if (NOT MINGW)
    remove_definitions(-DDEBUG)
    add_subdirectory(libarchive)
endif ()