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

# Copies the third-party DLLs the Windows build needs beside the executable,
# derived from the PE import tables of what was actually built.
#
#   cmake -DGF_DEPLOY_MANIFEST=<file> [-DGF_DEPLOY_SCAN_PLUGINS=ON]
#         [-DGF_DEPLOY_STAMP=<file> -DGF_DEPLOY_DEPFILE=<file>]
#         -P DeployMinGWRuntime.cmake
#
# The manifest is written by gf_add_mingw_runtime_deploy() (see
# MinGWRuntimeDeploy.cmake) and sets:
#
#   GF_DEPLOY_EXECUTABLES / _LIBRARIES / _MODULES  the project binaries
#   GF_DEPLOY_SEARCH_DIRS     where third-party DLLs may come from (MSYS2 bin)
#   GF_DEPLOY_DEST            where they go (artifacts/bin)
#   GF_DEPLOY_BINARY_DIR      the build tree; the scan dir must be inside it
#   GF_DEPLOY_SCAN_DIR        private scratch directory, recreated every run
#   GF_DEPLOY_REPORT          the deployed DLL names, one per line
#   GF_DEPLOY_OBJDUMP         the toolchain's objdump
#   GF_DEPLOY_EXTRA_DLLS      DLLs loaded at runtime by name, not imported
#   GF_DEPLOY_ALLOW_UNRESOLVED  warn instead of fail (development only)
#   GF_DEPLOY_SYSTEM_DIRS     optional; Windows' own directories
#
# Why the project binaries are scanned from a private copy rather than in
# place: Windows resolves a dependency in the depending file's own directory
# first, so scanning artifacts/bin would resolve against the DLLs a previous
# run copied there. A stale copy would then be traversed instead of the
# current one in MSYS2, and its dependencies would be the wrong ones. In the
# scan directory the only neighbours are other project binaries, and every
# third-party DLL is traversed from where it really comes from.
#
# Nothing in the destination is ever deleted.

cmake_minimum_required(VERSION 3.24)

# Match against normalized paths. A Windows host reports system DLLs as
# `C:\Windows\system32/x.dll`, which the forward-slash exclusions below would
# otherwise never match: harmless to the result, since such a path is still
# recognised as a system DLL afterwards, but Windows' own DLLs would be
# traversed for nothing and every one of them warned about.
if(POLICY CMP0207)
  cmake_policy(SET CMP0207 NEW)
endif()

function(_gf_deploy_fail)
  string(JOIN "" _msg ${ARGN})
  message(FATAL_ERROR "MinGW runtime deployment: ${_msg}")
endfunction()

# Forward slashes, no `.`/`..`, no trailing slash (except a root).
function(_gf_deploy_normalize path out_var)
  file(TO_CMAKE_PATH "${path}" _p)
  cmake_path(SET _p NORMALIZE "${_p}")
  if(_p MATCHES "^(.*[^:/])/+$")
    set(_p "${CMAKE_MATCH_1}")
  endif()
  set(${out_var} "${_p}" PARENT_SCOPE)
endfunction()

# Windows paths compare case-insensitively.
function(_gf_deploy_path_key path out_var)
  _gf_deploy_normalize("${path}" _p)
  string(TOLOWER "${_p}" _p)
  set(${out_var} "${_p}" PARENT_SCOPE)
endfunction()

# TRUE when `path` is `dir` itself or anything below it.
function(_gf_deploy_is_within path dir out_var)
  _gf_deploy_path_key("${path}" _p)
  _gf_deploy_path_key("${dir}" _d)
  set(_result FALSE)
  if(_p STREQUAL _d)
    set(_result TRUE)
  else()
    if(NOT _d MATCHES "/$")
      string(APPEND _d "/")
    endif()
    string(FIND "${_p}" "${_d}" _at)
    if(_at EQUAL 0)
      set(_result TRUE)
    endif()
  endif()
  set(${out_var} ${_result} PARENT_SCOPE)
endfunction()

# A literal path as a case-insensitive CMake regex.
function(_gf_deploy_literal_regex text out_var)
  set(_specials "^$.|?*+()[]{}\\")
  set(_regex "")
  string(LENGTH "${text}" _len)
  if(_len GREATER 0)
    math(EXPR _last "${_len} - 1")
    foreach(_i RANGE ${_last})
      string(SUBSTRING "${text}" ${_i} 1 _c)
      string(TOLOWER "${_c}" _lower)
      string(TOUPPER "${_c}" _upper)
      string(FIND "${_specials}" "${_c}" _special)
      if(NOT _lower STREQUAL _upper)
        string(APPEND _regex "[${_upper}${_lower}]")
      elseif(_special GREATER -1)
        string(APPEND _regex "\\${_c}")
      else()
        string(APPEND _regex "${_c}")
      endif()
    endforeach()
  endif()
  set(${out_var} "${_regex}" PARENT_SCOPE)
