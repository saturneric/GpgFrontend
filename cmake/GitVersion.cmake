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
# SPDX-License-Identifier: GPL-3.0-or-later

# The one place a GpgFrontend version is derived. Git tags are the only source
# of truth: CMake includes this file before project(), and CI, the Android
# package and the release tooling run it as a script, so nothing else ever
# decides what version a build is.
#
#   include(cmake/GitVersion.cmake)
#   gf_git_version("${CMAKE_SOURCE_DIR}")
#
#   cmake -P cmake/GitVersion.cmake                  # JSON on stdout
#   cmake -DGF_FIELD=version -P cmake/GitVersion.cmake  # one bare field
#
# Rules (SemVer 2.0.0):
#   on a tag vX.Y.Z[-beta.N|-rc.N]       -> the tag without "v"
#   N commits after vX.Y.Z-alpha.0       -> X.Y.Z-alpha.N+g<sha12>
#   N commits after vX.Y.Z-rc.K          -> X.Y.Z-rc.K.dev.N+g<sha12>
#   N commits after vX.Y.Z, release/X.Y  -> X.Y.(Z+1)-alpha.N+g<sha12>
#   N commits after vX.Y.Z, elsewhere    -> X.(Y+1).0-alpha.N+g<sha12>
#
# Untagged builds are "alpha", not "dev": SemVer compares prerelease
# identifiers in ASCII order, and "dev" sorts above "beta", so a nightly would
# outrank the beta tagged after it. vX.Y.0-alpha.0 is the base tag
# scripts/release/cut.sh puts on main when release/X.(Y-1) is cut: from then
# on main counts from it, whether or not the release it follows is reachable
# from main. It marks a starting point and is never a release itself.
#
# Where git cannot answer (a source archive), the VERSION file at the top of
# the tree is read: git archive expands it through export-subst. A packager
# that has neither passes -DGPGFRONTEND_VERSION_OVERRIDE=<semver>. A CI run
# (CI set in the environment) must get its answer from git: a shallow clone
# there is an error, not a silent 0.0.0.

# SemVer's own grammar, minus build metadata, which is ours to write.
set(_GF_SEMVER_CORE_RE "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
set(_GF_SEMVER_PRE_RE "^[0-9A-Za-z-]+(\\.[0-9A-Za-z-]+)*$")

# Which branch the tree is on, for the one rule that needs it: a release
# branch bumps the patch, everything else the minor. A detached CI checkout
# has no symbolic ref, so the runner's own variable is asked first.
function(_gf_version_ref source_dir out)
  if(DEFINED GPGFRONTEND_VERSION_REF AND NOT GPGFRONTEND_VERSION_REF STREQUAL "")
    set(${out} "${GPGFRONTEND_VERSION_REF}" PARENT_SCOPE)
    return()
  endif()

  if(DEFINED ENV{GITHUB_REF} AND "$ENV{GITHUB_REF}" MATCHES "^refs/heads/(.+)$")
    set(${out} "${CMAKE_MATCH_1}" PARENT_SCOPE)
    return()
  endif()

  set(_ref "")
  if(GIT_EXECUTABLE)
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" symbolic-ref --short -q HEAD
      WORKING_DIRECTORY "${source_dir}"
      OUTPUT_VARIABLE _ref
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET)
  endif()
  set(${out} "${_ref}" PARENT_SCOPE)
endfunction()

