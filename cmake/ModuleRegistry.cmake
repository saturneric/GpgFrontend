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
    "TARGET_NAME;SHORT_NAME;MODULE_ID;VERSION;MIN_HOST_VERSION;TRANSLATION_CONTEXT"
    "CAPABILITIES;EVENTS;META;RESOURCES"
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

  # One namespace per module, and the dev tree IS the deployment tree:
  #
  #   modules/<key>/module.gfmodule
  #   modules/<key>/native/lib<entry>.so
  #
  # The directory name is derived from the module id, never chosen, and the
  # Host recomputes it and refuses a descriptor found anywhere else. The
  # descriptor filename is fixed, which retires the old package-versus-library
  # name reconciliation entirely: the two used to be named for different
  # things (the CMake target and the SDK prefix) and had to be matched up.
  gf_module_directory_key("${GAMP_MODULE_ID}" namespace_key)
  set(package_dir "${GPGFRONTEND_MODULE_NAMESPACE_ROOT}/${namespace_key}")
  set(package_file "${package_dir}/module.gfmodule")

  set(packager_args
    --output "${package_file}"
    --id "${GAMP_MODULE_ID}"
    --version "${GAMP_VERSION}"
    --sdk-abi "${GF_SDK_ABI_VERSION}"
    --min-host-version "${GAMP_MIN_HOST_VERSION}"
    --signing-seed "${GF_MODULE_BUILD_KEY_DIR}/module-build.seed"
    --build-id "${GPGFRONTEND_BUILD_ID}"
    --os "${package_os}"
    --arch "${CMAKE_SYSTEM_PROCESSOR}"
    --qt "${gf_qt_version}")

  if(GAMP_TRANSLATION_CONTEXT)
    list(APPEND packager_args
      --translation-context "${GAMP_TRANSLATION_CONTEXT}")
  endif()

  foreach(capability IN LISTS GAMP_CAPABILITIES)
    list(APPEND packager_args --capability "${capability}")
  endforeach()

  foreach(event IN LISTS GAMP_EVENTS)
    list(APPEND packager_args --event "${event}")
  endforeach()

  foreach(entry IN LISTS GAMP_META)
    list(APPEND packager_args --meta "${entry}")
  endforeach()

  foreach(entry IN LISTS GAMP_RESOURCES)
    list(APPEND packager_args --file "${entry}")
  endforeach()

  # The entry native. Bound by the descriptor, and deliberately NOT packaged
  # into it: executable code never travels inside a descriptor. The logical
  # name is the CMake target name, which is what the Host maps back to a
  # filename using this toolchain's own library prefix and suffix.
  list(APPEND packager_args
    --entry-native "name=${module_target},file=$<TARGET_FILE:${module_target}>")

  # The finalize half of the release pipeline (see ModulePreparedEntry.h).
  #
  # OFF for an ordinary build, because there is nothing to seal against: the
  # native was linked by this same build graph moments ago. It is turned ON
  # only for the reconfigure CI does after platform deployment has rewritten
  # those natives, and then it is fail-closed -- a namespace with no seal, or
  # one whose native no longer binds to what was sealed, stops the build rather
  # than producing a descriptor that describes the file as it used to be.
  if(GPGFRONTEND_MODULE_REQUIRE_SEAL)
    list(APPEND packager_args
      --prepared-manifest "${package_dir}/native/prepared.json")
  endif()

  add_custom_command(
    OUTPUT "${package_file}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${package_dir}"
    COMMAND gf_module_packager ${packager_args}
    # The target FILE, not just the target: the descriptor records a value
    # computed from those bytes, so a relink has to re-run this even when the
    # target itself is considered up to date.
    DEPENDS $<TARGET_FILE:${module_target}> gf_module_packager
            "${GF_MODULE_TRUST_ROOT_SOURCE}"
    COMMENT "Packaging ${GAMP_SHORT_NAME}.gfmodule"
    VERBATIM)

  add_custom_target(${module_target}_package ALL DEPENDS "${package_file}")
endfunction()

# ---------------------------------------------------------------------------
# gf_add_module — one call per module, everything else from module.json
# ---------------------------------------------------------------------------

# Where GFModuleIdentity.h.in lives, captured while this file is being read so
# it is right regardless of which directory calls the function later.
set(GF_MODULE_REGISTRY_DIR "${CMAKE_CURRENT_LIST_DIR}")

