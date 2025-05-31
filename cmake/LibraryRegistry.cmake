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

function(register_library name out_var)
  if(NOT ARGN)
    message(FATAL_ERROR "register_library(${name} ...) requires at least one source file")
  endif()

  set(target_name "gf_${name}")

  add_library(${target_name} SHARED ${ARGN})

  set_target_properties(${target_name} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})

  if(SIGN_BUILT_BINARY)
    add_custom_command(TARGET ${target_name} POST_BUILD
      COMMAND ${OPENSSL_EXECUTABLE} dgst -sha256 -sign "${SIGN_PRIVATE_KEY}"
      -out "$<TARGET_FILE:${target_name}>.sig" "$<TARGET_FILE:${target_name}>"
      VERBATIM
    )
  endif()

  set_target_properties(${target_name} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN 1)

  # generate private macro
  string(TOUPPER ${name} upper_name)
  set(private_define "GF_${upper_name}_PRIVATE")
  target_compile_definitions(${target_name} PRIVATE ${private_define})

  # auto generate export header
  include(GenerateExportHeader)
  string(SUBSTRING ${name} 0 1 first_char)
  string(SUBSTRING ${name} 1 -1 rest_chars)
  string(TOUPPER ${first_char} first_char_upper)
  set(capitalized_name "${first_char_upper}${rest_chars}")
  set(export_file "${CMAKE_CURRENT_BINARY_DIR}/GF${capitalized_name}Export.h")
  generate_export_header(${target_name} EXPORT_FILE_NAME "${export_file}")

  target_precompile_headers(${target_name}
    PUBLIC ${CMAKE_SOURCE_DIR}/src/GpgFrontend.h
    PUBLIC ${export_file})

  target_include_directories(${target_name} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  )

  target_compile_features(${target_name} PRIVATE cxx_std_17)

  # skip install
  if(XCODE_BUILD)
    set_target_properties(${target_name} PROPERTIES
      XCODE_ATTRIBUTE_SKIP_INSTALL "Yes"
      XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${GPGFRONTEND_XCODE_CODE_SIGN_IDENTITY}")
  endif()

  set(current "${GPGFRONTEND_LIBRARY_TARGETS}")
  list(APPEND current "${target_name}")
  list(REMOVE_DUPLICATES current)
  set(GPGFRONTEND_LIBRARY_TARGETS "${current}" CACHE INTERNAL "All libraries" FORCE)

  set(${out_var} "${target_name}" PARENT_SCOPE)
endfunction()