endfunction()

# The file names in `dir`, listed once per directory: the resolver reports
# names in whatever case the import table used (lower case, on a Windows
# host), and what is copied should carry the name the file really has.
function(_gf_deploy_list_dir dir)
  _gf_deploy_path_key("${dir}" _key)
  string(MD5 _id "${_key}")
  get_property(_known GLOBAL PROPERTY "_gf_deploy_listed_${_id}" SET)
  if(_known)
    return()
  endif()
  file(GLOB _files LIST_DIRECTORIES false "${dir}/*")
  set(_names "")
  set(_keys "")
  foreach(_f IN LISTS _files)
    get_filename_component(_n "${_f}" NAME)
    string(TOLOWER "${_n}" _k)
    list(APPEND _names "${_n}")
    list(APPEND _keys "${_k}")
  endforeach()
  set_property(GLOBAL PROPERTY "_gf_deploy_listed_${_id}" TRUE)
  set_property(GLOBAL PROPERTY "_gf_deploy_names_${_id}" "${_names}")
  set_property(GLOBAL PROPERTY "_gf_deploy_keys_${_id}" "${_keys}")
endfunction()

# The on-disk name of `name` in `dir`, matched case-insensitively, or "".
function(_gf_deploy_find_in_dir dir name out_var)
  _gf_deploy_list_dir("${dir}")
  _gf_deploy_path_key("${dir}" _key)
  string(MD5 _id "${_key}")
  get_property(_names GLOBAL PROPERTY "_gf_deploy_names_${_id}")
  get_property(_keys GLOBAL PROPERTY "_gf_deploy_keys_${_id}")
  string(TOLOWER "${name}" _k)
  list(FIND _keys "${_k}" _at)
  if(_at EQUAL -1)
    set(${out_var} "" PARENT_SCOPE)
  else()
    list(GET _names ${_at} _n)
    set(${out_var} "${_n}" PARENT_SCOPE)
  endif()
endfunction()

# One gcc-style depfile path: spaces, `#` and `$` are what Ninja and Make
# would otherwise read as syntax.
function(_gf_deploy_depfile_escape path out_var)
  string(REPLACE "\\" "/" _p "${path}")
  string(REPLACE "$" "$$" _p "${_p}")
  string(REPLACE "#" "\\#" _p "${_p}")
  string(REPLACE " " "\\ " _p "${_p}")
  set(${out_var} "${_p}" PARENT_SCOPE)
endfunction()

# ---- inputs -----------------------------------------------------------------

if(NOT DEFINED GF_DEPLOY_MANIFEST OR GF_DEPLOY_MANIFEST STREQUAL "")
  _gf_deploy_fail("GF_DEPLOY_MANIFEST is not set; this script is run by the "
    "gf_deploy_runtime target, see cmake/MinGWRuntimeDeploy.cmake")
endif()
if(NOT EXISTS "${GF_DEPLOY_MANIFEST}")
  _gf_deploy_fail("the manifest does not exist: ${GF_DEPLOY_MANIFEST}. "
    "Re-run the CMake configure step.")
endif()
include("${GF_DEPLOY_MANIFEST}")

foreach(_var IN ITEMS GF_DEPLOY_DEST GF_DEPLOY_BINARY_DIR GF_DEPLOY_SCAN_DIR
                      GF_DEPLOY_SEARCH_DIRS GF_DEPLOY_OBJDUMP)
  if(NOT DEFINED ${_var} OR "${${_var}}" STREQUAL "")
    _gf_deploy_fail("${_var} is not set in ${GF_DEPLOY_MANIFEST}")
  endif()
endforeach()

if(NOT IS_ABSOLUTE "${GF_DEPLOY_OBJDUMP}" OR NOT EXISTS "${GF_DEPLOY_OBJDUMP}")
  _gf_deploy_fail("objdump not found at '${GF_DEPLOY_OBJDUMP}'. It comes from "
    "CMAKE_OBJDUMP; install binutils for this toolchain and reconfigure.")
endif()

