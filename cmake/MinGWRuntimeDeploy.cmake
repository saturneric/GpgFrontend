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

# Captured while this file is read, so it is right whoever calls the function.
set(GF_MINGW_RUNTIME_DEPLOY_SCRIPT
  "${CMAKE_CURRENT_LIST_DIR}/DeployMinGWRuntime.cmake")

# gf_add_mingw_runtime_deploy(<name>
#   TARGETS <target>...           executables, shared and module libraries
#   SEARCH_DIRS <dir>...          where third-party DLLs may be copied from
#   DESTINATION <dir>             where they go; generator expressions allowed
#   [EXTRA_DLLS <name>...]        loaded at runtime by name, so never imported
#   [ALLOW_UNRESOLVED <bool>]     warn instead of fail (development only)
#   [OBJDUMP <path>]              defaults to CMAKE_OBJDUMP
#   [SYSTEM_DIRS <dir>...])       defaults to %SystemRoot%; for tests
#
# Creates two targets:
#
#   <name>          in ALL. Reruns when a scanned binary is rebuilt, a source
#                   DLL changes, or a deployed copy goes missing (a depfile
#                   written by the script lists all three).
#   <name>_rescan   not in ALL, and runs every time it is asked for. Packaging
#                   asks for it after windeployqt has added Qt's plugins, so
#                   their dependencies are collected too.
#
# Both run the same script against the same manifest. The rescan deliberately
# depends on NOTHING, not even <name>: it is for a tree that has already been
# built, and asking for it must never build anything. With a dependency, the
# packaging step's own clean-up (it deletes the static libraries from
# artifacts/bin) made Ninja relink every module after its descriptor had been
# signed, and every module was then refused. Run on a tree that was never
# built, it fails on the first missing binary instead.
#
# Static, object and interface libraries are skipped: they have no import
# table of their own, and what they pull in is in the binary they link into.
function(gf_add_mingw_runtime_deploy name)
  cmake_parse_arguments(GMRD
    ""
    "DESTINATION;ALLOW_UNRESOLVED;OBJDUMP"
    "TARGETS;SEARCH_DIRS;EXTRA_DLLS;SYSTEM_DIRS"
    ${ARGN})

  if(GMRD_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "gf_add_mingw_runtime_deploy: unknown arguments ${GMRD_UNPARSED_ARGUMENTS}")
  endif()
  foreach(required IN ITEMS TARGETS SEARCH_DIRS DESTINATION)
    if(NOT GMRD_${required})
      message(FATAL_ERROR "gf_add_mingw_runtime_deploy: ${required} is required")
    endif()
  endforeach()

  if(NOT GMRD_OBJDUMP)
    set(GMRD_OBJDUMP "${CMAKE_OBJDUMP}")
  endif()
  if(NOT GMRD_OBJDUMP)
    message(FATAL_ERROR
      "gf_add_mingw_runtime_deploy: no objdump (CMAKE_OBJDUMP is empty); "
      "install binutils for this toolchain")
  endif()

  if(GMRD_ALLOW_UNRESOLVED)
    set(allow_unresolved TRUE)
  else()
    set(allow_unresolved FALSE)
  endif()

  # Bracket arguments, so a path with spaces or a `;` survives the round trip
  # through the manifest unchanged.
  set(executables "")
  set(libraries "")
  set(modules "")
  set(target_deps "")
  set(file_deps "")
  foreach(target IN LISTS GMRD_TARGETS)
    if(NOT TARGET ${target})
      message(FATAL_ERROR "gf_add_mingw_runtime_deploy: no target ${target}")
    endif()
    get_target_property(type ${target} TYPE)
    if(type STREQUAL "EXECUTABLE")
      string(APPEND executables "  [==[$<TARGET_FILE:${target}>]==]\n")
    elseif(type STREQUAL "SHARED_LIBRARY")
      string(APPEND libraries "  [==[$<TARGET_FILE:${target}>]==]\n")
    elseif(type STREQUAL "MODULE_LIBRARY")
      string(APPEND modules "  [==[$<TARGET_FILE:${target}>]==]\n")
    else()
      message(VERBOSE "MinGW runtime: ${target} is ${type}, not scanned")
      continue()
    endif()

    # A target name gives ordering and, for a built binary, a file
    # dependency; an imported one has no rule, so its file is named instead.
    get_target_property(imported ${target} IMPORTED)
    if(NOT imported)
      list(APPEND target_deps ${target})
    endif()
    list(APPEND file_deps "$<TARGET_FILE:${target}>")
  endforeach()
  if(NOT file_deps)
    message(FATAL_ERROR
      "gf_add_mingw_runtime_deploy: none of ${GMRD_TARGETS} has an import table")
  endif()

  set(search_dirs "")
  foreach(dir IN LISTS GMRD_SEARCH_DIRS)
    string(APPEND search_dirs "  [==[${dir}]==]\n")
  endforeach()
  set(extra_dlls "")
  foreach(dll IN LISTS GMRD_EXTRA_DLLS)
    string(APPEND extra_dlls "  [==[${dll}]==]\n")
  endforeach()
  set(system_dirs "")
  if(DEFINED GMRD_SYSTEM_DIRS)
    set(system_dirs "set(GF_DEPLOY_SYSTEM_DIRS\n")
    foreach(dir IN LISTS GMRD_SYSTEM_DIRS)
      string(APPEND system_dirs "  [==[${dir}]==]\n")
    endforeach()
    string(APPEND system_dirs ")\n")
  endif()

  # One manifest per configuration, so a multi-config generator has one
  # self-consistent set of paths for each.
  set(deploy_dir "${CMAKE_BINARY_DIR}/deploy")
  set(base "${deploy_dir}/${name}-$<CONFIG>")
  set(manifest "${base}.cmake")
  set(stamp "${base}.stamp")
  set(depfile "${base}.d")

  file(GENERATE OUTPUT "${manifest}" CONTENT
"# Generated by gf_add_mingw_runtime_deploy(${name}); do not edit.
set(GF_DEPLOY_EXECUTABLES
${executables})
set(GF_DEPLOY_LIBRARIES
${libraries})
set(GF_DEPLOY_MODULES
${modules})
set(GF_DEPLOY_SEARCH_DIRS
${search_dirs})
set(GF_DEPLOY_EXTRA_DLLS
${extra_dlls})
${system_dirs}set(GF_DEPLOY_DEST [==[${GMRD_DESTINATION}]==])
set(GF_DEPLOY_BINARY_DIR [==[${CMAKE_BINARY_DIR}]==])
set(GF_DEPLOY_SCAN_DIR [==[${base}.scan]==])
set(GF_DEPLOY_REPORT [==[${base}.txt]==])
set(GF_DEPLOY_OBJDUMP [==[${GMRD_OBJDUMP}]==])
set(GF_DEPLOY_ALLOW_UNRESOLVED ${allow_unresolved})
")

  set(script "${GF_MINGW_RUNTIME_DEPLOY_SCRIPT}")

  add_custom_command(
    OUTPUT "${stamp}"
    COMMAND "${CMAKE_COMMAND}"
      "-DGF_DEPLOY_MANIFEST=${manifest}"
      "-DGF_DEPLOY_STAMP=${stamp}"
      "-DGF_DEPLOY_DEPFILE=${depfile}"
      -DGF_DEPLOY_SCAN_PLUGINS=ON
      -P "${script}"
    DEPENDS "${script}" "${manifest}" ${target_deps} ${file_deps}
    DEPFILE "${depfile}"
    COMMENT "Deploying MinGW runtime DLLs"
    VERBATIM)
  add_custom_target(${name} ALL DEPENDS "${stamp}")

  add_custom_target(${name}_rescan
    COMMAND "${CMAKE_COMMAND}"
      "-DGF_DEPLOY_MANIFEST=${manifest}"
      -DGF_DEPLOY_SCAN_PLUGINS=ON
      -P "${script}"
    COMMENT "Rescanning MinGW runtime DLLs, Qt plugins included"
    VERBATIM)
endfunction()
