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

function(register_module name out_var)
  if(NOT ARGN)
    message(FATAL_ERROR "register_module(${name} ...) requires at least one source file")
  endif()

  set(target_name "gf_mod_${name}")

  add_library(${target_name} SHARED ${ARGN})

  set_target_properties(${target_name} PROPERTIES POSITION_INDEPENDENT_CODE ON)

  if(SIGN_BUILT_BINARY)
    add_custom_command(TARGET ${target_name} POST_BUILD
      COMMAND ${OPENSSL_EXECUTABLE} dgst -sha256 -sign "${SIGN_PRIVATE_KEY}"
      -out "$<TARGET_FILE:${target_name}>.sig" "$<TARGET_FILE:${target_name}>"
      VERBATIM
    )
  endif()

  target_compile_features(${target_name} PRIVATE cxx_std_17)

  target_link_libraries(${target_name} PRIVATE gf_sdk)

  # skip install
  if(XCODE_BUILD)
    set_target_properties(${target_name} PROPERTIES
      XCODE_ATTRIBUTE_SKIP_INSTALL "Yes"
      XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${GPGFRONTEND_XCODE_CODE_SIGN_IDENTITY}")
  endif()

  # install paths
  install(TARGETS ${target_name}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  set(current "${GPGFRONTEND_MODULE_TARGETS}")
  list(APPEND current "${target_name}")
  list(REMOVE_DUPLICATES current)
  set(GPGFRONTEND_MODULE_TARGETS "${current}" CACHE INTERNAL "All modules" FORCE)

  set(${out_var} "${target_name}" PARENT_SCOPE)
endfunction()