_gf_deploy_normalize("${GF_DEPLOY_DEST}" _dest)
_gf_deploy_normalize("${GF_DEPLOY_BINARY_DIR}" _binary_dir)
_gf_deploy_normalize("${GF_DEPLOY_SCAN_DIR}" _scan_dir)
if(NOT IS_DIRECTORY "${_dest}")
  _gf_deploy_fail("the destination is not a directory: ${_dest}")
endif()

set(_search_dirs "")
foreach(_d IN LISTS GF_DEPLOY_SEARCH_DIRS)
  _gf_deploy_normalize("${_d}" _d)
  if(NOT IS_DIRECTORY "${_d}")
    _gf_deploy_fail("the runtime search directory does not exist: ${_d}")
  endif()
  list(APPEND _search_dirs "${_d}")
endforeach()

# Windows' own directories. CMake consults them by itself on a Windows host;
# naming them here as well is what lets them be excluded, and lets a test on
# another host stand in its own.
if(NOT DEFINED GF_DEPLOY_SYSTEM_DIRS)
  set(GF_DEPLOY_SYSTEM_DIRS "")
  if(DEFINED ENV{SystemRoot})
    list(APPEND GF_DEPLOY_SYSTEM_DIRS
      "$ENV{SystemRoot}/System32" "$ENV{SystemRoot}")
  endif()
endif()
set(_system_dirs "")
foreach(_d IN LISTS GF_DEPLOY_SYSTEM_DIRS)
  _gf_deploy_normalize("${_d}" _d)
  if(IS_DIRECTORY "${_d}")
    list(APPEND _system_dirs "${_d}")
  endif()
endforeach()

# Each kind of project binary, checked to exist before anything is touched.
set(_inputs_exe "")
set(_inputs_lib "")
set(_inputs_mod "")
foreach(_kind IN ITEMS EXECUTABLES LIBRARIES MODULES)
  foreach(_f IN LISTS GF_DEPLOY_${_kind})
    _gf_deploy_normalize("${_f}" _f)
    if(NOT EXISTS "${_f}" OR IS_DIRECTORY "${_f}")
      _gf_deploy_fail("a binary to scan does not exist: ${_f}")
    endif()
    if(_kind STREQUAL "EXECUTABLES")
      list(APPEND _inputs_exe "${_f}")
    elseif(_kind STREQUAL "LIBRARIES")
      list(APPEND _inputs_lib "${_f}")
    else()
      list(APPEND _inputs_mod "${_f}")
    endif()
  endforeach()
endforeach()
if(NOT _inputs_exe AND NOT _inputs_lib AND NOT _inputs_mod)
  _gf_deploy_fail("the manifest names no binaries to scan")
endif()

# Plugins windeployqt placed in subdirectories of the destination
# (platforms/, tls/, imageformats/ ...). Looked for when the script runs, not
# at configure time, because they appear after the build. modules/ is the
# old flat module directory, which nothing ships any more.
set(_inputs_plugin "")
if(GF_DEPLOY_SCAN_PLUGINS)
  file(GLOB _subdirs LIST_DIRECTORIES true "${_dest}/*")
  list(SORT _subdirs)
  foreach(_sub IN LISTS _subdirs)
    get_filename_component(_sub_name "${_sub}" NAME)
    string(TOLOWER "${_sub_name}" _sub_key)
    if(NOT IS_DIRECTORY "${_sub}" OR _sub_key STREQUAL "modules")
      continue()
    endif()
    file(GLOB _plugins LIST_DIRECTORIES false "${_sub}/*")
    list(SORT _plugins)
    foreach(_p IN LISTS _plugins)
      if(_p MATCHES "\\.[Dd][Ll][Ll]$")
        list(APPEND _inputs_plugin "${_p}")
      endif()
    endforeach()
  endforeach()
endif()

# ---- the private scan directory ----------------------------------------------

# It is deleted and recreated, so it is held to strict rules first: inside the
# build tree's deploy/ directory, overlapping nothing that matters, and, when
# it already exists, carrying the marker this script leaves in it.
set(_scan_parent "${_binary_dir}/deploy")
_gf_deploy_is_within("${_scan_dir}" "${_scan_parent}" _inside)
_gf_deploy_path_key("${_scan_dir}" _scan_key)
_gf_deploy_path_key("${_scan_parent}" _scan_parent_key)
if(NOT _inside OR _scan_key STREQUAL _scan_parent_key)
  _gf_deploy_fail("refusing to use '${_scan_dir}' as the scan directory: it "
    "must be inside '${_scan_parent}'")
