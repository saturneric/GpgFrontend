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

# module_add_translations(<target>
#   TS_FILES <file>...
#   [SOURCES <file>...]
#   [INCLUDE_DIRECTORIES <dir>...])
#
# Adds Qt translation support to a module target, handling both the standard
# path and the Xcode path transparently.
function(module_add_translations target)
  cmake_parse_arguments(MAT "" "" "TS_FILES;SOURCES;INCLUDE_DIRECTORIES" ${ARGN})

  if(NOT MAT_TS_FILES)
    message(FATAL_ERROR "module_add_translations: TS_FILES is required")
  endif()

  if(NOT XCODE_BUILD)
    # Pin the lupdate scope to this module's own target. Without SOURCE_TARGETS,
    # qt_add_translations walks the whole modules/ tree and funnels every other
    # module's (and vmime's) strings into this module's .ts files.
    qt_add_translations(${target}
      SOURCE_TARGETS ${target}
      RESOURCE_PREFIX "/i18n"
      TS_FILES ${MAT_TS_FILES}
      SOURCES ${MAT_SOURCES}
      INCLUDE_DIRECTORIES ${MAT_INCLUDE_DIRECTORIES})
  else()
    set(i18n_target "${target}_i18n")
    add_custom_target(${i18n_target} ALL)
    qt_add_lrelease(${i18n_target}
      TS_FILES ${MAT_TS_FILES}
      QM_FILES_OUTPUT_VARIABLE TRANSLATIONS_QM)
    qt_add_resources(${target} ${i18n_target}
      PREFIX "/i18n"
      BASE ${CMAKE_CURRENT_BINARY_DIR}
      FILES ${TRANSLATIONS_QM})
  endif()
endfunction()

function(register_module name out_var)
  if(NOT ARGN)
    message(FATAL_ERROR "register_module(${name} ...) requires at least one source file")
  endif()

  set(target_name "gf_mod_${name}")

  add_library(${target_name} SHARED ${ARGN})

  set_target_properties(${target_name} PROPERTIES POSITION_INDEPENDENT_CODE ON)

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
# gf_add_module_package(
#   TARGET           <name as given to register_module>
#   MODULE_ID        <reverse-dns identifier>
#   [VERSION         <x.y.z>]              defaults to PROJECT_VERSION
#   [MIN_HOST_VERSION <x.y.z>]             defaults to PROJECT_VERSION
#   [CAPABILITIES    <name>...]
#   [META            KEY=VALUE...]
#   [RESOURCES       <archive path>=<source file>...])
#
# Produces `<name>.gfmodule` as an ordinary build artifact, signed with a key
# generated for that build and destroyed with it. There is no CI-specific
# environment and no key to configure: a developer runs the normal build and
# gets a signed package.
#
# What that signature proves is narrow, and is spelled out in full on
# VerifyModulePackage(): the public key travels inside the package, so it
# establishes that the package agrees with itself, not who built it.
function(gf_add_module_package)
  cmake_parse_arguments(GAMP
    ""
    "TARGET;MODULE_ID;VERSION;MIN_HOST_VERSION"
    "CAPABILITIES;META;RESOURCES"
    ${ARGN})

  if(NOT GAMP_TARGET)
    message(FATAL_ERROR "gf_add_module_package: TARGET is required")
  endif()
  if(NOT GAMP_MODULE_ID)
    message(FATAL_ERROR "gf_add_module_package: MODULE_ID is required")
  endif()

  set(module_target "gf_mod_${GAMP_TARGET}")
  if(NOT TARGET ${module_target})
    message(FATAL_ERROR
      "gf_add_module_package: no such module target: ${module_target}")
  endif()

  if(NOT GAMP_VERSION)
    set(GAMP_VERSION "${PROJECT_VERSION}")
  endif()
  if(NOT GAMP_MIN_HOST_VERSION)
    set(GAMP_MIN_HOST_VERSION "${PROJECT_VERSION}")
  endif()

  # The target platform, not the host one. A cross build describes what it
  # built for, and a package whose manifest says otherwise is refused by the
  # verifier on the machine that would have run it.
  if(WIN32)
    set(package_os "windows")
  elseif(APPLE)
    set(package_os "macos")
  else()
    set(package_os "linux")
  endif()

  if(GPGFRONTEND_QT5_BUILD)
    set(gf_qt_version "${Qt5_VERSION}")
  else()
    set(gf_qt_version "${Qt6_VERSION}")
  endif()

  set(package_dir "${CMAKE_BINARY_DIR}/artifacts/module-packages")
  set(package_file "${package_dir}/${GAMP_TARGET}.gfmodule")

  set(packager_args
    --output "${package_file}"
    --id "${GAMP_MODULE_ID}"
    --version "${GAMP_VERSION}"
    --sdk-abi "${GF_SDK_ABI_VERSION}"
    --min-host-version "${GAMP_MIN_HOST_VERSION}"
    --os "${package_os}"
    --arch "${CMAKE_SYSTEM_PROCESSOR}"
    --qt "${gf_qt_version}")

  foreach(capability IN LISTS GAMP_CAPABILITIES)
    list(APPEND packager_args --capability "${capability}")
  endforeach()

  foreach(entry IN LISTS GAMP_META)
    list(APPEND packager_args --meta "${entry}")
  endforeach()

  foreach(entry IN LISTS GAMP_RESOURCES)
    list(APPEND packager_args --file "${entry}")
  endforeach()

  # The module binary itself, named by its own extension so a package built on
  # one platform is not silently loadable on another.
  list(APPEND packager_args
    --file "bin/$<TARGET_FILE_NAME:${module_target}>=$<TARGET_FILE:${module_target}>")

  add_custom_command(
    OUTPUT "${package_file}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${package_dir}"
    COMMAND gf_module_packager ${packager_args}
    DEPENDS ${module_target} gf_module_packager
    COMMENT "Packaging ${GAMP_TARGET}.gfmodule"
    VERBATIM)

  add_custom_target(${module_target}_package ALL DEPENDS "${package_file}")
endfunction()