# Start every configure from an empty list.
#
# This is a CACHE variable that gf_add_module() appends to, and it used to only
# ever grow: renaming a module left the OLD target name in it permanently, and
# deleting one left a name that referred to nothing. src/CMakeLists.txt feeds
# this list to XCODE_EMBED_PLUGINS, so a phantom entry is a broken Xcode build
# that no source change explains. This file is included exactly once, from the
# top-level CMakeLists, before any module is added -- so here is the one moment
# when clearing it is unambiguously right.
set(GPGFRONTEND_MODULE_TARGETS "" CACHE INTERNAL "All modules" FORCE)
set(GPGFRONTEND_MODULE_DIRECTORY_KEYS "" CACHE INTERNAL
  "module id=directory key, as CMake derives them" FORCE)


# The directory a module owns, derived from the identity it signs.
#
# THIS MUST AGREE, CHARACTER FOR CHARACTER, WITH
# ModuleDirectoryKey() in src/core/module/ModuleNamespace.cpp.
#
# It exists twice because the two halves run at different times: CMake places
# the build output, and the Host resolves it again at load. There is no way to
# share one implementation across that boundary -- a host tool that could
# compute it is not built yet when the output directory has to be named -- so
# the agreement is pinned by a test instead (GpgCoreTestModuleNamespace.cpp),
# which compares this function's answer for every in-tree module against the
# C++ one.
#
# The suffix is hex rather than base32 for exactly this reason: CMake has
# string(SHA256) and substrings, and hand-rolling a base32 here to save four
# characters would buy a way for the two to disagree in silence.
function(gf_module_directory_key module_id out_var)
  if(module_id STREQUAL "")
    set(${out_var} "" PARENT_SCOPE)
    return()
  endif()

  # The readable half: the final dotted component, reduced to a safe spelling.
  string(REGEX REPLACE "^.*\\." "" leaf "${module_id}")
  string(TOLOWER "${leaf}" leaf)
  string(REPLACE "_" "-" leaf "${leaf}")
  string(REGEX REPLACE "[^a-z0-9-]" "" leaf "${leaf}")
  string(REGEX REPLACE "^-+" "" leaf "${leaf}")
  string(REGEX REPLACE "-+$" "" leaf "${leaf}")

  # A component that does not begin with a letter is prefixed rather than
  # rejected, so an id ending in a digit is a naming choice and not a build
  # failure a long way from its cause.
  if(NOT leaf MATCHES "^[a-z]")
    set(leaf "m${leaf}")
  endif()

  string(LENGTH "${leaf}" leaf_len)
  if(leaf_len GREATER 24)
    string(SUBSTRING "${leaf}" 0 24 leaf)
    # Truncation can strand a separator, which would produce two adjacent
    # dashes once the suffix is joined on.
    string(REGEX REPLACE "-+$" "" leaf "${leaf}")
  endif()

  # Over the id exactly as the manifest spells it. First 80 bits.
  string(SHA256 digest "${module_id}")
  string(SUBSTRING "${digest}" 0 20 suffix)

  set(${out_var} "${leaf}-${suffix}" PARENT_SCOPE)
endfunction()

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