# The position of a version among all others, as one integer: the Android
# versionCode and the macOS CFBundleVersion. Both stores only accept a build
# that is greater than the last, so this must order as SemVer does.
#
#   1 000 000 000 + major * 10^8 + minor * 10^6 + patch * 10^4 + stage
#
# stage: alpha.N          -> N (at most 5999)
#        beta.K[.dev.M]   -> 6000 + K*100 + M (K at most 9, M at most 99)
#        rc.K[.dev.M]     -> 7000 + K*100 + M
#        final            -> 9000
#
# The leading 10^9 keeps it above the configure-time timestamps
# (%y%j%H%M, about 2.6 * 10^8) earlier builds were uploaded with. The ceiling
# of 2 100 000 000 is reached at major 11.
function(_gf_version_build_number major minor patch pre out)
  set(_stage 9000)
  if(pre MATCHES "^alpha\\.([0-9]+)$")
    set(_stage ${CMAKE_MATCH_1})
    if(_stage GREATER 5999)
      set(_stage 5999)
    endif()
  elseif(pre MATCHES "^(beta|rc)\\.([0-9]+)(\\.dev\\.([0-9]+))?$")
    set(_kind "${CMAKE_MATCH_1}")
    set(_k "${CMAKE_MATCH_2}")
    set(_m 0)
    if(NOT CMAKE_MATCH_4 STREQUAL "")
      set(_m "${CMAKE_MATCH_4}")
    endif()
    if(_k GREATER 9)
      set(_k 9)
    endif()
    if(_m GREATER 99)
      set(_m 99)
    endif()
    if(_kind STREQUAL "beta")
      math(EXPR _stage "6000 + ${_k} * 100 + ${_m}")
    else()
      math(EXPR _stage "7000 + ${_k} * 100 + ${_m}")
    endif()
  elseif(NOT pre STREQUAL "")
    message(WARNING
      "GitVersion: prerelease \"${pre}\" is neither alpha.N, beta.N nor rc.N; "
      "its build number sorts with the alpha builds")
    set(_stage 0)
  endif()

  if(major GREATER 10 OR minor GREATER 99 OR patch GREATER 99)
    message(FATAL_ERROR
      "GitVersion: ${major}.${minor}.${patch} does not fit the build number "
      "scheme (major <= 10, minor and patch <= 99)")
  endif()

  math(EXPR _code
    "1000000000 + ${major} * 100000000 + ${minor} * 1000000 + ${patch} * 10000 + ${_stage}")
  set(${out} "${_code}" PARENT_SCOPE)
endfunction()

# Turns one `git describe` answer into the version. Accepts both the --long
# form (v2.2.4-0-gabc) and the short one git archive writes on a tag (v2.3.0).
function(_gf_version_from_describe describe ref out_version out_ok)
  set(${out_ok} FALSE PARENT_SCOPE)

  set(_tag "${describe}")
  set(_distance 0)
  set(_sha "")
  set(_dirty FALSE)

  if(_tag MATCHES "^(.*)-dirty$")
    set(_tag "${CMAKE_MATCH_1}")
    set(_dirty TRUE)
  endif()
  if(_tag MATCHES "^(.*)-([0-9]+)-g([0-9a-f]+)$")
    set(_tag "${CMAKE_MATCH_1}")
    set(_distance "${CMAKE_MATCH_2}")
    set(_sha "${CMAKE_MATCH_3}")
  endif()

  if(NOT _tag MATCHES "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)(-(.+))?$")
    return()
  endif()
  set(_major "${CMAKE_MATCH_1}")
  set(_minor "${CMAKE_MATCH_2}")
  set(_patch "${CMAKE_MATCH_3}")
  set(_pre "${CMAKE_MATCH_5}")
  if(NOT _pre STREQUAL "" AND NOT _pre MATCHES "${_GF_SEMVER_PRE_RE}")
    return()
  endif()

  if(_distance GREATER 0)
    if(_pre STREQUAL "alpha.0")
      set(_pre "alpha.${_distance}")
    elseif(NOT _pre STREQUAL "")
      set(_pre "${_pre}.dev.${_distance}")
    elseif(ref MATCHES "^release/[0-9]+\\.[0-9]+$")
      math(EXPR _patch "${_patch} + 1")
      set(_pre "alpha.${_distance}")
    else()
      math(EXPR _minor "${_minor} + 1")
      set(_patch 0)
      set(_pre "alpha.${_distance}")
    endif()
  endif()

  set(_build "")
  if(_distance GREATER 0 AND NOT _sha STREQUAL "")
    set(_build "g${_sha}")
  endif()
  if(_dirty)
    if(_build STREQUAL "")
      set(_build "dirty")
    else()
      set(_build "${_build}.dirty")
    endif()
  endif()

  set(_version "${_major}.${_minor}.${_patch}")
  if(NOT _pre STREQUAL "")
    string(APPEND _version "-${_pre}")
  endif()
  if(NOT _build STREQUAL "")
    string(APPEND _version "+${_build}")
  endif()

  set(${out_version} "${_version}" PARENT_SCOPE)
  set(${out_ok} TRUE PARENT_SCOPE)
endfunction()

