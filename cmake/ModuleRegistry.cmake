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

# The packaging rule: how a `*.gfmodule` is produced.
#
# Signed with a key generated for that build and destroyed with it. There is no
# CI-specific environment and no key to configure: a developer runs the normal
# build and gets a signed package.
#
# What that signature proves is narrow, and is spelled out in full on
# VerifyModulePackage(): the public key travels inside the package, so it
# establishes that the package agrees with itself, not who built it.
function(_gf_module_package_command)
  cmake_parse_arguments(GAMP
    ""
    "TARGET_NAME;SHORT_NAME;MODULE_ID;VERSION;MIN_HOST_VERSION"
    "CAPABILITIES;META;RESOURCES"
    ${ARGN})

  set(module_target "${GAMP_TARGET_NAME}")

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
  set(package_file "${package_dir}/${GAMP_SHORT_NAME}.gfmodule")

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
    COMMENT "Packaging ${GAMP_SHORT_NAME}.gfmodule"
    VERBATIM)

  add_custom_target(${module_target}_package ALL DEPENDS "${package_file}")

  # Where the application actually looks. The loose-library rule installed to
  # ${CMAKE_INSTALL_LIBDIR} while GlobalSettingStation searches
  # ${CMAKE_INSTALL_FULL_LIBDIR}/gpgfrontend/modules, so nothing installed that
  # way was ever found.
  set(GF_MODULE_PACKAGE_FILE_${GAMP_SHORT_NAME} "${package_file}"
    CACHE INTERNAL "package for ${GAMP_SHORT_NAME}" FORCE)
endfunction()

# ---------------------------------------------------------------------------
# gf_add_module — one call per module, everything else from module.json
# ---------------------------------------------------------------------------

# Where GFModuleIdentity.h.in lives, captured while this file is being read so
# it is right regardless of which directory calls the function later.
set(GF_MODULE_REGISTRY_DIR "${CMAKE_CURRENT_LIST_DIR}")

# Read a required string out of a module.json, or stop the configure.
#
# The inversion is deliberate and matches the runtime manifest parser: a
# missing or wrongly-typed field is a hard failure, never a default. A module
# that silently acquires an empty author is a module whose metadata nobody can
# trust, and the moment to find out is now rather than at load time.
function(_gf_module_json_string json file key out_var)
  string(JSON value ERROR_VARIABLE err GET "${json}" ${key})
  if(err)
    message(FATAL_ERROR "${file}: missing required field \"${key}\"")
  endif()

  string(JSON type ERROR_VARIABLE type_err TYPE "${json}" ${key})
  if(type_err OR NOT type STREQUAL "STRING")
    message(FATAL_ERROR "${file}: \"${key}\" must be a string, got ${type}")
  endif()
  if(value STREQUAL "")
    message(FATAL_ERROR "${file}: \"${key}\" must not be empty")
  endif()

  set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

# Read an optional string, leaving out_var untouched when absent.
function(_gf_module_json_optional_string json file key out_var)
  string(JSON type ERROR_VARIABLE type_err TYPE "${json}" ${key})
  if(type_err)
    return()
  endif()
  if(NOT type STREQUAL "STRING")
    message(FATAL_ERROR "${file}: \"${key}\" must be a string, got ${type}")
  endif()
  string(JSON value GET "${json}" ${key})
  set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

# Read a required array of strings into a CMake list.
function(_gf_module_json_string_array json file key out_var)
  string(JSON type ERROR_VARIABLE type_err TYPE "${json}" ${key})
  if(type_err)
    message(FATAL_ERROR "${file}: missing required field \"${key}\"")
  endif()
  if(NOT type STREQUAL "ARRAY")
    message(FATAL_ERROR "${file}: \"${key}\" must be an array, got ${type}")
  endif()

  string(JSON count LENGTH "${json}" ${key})
  set(values)
  if(count GREATER 0)
    math(EXPR last "${count} - 1")
    foreach(i RANGE ${last})
      string(JSON element_type TYPE "${json}" ${key} ${i})
      if(NOT element_type STREQUAL "STRING")
        message(FATAL_ERROR
          "${file}: \"${key}\"[${i}] must be a string, got ${element_type}")
      endif()
      string(JSON element GET "${json}" ${key} ${i})
      list(APPEND values "${element}")
    endforeach()
  endif()

  set(${out_var} "${values}" PARENT_SCOPE)