# The same, but tolerating an absent key. For a field being introduced across
# two repositories, where the modules cannot gain it in the same commit that
# starts reading it.
function(_gf_module_json_string_array_optional json file key out_var)
  string(JSON type ERROR_VARIABLE type_err TYPE "${json}" ${key})
  if(type_err)
    set(${out_var} "" PARENT_SCOPE)
    return()
  endif()
  _gf_module_json_string_array("${json}" "${file}" ${key} values)
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

  # events: the subscription allowlist, required. The runtime subscribes to
  # exactly these and refuses to activate if the module's handler table
  # disagrees, so an omission here is not a smaller claim -- it is no claim.
  _gf_module_json_string_array("${manifest_json}" "${manifest_file}"
    events module_events)
  foreach(event IN LISTS module_events)
    if(NOT event MATCHES "^[A-Z][A-Z0-9_]*$")
      message(FATAL_ERROR
        "${manifest_file}: event \"${event}\" is not an upper-case identifier")
    endif()
  endforeach()
  # Sorted and de-duplicated, so the canonical manifest -- which the signature
  # covers -- is byte-identical however the file happened to be ordered.
  list(LENGTH module_events _gf_events_raw)
  list(SORT module_events)
  list(REMOVE_DUPLICATES module_events)
  list(LENGTH module_events _gf_events_uniq)
  if(NOT _gf_events_raw EQUAL _gf_events_uniq)
    message(FATAL_ERROR "${manifest_file}: \"events\" contains duplicates")
  endif()

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
  # The runtime first: its undefined SDK symbols are resolved by gf_sdk, which
  # follows it on the link line. Nothing forces the archive open -- the
  # module's own GFModuleGetApi references GFModuleRuntimeGetApi, and that
  # reference is what makes the linker keep the entry point.
  target_link_libraries(${target_name} PRIVATE gf_module_runtime gf_sdk)

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

  # The module's own namespace: modules/<key>/native. Derived from the id it
  # signs, so the build tree is laid out the way a shipped tree is and a
  # development build exercises the same resolution a user's does.
  #
  # It also gives the platform loader a module-local directory to resolve
  # private dependencies from -- $ORIGIN on Linux, @loader_path on macOS, the
  # DLL load directory on Windows -- which a single flat directory could not.
  gf_module_directory_key("${module_id}" target_namespace_key)
  set(target_native_dir
    "${GPGFRONTEND_MODULE_NAMESPACE_ROOT}/${target_namespace_key}/native")
  # Pinned, not merely set: on Xcode a plain RUNTIME_OUTPUT_DIRECTORY gains a
  # per-config subdirectory, and the entry native must be a DIRECT child of
  # native/ or the Host refuses to resolve it.
  gf_pin_output_directory(${target_name} "${target_native_dir}")

  if(APPLE)
    # The GpgFrontend binding, placed in its own Mach-O section at LINK time.
    #
    # macOS binds its entry by this identifier rather than by a digest of the
    # file, because Apple signing rewrites __LINKEDIT and the App Store may
    # re-sign again on download -- so a digest would either forbid normal
    # platform signing or have to be regenerated after it, which is what would
    # drag a build-produced packager into a privileged signing job.
    #
    # Sections survive both: codesign appends and rewrites load commands, and
    # install_name_tool rewrites load commands, and neither touches section
    # contents. The reader walks load commands rather than file offsets, so it
    # is indifferent to both.
    #
    # It must be here, at link time, because everything it is derived from --
    # module id, build id, SDK ABI -- is known at configure time and the
    # section cannot be added afterwards without invalidating a signature.
    # Computed by gf_module_packager, never here. The derivation hashes
    # NUL-separated fields and a CMake string cannot hold a NUL, so a CMake
    # implementation could not agree with the runtime's even in principle.
    set(binding_file "${CMAKE_CURRENT_BINARY_DIR}/${target_name}_binding.bin")

    add_custom_command(
      OUTPUT "${binding_file}"
      COMMAND gf_module_packager binding-id
        --id "${module_id}"
        --build-id "${GPGFRONTEND_BUILD_ID}"
        --sdk-abi "${GF_SDK_ABI_VERSION}"
        --output "${binding_file}"
      DEPENDS gf_module_packager "${GF_MODULE_TRUST_ROOT_SOURCE}"
      COMMENT "Binding id for ${target_name}"
      VERBATIM)

    add_custom_target(${target_name}_binding DEPENDS "${binding_file}")
    add_dependencies(${target_name} ${target_name}_binding)

    target_link_options(${target_name} PRIVATE
      "LINKER:-sectcreate,__GPGFRONTEND,__gf_binding,${binding_file}")
  endif()

  if(APPLE)
    # Where a module looks for Qt and the gf_* libraries.
    #
    # Set at LINK time rather than patched in later with install_name_tool:
    # adding a load command after the fact needs header padding that may not be
    # there, and a failure would show up as a module that does not load rather
    # than as a build that does not finish.
    #
    # Both hops, for the same reason Linux carries both -- the two layouts put
    # the host's libraries at different depths, and a load command that
    # resolves to nothing costs one failed stat:
    #
    #   bundle   Contents/Frameworks/GpgFrontendModules/<key>/  -> ../..
    #   dev tree artifacts/modules/<key>/native/                -> ../../..
    #
    # @loader_path itself is first, so the private dependencies bundled beside
    # the entry resolve without any search widening -- the same module-local
    # namespace $ORIGIN gives on Linux.
    set_target_properties(${target_name} PROPERTIES
      INSTALL_RPATH "@loader_path;@loader_path/../..;@loader_path/../../.."
      BUILD_WITH_INSTALL_RPATH TRUE)
  endif()

  if(NOT WIN32 AND NOT APPLE)
    # $ORIGIN first, so a private helper beside the entry resolves without any
    # search path being widened. Then the hop up to wherever the host's own
    # libraries are, which is a property of the LAYOUT and differs between the
    # two the tree produces:
    #
    #   dev tree   artifacts/modules/<key>/native      -> ../../..
    #   AppImage   usr/lib/gpgfrontend/modules/<key>/native -> ../../../..
    #
    # Both are listed rather than switched on BUILD_APP_IMAGE. A RUNPATH entry
    # that does not exist costs one failed stat at load time and nothing else,
    # while picking the wrong one is a startup failure -- and the two trees are
    # built by the same code path, so keeping them in step by hand is exactly
    # the kind of bookkeeping that quietly stops being true. The dependency
    # audit asserts reachability against a real tree either way, which is the
    # check that matters.
    set_target_properties(${target_name} PROPERTIES
      INSTALL_RPATH "$ORIGIN:$ORIGIN/../../..:$ORIGIN/../../../.."
      BUILD_WITH_INSTALL_RPATH TRUE)
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

  # ---- the namespace key CMake computed, recorded for the test ------------
  #
  # gf_module_directory_key() has to agree with ModuleDirectoryKey() in
  # gf_core, and nothing in a build failure would say otherwise if it drifted:
  # the directory would simply be named something the Host later refuses to
  # find. Writing the pairs out is what lets a unit test recompute them and
  # compare, which is the only place the two implementations ever meet.
  gf_module_directory_key("${module_id}" module_dir_key)

  set(keys "${GPGFRONTEND_MODULE_DIRECTORY_KEYS}")
  list(APPEND keys "${module_id}=${module_dir_key}")
  list(REMOVE_DUPLICATES keys)
  set(GPGFRONTEND_MODULE_DIRECTORY_KEYS "${keys}"
    CACHE INTERNAL "module id=directory key, as CMake derives them" FORCE)

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
    TRANSLATION_CONTEXT "${module_translation_context}"
    CAPABILITIES ${module_capabilities}
    EVENTS       ${module_events}
    META         "Name=${module_name}"
                 "Description=${module_description}"
                 "Author=${module_author}"
    RESOURCES    ${GAM_RESOURCES})

  # ---- what ships is the namespace: descriptor AND native ----------------

  # Both, now, and that is the change. The native library used to be built and
  # deliberately not installed, because it travelled inside the package; it is
  # now an ordinary platform file that the descriptor binds by digest. Shipping
  # it is what lets $ORIGIN, @loader_path, debuggers, dependency scanners and
  # platform code signing all treat it as what it is.
  #
  # "No loose module binaries in a shipping tree" is therefore no longer the
  # invariant. The invariant is that every one of them is bound by exactly one
  # verified descriptor, which is a thing gf_module_tool can check and a
  # directory listing cannot.
  # RELATIVE, deliberately. This used to be CMAKE_INSTALL_FULL_LIBDIR, which is
  # absolute and frozen at configure time -- so `cmake --install --prefix /opt`
  # put the binary under /opt and then tried to write the modules into
  # /usr/local/lib, failing outright without root. A downstream packager could
  # not install this project at all, and nothing noticed because nothing in the
  # repository ever ran `cmake --install`.
  gf_module_directory_key("${module_id}" install_namespace_key)
  set(install_namespace
    "${CMAKE_INSTALL_LIBDIR}/gpgfrontend/modules/${install_namespace_key}")

  install(FILES
    "${GPGFRONTEND_MODULE_NAMESPACE_ROOT}/${install_namespace_key}/module.gfmodule"
    DESTINATION "${install_namespace}"
    COMPONENT runtime)

  # ARCHIVE is omitted on purpose: an import library is a build input, and on
  # Windows it is what used to be swept out of the payload by hand afterwards.
  install(TARGETS ${target_name}
    LIBRARY DESTINATION "${install_namespace}/native" COMPONENT Runtime
    RUNTIME DESTINATION "${install_namespace}/native" COMPONENT runtime)
endfunction()