endif()
foreach(_d IN LISTS _dest _search_dirs _system_dirs)
  _gf_deploy_is_within("${_scan_dir}" "${_d}" _a)
  _gf_deploy_is_within("${_d}" "${_scan_dir}" _b)
  if(_a OR _b)
    _gf_deploy_fail("refusing to use '${_scan_dir}' as the scan directory: "
      "it overlaps '${_d}'")
  endif()
endforeach()

set(_marker "${_scan_dir}/.gf-deploy-scan")
if(IS_SYMLINK "${_scan_dir}")
  _gf_deploy_fail("refusing to delete '${_scan_dir}': it is a symbolic link")
endif()
if(EXISTS "${_scan_dir}")
  if(NOT IS_DIRECTORY "${_scan_dir}" OR NOT EXISTS "${_marker}")
    _gf_deploy_fail("refusing to delete '${_scan_dir}': it was not created by "
      "this script (no .gf-deploy-scan marker). Remove it by hand if it is "
      "safe to.")
  endif()
  file(REMOVE_RECURSE "${_scan_dir}")
endif()
file(MAKE_DIRECTORY "${_scan_dir}/bin")
file(WRITE "${_marker}"
  "Scratch directory of cmake/DeployMinGWRuntime.cmake; recreated every run.\n")

# Hard links where the filesystem allows, copies where it does not. Removing
# a hard link later never touches the file it points at.
set(_scan_names "")
set(_scan_exe "")
set(_scan_lib "")
set(_scan_mod "")
function(_gf_deploy_stage src dst_dir out_var)
  get_filename_component(_n "${src}" NAME)
  file(MAKE_DIRECTORY "${dst_dir}")
  file(CREATE_LINK "${src}" "${dst_dir}/${_n}" COPY_ON_ERROR)
  set(${out_var} "${dst_dir}/${_n}" PARENT_SCOPE)
endfunction()

foreach(_f IN LISTS _inputs_exe _inputs_lib)
  get_filename_component(_n "${_f}" NAME)
  string(TOLOWER "${_n}" _k)
  if(_k IN_LIST _scan_names)
    _gf_deploy_fail("two project binaries are both named '${_n}'")
  endif()
  list(APPEND _scan_names "${_k}")
  _gf_deploy_stage("${_f}" "${_scan_dir}/bin" _staged)
  if(_f IN_LIST _inputs_exe)
    list(APPEND _scan_exe "${_staged}")
  else()
    list(APPEND _scan_lib "${_staged}")
  endif()
endforeach()
set(_project_names "${_scan_names}")

# One directory each, so that a module or plugin resolves only itself in its
# own directory, as it would beside the other files of its namespace.
set(_index 0)
foreach(_f IN LISTS _inputs_mod)
  _gf_deploy_stage("${_f}" "${_scan_dir}/modules/${_index}" _staged)
  list(APPEND _scan_mod "${_staged}")
  get_filename_component(_n "${_f}" NAME)
  string(TOLOWER "${_n}" _k)
  list(APPEND _project_names "${_k}")
  math(EXPR _index "${_index} + 1")
endforeach()
foreach(_f IN LISTS _inputs_plugin)
  _gf_deploy_stage("${_f}" "${_scan_dir}/plugins/${_index}" _staged)
  list(APPEND _scan_mod "${_staged}")
  math(EXPR _index "${_index} + 1")
endforeach()

# ---- runtime-loaded DLLs -------------------------------------------------------

# Named, not imported, so no scan can find them. Each is looked up in the
# approved directories only; it is copied itself AND scanned, because the
# resolver reports what its inputs need, never the inputs.
set(_problems "")
set(_extra_sources "")
foreach(_name IN LISTS GF_DEPLOY_EXTRA_DLLS)
  set(_hits "")
  foreach(_d IN LISTS _search_dirs)
    _gf_deploy_find_in_dir("${_d}" "${_name}" _found)
    if(NOT _found STREQUAL "")
      list(APPEND _hits "${_d}/${_found}")
    endif()
  endforeach()
  list(LENGTH _hits _hit_count)
  if(_hit_count EQUAL 0)
    list(APPEND _problems "runtime-loaded '${_name}' is in none of: ${_search_dirs}")
  elseif(_hit_count GREATER 1)
    string(REPLACE ";" "\n    " _hit_list "${_hits}")
    _gf_deploy_fail("runtime-loaded '${_name}' was found in more than one "
      "approved directory:\n    ${_hit_list}")
  else()
    list(APPEND _extra_sources "${_hits}")
  endif()