# Splits a full SemVer and sets every GF_VERSION_* output in the caller's
# caller (gf_git_version's parent scope).
macro(_gf_version_publish version commit source tag)
  if(NOT "${version}" MATCHES
      "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(-([^+]+))?(\\+(.+))?$")
    message(FATAL_ERROR "GitVersion: \"${version}\" is not a SemVer version")
  endif()
  set(_gfv_major "${CMAKE_MATCH_1}")
  set(_gfv_minor "${CMAKE_MATCH_2}")
  set(_gfv_patch "${CMAKE_MATCH_3}")
  set(_gfv_pre "${CMAKE_MATCH_5}")
  set(_gfv_build "${CMAKE_MATCH_7}")
  if(NOT _gfv_pre STREQUAL "" AND NOT _gfv_pre MATCHES "${_GF_SEMVER_PRE_RE}")
    message(FATAL_ERROR
      "GitVersion: prerelease \"${_gfv_pre}\" of \"${version}\" is not SemVer")
  endif()

  _gf_version_build_number(${_gfv_major} ${_gfv_minor} ${_gfv_patch}
    "${_gfv_pre}" _gfv_number)

  # "+" is legal SemVer but not in an object key, a URL path segment without
  # escaping, or some package names. Build metadata never decides precedence,
  # so folding it into the prerelease with "-" loses nothing a reader needs.
  string(REPLACE "+" "-" _gfv_filesafe "${version}")

  set(GF_VERSION "${version}" PARENT_SCOPE)
  set(GF_VERSION_CORE "${_gfv_major}.${_gfv_minor}.${_gfv_patch}" PARENT_SCOPE)
  set(GF_VERSION_MAJOR "${_gfv_major}" PARENT_SCOPE)
  set(GF_VERSION_MINOR "${_gfv_minor}" PARENT_SCOPE)
  set(GF_VERSION_PATCH "${_gfv_patch}" PARENT_SCOPE)
  set(GF_VERSION_PRERELEASE "${_gfv_pre}" PARENT_SCOPE)
  set(GF_VERSION_BUILD_METADATA "${_gfv_build}" PARENT_SCOPE)
  set(GF_VERSION_SERIES "${_gfv_major}.${_gfv_minor}" PARENT_SCOPE)
  set(GF_VERSION_FILESAFE "${_gfv_filesafe}" PARENT_SCOPE)
  set(GF_VERSION_BUILD_NUMBER "${_gfv_number}" PARENT_SCOPE)
  set(GF_VERSION_COMMIT "${commit}" PARENT_SCOPE)
  set(GF_VERSION_SOURCE "${source}" PARENT_SCOPE)
  set(GF_VERSION_TAG "${tag}" PARENT_SCOPE)
endmacro()

