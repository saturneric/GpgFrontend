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
# Single-branch (trunk + tags) model:
#   - a version tag (v*)  -> stable release build
#   - a push to main      -> nightly build
#   - a pull request      -> PR validation build
#   - workflow_dispatch   -> honour the chosen input, so a release package can
#                            be produced without re-tagging

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

# A fixed slice of GITHUB_SHA, not `git rev-parse --short`: git's abbreviation
# length scales with object count, so it yields 7 in a shallow checkout and 8
# in a full clone. Pinning it keeps artifact names stable across platforms.
echo "short_sha=${GITHUB_SHA:0:7}"
echo "build_mode=$build_mode"
echo "build_type=$build_type"
echo "gpgfrontend_build_mode=$gpgfrontend_build_mode"
echo "screenshot_cmake_flag=$screenshot_cmake_flag"

{
  echo "Build mode: $build_mode"
  echo "Build type: $build_type"
  echo "Screenshot mode: ${SCREENSHOT_INPUT:-false}"
} >&2