endforeach()

# ---- the scan --------------------------------------------------------------

set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM "windows+pe")
set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL "objdump")
set(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND "${GF_DEPLOY_OBJDUMP}")

# Windows' own DLLs are left to Windows, and not traversed either.
set(_post_exclude "")
foreach(_d IN LISTS _system_dirs)
  _gf_deploy_literal_regex("${_d}/" _re)
  list(APPEND _post_exclude "^${_re}")
endforeach()

set(_args "")
if(_scan_exe)
  list(APPEND _args EXECUTABLES ${_scan_exe})
endif()
if(_scan_lib OR _extra_sources)
  list(APPEND _args LIBRARIES ${_scan_lib} ${_extra_sources})
endif()
if(_scan_mod)
  list(APPEND _args MODULES ${_scan_mod})
endif()
if(_post_exclude)
  list(APPEND _args POST_EXCLUDE_REGEXES ${_post_exclude})
endif()

file(GET_RUNTIME_DEPENDENCIES
  ${_args}
  DIRECTORIES "${_scan_dir}/bin" ${_search_dirs} ${_system_dirs}
  # API sets are virtual names the loader maps itself; there is no file.
  PRE_EXCLUDE_REGEXES "^[Aa][Pp][Ii]-[Mm][Ss]-" "^[Ee][Xx][Tt]-[Mm][Ss]-"
  RESOLVED_DEPENDENCIES_VAR _resolved
  UNRESOLVED_DEPENDENCIES_VAR _unresolved
  CONFLICTING_DEPENDENCIES_PREFIX _conflict)

# The same name reached from two directories: whichever the loader picks,
# one of the two scans was describing a file that will not be loaded.
foreach(_name IN LISTS _conflict_FILENAMES)
  set(_keys "")
  foreach(_p IN LISTS _conflict_${_name})
    _gf_deploy_path_key("${_p}" _k)
    list(APPEND _keys "${_k}")
  endforeach()
  list(REMOVE_DUPLICATES _keys)
  list(LENGTH _keys _count)
  if(_count GREATER 1)
    string(REPLACE ";" "\n    " _paths "${_conflict_${_name}}")
    _gf_deploy_fail("'${_name}' resolves to more than one file:\n    ${_paths}\n"
      "Remove or rename the copy that should not be loaded.")
  endif()
endforeach()

foreach(_name IN LISTS _unresolved)
  list(APPEND _problems
    "'${_name}' is imported but was found nowhere (try: pacman -F ${_name})")
endforeach()

# ---- what to copy -----------------------------------------------------------

set(_copy_sources "")
set(_copy_keys "")
set(_unapproved "")
foreach(_path IN LISTS _resolved _extra_sources)
  _gf_deploy_normalize("${_path}" _path)
  _gf_deploy_is_within("${_path}" "${_scan_dir}" _is_project)
  if(_is_project)
    continue()
  endif()

  set(_source "")
  foreach(_d IN LISTS _search_dirs)
    _gf_deploy_is_within("${_path}" "${_d}" _in)
    if(_in)
      get_filename_component(_n "${_path}" NAME)
      _gf_deploy_find_in_dir("${_d}" "${_n}" _found)
      if(_found STREQUAL "")
        set(_found "${_n}")
      endif()
      set(_source "${_d}/${_found}")
      break()
    endif()
  endforeach()

  if(_source STREQUAL "")
    set(_is_system FALSE)
    foreach(_d IN LISTS _system_dirs)
      _gf_deploy_is_within("${_path}" "${_d}" _in)
      if(_in)
        set(_is_system TRUE)
        break()
      endif()
    endforeach()
    if(NOT _is_system)
      list(APPEND _unapproved "${_path}")
    endif()
    continue()
  endif()

  get_filename_component(_n "${_source}" NAME)
  string(TOLOWER "${_n}" _k)
  if(_k IN_LIST _copy_keys)
    continue()
  endif()
  if(_k IN_LIST _project_names)
    _gf_deploy_fail("'${_n}' is both a project binary and a third-party DLL "
      "in ${_source}; copying it would overwrite the project's own file")
  endif()
  list(APPEND _copy_keys "${_k}")
  list(APPEND _copy_sources "${_source}")
endforeach()

if(_unapproved)
  string(REPLACE ";" "\n    " _paths "${_unapproved}")
  _gf_deploy_fail("these dependencies resolved outside every approved "
    "directory (${_search_dirs}):\n    ${_paths}")