function(gf_git_version source_dir)
  if(NOT GIT_EXECUTABLE)
    find_program(GIT_EXECUTABLE git)
  endif()

  _gf_version_ref("${source_dir}" _ref)
  set(GF_VERSION_REF "${_ref}" PARENT_SCOPE)

  set(_strict FALSE)
  if(DEFINED ENV{CI} AND NOT "$ENV{CI}" STREQUAL ""
     AND NOT "$ENV{CI}" STREQUAL "false")
    set(_strict TRUE)
  endif()

  # A packager's word beats everything: they know what they are shipping.
  if(DEFINED GPGFRONTEND_VERSION_OVERRIDE
     AND NOT GPGFRONTEND_VERSION_OVERRIDE STREQUAL "")
    _gf_version_publish("${GPGFRONTEND_VERSION_OVERRIDE}" "" "override" "")
    return()
  endif()

  set(_describe "")
  set(_commit "")
  set(_tag "")
  if(GIT_EXECUTABLE AND EXISTS "${source_dir}/.git")
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" describe --tags --long --abbrev=12
              --dirty=-dirty "--match=v[0-9]*.[0-9]*.[0-9]*"
      WORKING_DIRECTORY "${source_dir}"
      OUTPUT_VARIABLE _describe
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE _describe_rc
      ERROR_QUIET)
    if(NOT _describe_rc EQUAL 0)
      set(_describe "")
    endif()
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
      WORKING_DIRECTORY "${source_dir}"
      OUTPUT_VARIABLE _commit
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET)
  endif()

  if(NOT _describe STREQUAL "")
    _gf_version_from_describe("${_describe}" "${_ref}" _version _ok)
    if(NOT _ok)
      message(FATAL_ERROR
        "GitVersion: the nearest tag in \"${_describe}\" is not vX.Y.Z[-pre]")
    endif()
    if(_describe MATCHES "^(v[^ ]+)-0-g[0-9a-f]+(-dirty)?$")
      set(_tag "${CMAKE_MATCH_1}")
    endif()
    _gf_version_publish("${_version}" "${_commit}" "git" "${_tag}")
    return()
  endif()

  if(_strict)
    message(FATAL_ERROR
      "GitVersion: `git describe` found no release tag. A CI checkout needs "
      "the full history and tags (actions/checkout: fetch-depth: 0).")
  endif()

  # A source archive: line 1 is the describe, line 2 the commit, both expanded
  # by git archive. An unexpanded file still reads "$Format:...".
  set(_file "${source_dir}/VERSION")
  if(EXISTS "${_file}")
    file(STRINGS "${_file}" _lines LIMIT_COUNT 2)
    list(LENGTH _lines _n)
    if(_n GREATER_EQUAL 1)
      list(GET _lines 0 _file_describe)
      string(STRIP "${_file_describe}" _file_describe)
      set(_file_commit "")
      if(_n GREATER_EQUAL 2)
        list(GET _lines 1 _file_commit)
        string(STRIP "${_file_commit}" _file_commit)
      endif()
      if(NOT _file_describe MATCHES "\\$Format")
        if(_file_commit MATCHES "\\$Format")
          set(_file_commit "")
        endif()
        _gf_version_from_describe("${_file_describe}" "${_ref}" _version _ok)
        if(_ok)
          if(_file_describe MATCHES "^v[0-9]+\\.[0-9]+\\.[0-9]+(-[^-]+)?$")
            set(_tag "${_file_describe}")
          endif()
          _gf_version_publish("${_version}" "${_file_commit}" "archive" "${_tag}")
          return()
        endif()
        message(WARNING
          "GitVersion: VERSION holds \"${_file_describe}\", which is not a "
          "release describe")
      endif()
    endif()
  endif()

  message(WARNING
    "GitVersion: no git history and no expanded VERSION file; this build is "
    "0.0.0-unknown. Pass -DGPGFRONTEND_VERSION_OVERRIDE=<version> to name it.")
  _gf_version_publish("0.0.0-unknown" "${_commit}" "unknown" "")
endfunction()

# Script mode: print what a build of this tree would call itself.
if(CMAKE_SCRIPT_MODE_FILE AND CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
  if(NOT DEFINED GF_SOURCE_DIR)
    get_filename_component(GF_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
  endif()
  gf_git_version("${GF_SOURCE_DIR}")

  set(_gf_fields
    version GF_VERSION
    core GF_VERSION_CORE
    major GF_VERSION_MAJOR
    minor GF_VERSION_MINOR
    patch GF_VERSION_PATCH
    prerelease GF_VERSION_PRERELEASE
    build_metadata GF_VERSION_BUILD_METADATA
    series GF_VERSION_SERIES
    filesafe GF_VERSION_FILESAFE
    build_number GF_VERSION_BUILD_NUMBER
    commit GF_VERSION_COMMIT
    tag GF_VERSION_TAG
    ref GF_VERSION_REF
    source GF_VERSION_SOURCE)

  if(DEFINED GF_FIELD)
    list(FIND _gf_fields "${GF_FIELD}" _gf_at)
    if(_gf_at LESS 0)
      message(FATAL_ERROR "GitVersion: unknown field \"${GF_FIELD}\"")
    endif()
    math(EXPR _gf_at "${_gf_at} + 1")
    list(GET _gf_fields ${_gf_at} _gf_var)
    execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "${${_gf_var}}")
    return()
  endif()

  set(_gf_json "{}")
  list(LENGTH _gf_fields _gf_len)
  math(EXPR _gf_last "${_gf_len} - 1")
  foreach(_gf_i RANGE 0 ${_gf_last} 2)
    math(EXPR _gf_j "${_gf_i} + 1")
    list(GET _gf_fields ${_gf_i} _gf_key)
    list(GET _gf_fields ${_gf_j} _gf_var)
    if(_gf_key STREQUAL "major" OR _gf_key STREQUAL "minor"
       OR _gf_key STREQUAL "patch" OR _gf_key STREQUAL "build_number")
      string(JSON _gf_json SET "${_gf_json}" "${_gf_key}" "${${_gf_var}}")
    else()
      string(JSON _gf_json SET "${_gf_json}" "${_gf_key}" "\"${${_gf_var}}\"")
    endif()
  endforeach()
  execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "${_gf_json}")
endif()
