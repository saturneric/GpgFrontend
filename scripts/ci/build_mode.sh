#!/bin/bash
# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
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
#
# Decide what kind of build this run is, once, for every job.
#
# Prints `key=value` lines meant for $GITHUB_OUTPUT. Reads only the runner's
# own variables plus two inputs, passed through the environment rather than
# interpolated into a script: an expression expanded inside `run:` is textual
# substitution.
#
#   BUILD_MODE_INPUT  workflow_dispatch's build_mode choice
#   SCREENSHOT_INPUT  "true" for a screenshot-mode build
#
# Trunk model: main is the next minor release, release/X.Y receives fixes
# for one series, and versions come from tags (cmake/GitVersion.cmake):
#   - a version tag (v*)    -> release build (final, beta or rc alike: they
#                              share the stable identity)
#   - a push to main        -> nightly build
#   - a push to release/X.Y -> nightly build of that series
#   - a pull request        -> PR validation build
#   - workflow_dispatch     -> honour the chosen input, so a release package
#                              can be produced without re-tagging
#
# Also prints the version, derived once here from the full history (the
# checkout must have fetch-depth: 0) so every job names its files alike.

set -euo pipefail

: "${GITHUB_EVENT_NAME:?}" "${GITHUB_REF:?}" "${GITHUB_SHA:?}"

if [[ "$GITHUB_EVENT_NAME" == "workflow_dispatch" ]]; then
  build_mode="${BUILD_MODE_INPUT:?workflow_dispatch needs a build_mode}"
  build_type="Release"
elif [[ "$GITHUB_EVENT_NAME" == "pull_request" ]]; then
  build_mode="pr"
  build_type="RelWithDebInfo"
elif [[ "$GITHUB_REF" == refs/tags/* ]]; then
  build_mode="release"
  build_type="Release"
else
  build_mode="nightly"
  build_type="Release"
fi

# Stable release builds drop the "Testing" suffix from the app name; nightly
# and PR builds are both "nightly" to CMake. The bundle identifier is fixed
# either way, so the App Store provisioning profile matches both.
if [[ "$build_mode" == "release" ]]; then
  gpgfrontend_build_mode="release"
else
  gpgfrontend_build_mode="nightly"
fi

# The screenshot mode only ever rides on a nightly-style build, and its
# artifact lives outside the gpgfrontend-*/buildinfo-* names the release
# collects. CMake refuses the screenshot mode without the tests: its own tests
# are compiled into gf_test. Nothing in CI runs them; the MSYS2 gtest DLL is
# collected by the runtime rescan like any other import, and the smoke launch
# fails if it is missing.
screenshot_cmake_flag=""
if [[ "${SCREENSHOT_INPUT:-false}" == "true" ]]; then
  if [[ "$build_mode" == "release" ]]; then
    echo "::error::the screenshot build cannot be a release build" >&2
    exit 1
  fi
  screenshot_cmake_flag="-DGPGFRONTEND_BUILD_SCREENSHOT_MODE=ON -DGPGFRONTEND_BUILD_TESTS=ON"
fi

# The version, from the one derivation the build itself uses. artifact_version
# is its file-safe spelling ("+" folded to "-"), which names every artifact.
version_field() {
  cmake -DGF_FIELD="$1" -P cmake/GitVersion.cmake
}
version="$(version_field version)"
artifact_version="$(version_field filesafe)"
[[ "$artifact_version" =~ ^[0-9A-Za-z.-]+$ ]] || {
  echo "::error::unexpected file-safe version: $artifact_version" >&2
  exit 1
}
echo "version=$version"
echo "artifact_version=$artifact_version"
echo "version_series=$(version_field series)"
echo "build_number=$(version_field build_number)"
echo "build_mode=$build_mode"
echo "build_type=$build_type"
echo "gpgfrontend_build_mode=$gpgfrontend_build_mode"
echo "screenshot_cmake_flag=$screenshot_cmake_flag"

{
  echo "Version: $version"
  echo "Build mode: $build_mode"
  echo "Build type: $build_type"
  echo "Screenshot mode: ${SCREENSHOT_INPUT:-false}"
} >&2