endif()

if(_problems)
  string(REPLACE ";" "\n    " _list "${_problems}")
  if(GF_DEPLOY_ALLOW_UNRESOLVED)
    message(WARNING "MinGW runtime deployment: unresolved runtime DLLs "
      "(allowed by GPGFRONTEND_DEPLOY_ALLOW_UNRESOLVED_DLLS; the result will "
      "not run on a clean machine):\n    ${_list}")
  else()
    _gf_deploy_fail("unresolved runtime DLLs:\n    ${_list}\n"
      "Install the package that provides them. For a development build only, "
      "-DGPGFRONTEND_DEPLOY_ALLOW_UNRESOLVED_DLLS=ON downgrades this to a "
      "warning.")
  endif()
endif()

# ---- copy ---------------------------------------------------------------------

# file(COPY) keeps the source timestamp and skips a destination that already
# has it. So a repeated run copies nothing, a refreshed MSYS2 package or a
# deleted copy is copied again, and a deployed file's timestamp never makes
# the build think an input changed while the command ran.
list(SORT _copy_sources)
set(_copied "")
set(_deployed "")
set(_deployed_names "")
foreach(_source IN LISTS _copy_sources)
  get_filename_component(_n "${_source}" NAME)
  set(_target "${_dest}/${_n}")
  set(_before "")
  if(EXISTS "${_target}")
    file(TIMESTAMP "${_target}" _before "%Y%m%d%H%M%S" UTC)
  endif()
  file(COPY "${_source}" DESTINATION "${_dest}" FOLLOW_SYMLINK_CHAIN)
  file(TIMESTAMP "${_target}" _after "%Y%m%d%H%M%S" UTC)
  if(NOT _before STREQUAL _after)
    list(APPEND _copied "${_n}")
  endif()
  list(APPEND _deployed "${_target}")
  list(APPEND _deployed_names "${_n}")
endforeach()

if(DEFINED GF_DEPLOY_REPORT AND NOT GF_DEPLOY_REPORT STREQUAL "")
  string(REPLACE ";" "\n" _report "${_deployed_names}")
  if(NOT _report STREQUAL "")
    string(APPEND _report "\n")
  endif()
  file(WRITE "${GF_DEPLOY_REPORT}" "${_report}")
endif()

# ---- depfile and stamp ------------------------------------------------------

# Everything the result depends on, so that the next build reruns this when
# any of it changes: the binaries scanned, every source DLL, and every copy in
# the destination, whose disappearance is itself a reason to run again.
if(DEFINED GF_DEPLOY_STAMP AND NOT GF_DEPLOY_STAMP STREQUAL "")
  if(DEFINED GF_DEPLOY_DEPFILE AND NOT GF_DEPLOY_DEPFILE STREQUAL "")
    _gf_deploy_depfile_escape("${GF_DEPLOY_STAMP}" _line)
    set(_depfile "${_line}:")
    foreach(_dep IN LISTS GF_DEPLOY_MANIFEST CMAKE_CURRENT_LIST_FILE
                          _inputs_exe _inputs_lib _inputs_mod _inputs_plugin
                          _copy_sources _deployed)
      _gf_deploy_depfile_escape("${_dep}" _line)
      string(APPEND _depfile " \\\n  ${_line}")
    endforeach()
    file(WRITE "${GF_DEPLOY_DEPFILE}" "${_depfile}\n")
  endif()
  file(TOUCH "${GF_DEPLOY_STAMP}")
endif()

# ---- summary --------------------------------------------------------------

list(LENGTH _inputs_exe _n_exe)
list(LENGTH _inputs_lib _n_lib)
list(LENGTH _inputs_mod _n_mod)
list(LENGTH _inputs_plugin _n_plugin)
list(LENGTH _deployed _n_deployed)
list(LENGTH _copied _n_copied)
list(LENGTH _extra_sources _n_extra)
math(EXPR _n_current "${_n_deployed} - ${_n_copied}")
message(STATUS "MinGW runtime: scanned ${_n_exe} executable(s), "
  "${_n_lib} librar(ies), ${_n_mod} module(s), ${_n_plugin} plugin(s); "
  "${_n_deployed} third-party DLL(s) (${_n_copied} copied, ${_n_current} "
  "up to date), ${_n_extra} of them runtime-loaded")
if(_copied)
  string(REPLACE ";" ", " _list "${_copied}")
  message(STATUS "MinGW runtime: copied ${_list}")
endif()