endfunction()

# gf_add_module(
#   NAME         <short name>          -> target gf_mod_<name>
#   QT           <component>...        -> Qt::<component>
#   [SOURCES     <file>...]            defaults to aux_source_directory(.)
#   [UI_DIR      <dir>]                added to CMAKE_AUTOUIC_SEARCH_PATHS
#   [LINK        <target>...]          extra libraries to link privately
#   [INCLUDE_DIRS <dir>...]            extra private include directories
#   [RESOURCES   <archive path>=<source file>...])
#
# Everything about the module's identity -- id, version, name, description,
# author, capabilities, minimum host version, translation context -- comes from
# `module.json` beside the caller's CMakeLists.txt. That file is the single
# source: it feeds the generated C++ identity header, the translation wiring and
# the signed package manifest, so the values cannot drift apart because there is
# only one of each.
#
# This replaces register_module() + target_link_libraries() +
# gpgfrontend_collect_ts_files() + module_add_translations() +
# gf_add_module_package(), which between them repeated the same five values in
# two languages.
function(gf_add_module)
  cmake_parse_arguments(GAM
    ""
    "NAME;UI_DIR"
    "QT;SOURCES;LINK;INCLUDE_DIRS;RESOURCES"
    ${ARGN})

  if(NOT GAM_NAME)
    message(FATAL_ERROR "gf_add_module: NAME is required")
  endif()

  # ---- module.json, validated strictly -------------------------------------

  set(manifest_file "${CMAKE_CURRENT_SOURCE_DIR}/module.json")
  if(NOT EXISTS "${manifest_file}")
    message(FATAL_ERROR
      "gf_add_module(${GAM_NAME}): no module.json beside this CMakeLists.txt")
  endif()
  file(READ "${manifest_file}" manifest_json)

  string(JSON schema_type ERROR_VARIABLE schema_err TYPE "${manifest_json}"
    schema_version)
  if(schema_err OR NOT schema_type STREQUAL "NUMBER")
    message(FATAL_ERROR
      "${manifest_file}: \"schema_version\" must be a number")
  endif()
  string(JSON schema_version GET "${manifest_json}" schema_version)
  if(NOT schema_version EQUAL 1)
    message(FATAL_ERROR
      "${manifest_file}: schema_version ${schema_version} is not supported")
  endif()

  _gf_module_json_string("${manifest_json}" "${manifest_file}" id module_id)
  _gf_module_json_string("${manifest_json}" "${manifest_file}" version
    module_version)
  _gf_module_json_string("${manifest_json}" "${manifest_file}" name module_name)
  _gf_module_json_string("${manifest_json}" "${manifest_file}" description
    module_description)
  _gf_module_json_string("${manifest_json}" "${manifest_file}" author
    module_author)
  _gf_module_json_string("${manifest_json}" "${manifest_file}"
    translation_context module_translation_context)
  _gf_module_json_string_array("${manifest_json}" "${manifest_file}"
    capabilities module_capabilities)

  set(module_min_host_version "${PROJECT_VERSION}")
  _gf_module_json_optional_string("${manifest_json}" "${manifest_file}"
    min_host_version module_min_host_version)

  # ---- the target ----------------------------------------------------------

  set(module_sources "${GAM_SOURCES}")
  if(NOT module_sources)
    aux_source_directory(. module_sources)
  endif()
  if(NOT module_sources)
    message(FATAL_ERROR "gf_add_module(${GAM_NAME}): no sources to build")
  endif()

  set(target_name "gf_mod_${GAM_NAME}")
  add_library(${target_name} SHARED ${module_sources})

  set_target_properties(${target_name} PROPERTIES POSITION_INDEPENDENT_CODE ON)
  target_compile_features(${target_name} PRIVATE cxx_std_17)
  target_link_libraries(${target_name} PRIVATE gf_sdk)

  foreach(component IN LISTS GAM_QT)
    target_link_libraries(${target_name} PRIVATE Qt::${component})
  endforeach()

  if(GAM_LINK)
    target_link_libraries(${target_name} PRIVATE ${GAM_LINK})
  endif()
  if(GAM_INCLUDE_DIRS)
    target_include_directories(${target_name} PRIVATE ${GAM_INCLUDE_DIRS})
  endif()

  if(GAM_UI_DIR)
    set(CMAKE_AUTOUIC_SEARCH_PATHS
      ${CMAKE_AUTOUIC_SEARCH_PATHS} "${CMAKE_CURRENT_SOURCE_DIR}/${GAM_UI_DIR}"
      PARENT_SCOPE)
    set_property(TARGET ${target_name} APPEND PROPERTY
      AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/${GAM_UI_DIR}")
  endif()

  if(XCODE_BUILD)
    set_target_properties(${target_name} PROPERTIES
      XCODE_ATTRIBUTE_SKIP_INSTALL "Yes"
      XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${GPGFRONTEND_XCODE_CODE_SIGN_IDENTITY}")
  endif()

  set(current "${GPGFRONTEND_MODULE_TARGETS}")
  list(APPEND current "${target_name}")
  list(REMOVE_DUPLICATES current)
  set(GPGFRONTEND_MODULE_TARGETS "${current}" CACHE INTERNAL "All modules" FORCE)

  # ---- generated identity, minimal ----------------------------------------

  set(GF_MODULE_ID "${module_id}")
  set(GF_MODULE_VERSION "${module_version}")
  set(GF_MODULE_TRANSLATION_CONTEXT "${module_translation_context}")
  configure_file(
    "${GF_MODULE_REGISTRY_DIR}/GFModuleIdentity.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/GFModuleIdentity.h"
    @ONLY)
  target_include_directories(${target_name} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

  # ---- translations, context from the manifest ----------------------------

  gpgfrontend_collect_ts_files(ts_files
    "${module_translation_context}" "${CMAKE_CURRENT_SOURCE_DIR}/ts")
  module_add_translations(${target_name}
    TS_FILES ${ts_files}
    SOURCES ${module_sources}
    INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR})

  # ---- the signed package, which is what ships ----------------------------

  _gf_module_package_command(
    TARGET_NAME  ${target_name}
    SHORT_NAME   ${GAM_NAME}
    MODULE_ID    "${module_id}"
    VERSION      "${module_version}"
    MIN_HOST_VERSION "${module_min_host_version}"
    CAPABILITIES ${module_capabilities}
    META         "Name=${module_name}"
                 "Description=${module_description}"
                 "Author=${module_author}"
    RESOURCES    ${GAM_RESOURCES})

  # ---- what ships is the package, and only the package -------------------

  # The loose library is still built: it is what the packager packages, and it
  # is what a developer iterates on. It is simply not installed. A shipped
  # tree therefore holds no unsigned module binary at all, which is also what
  # makes the supersede rule in ModuleInit a development-only concern.
  #
  # The destination is the one GlobalSettingStation actually searches. The old
  # rule installed to ${CMAKE_INSTALL_LIBDIR} while the application looked in
  # ${CMAKE_INSTALL_FULL_LIBDIR}/gpgfrontend/modules, so a module installed
  # that way was never found.
  install(FILES "${CMAKE_BINARY_DIR}/artifacts/module-packages/${GAM_NAME}.gfmodule"
    DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/gpgfrontend/modules")
endfunction()
