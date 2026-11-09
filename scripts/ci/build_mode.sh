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
# for one series, and versions come from tags (cmake/GitVersion.cmake). Three
# channels, each fed by one kind of ref (scripts/release/refs.py):
#   - a release tag vX.Y.Z  -> release build                 publish: final
#   - a push to main        -> nightly build                 publish: nightly-main
#   - a push to release/X.Y -> nightly build of that series  publish: beta
#                              (the opt-in beta channel of that series)
#   - any other tag (a vX.Y.0-alpha.0 base tag, a stray prerelease tag)
#                           -> nightly build, published nowhere
#   - a pull request        -> PR validation build, published nowhere
#   - workflow_dispatch     -> honour the chosen input, so a release package
#                              can be produced without re-tagging; published
#                              nowhere
#
# `publish_kind` is the one answer to "does this run sign and stage what it
# builds": every signing, provenance, staging and upload job gates on it
# (`publishes` is it as a boolean). `ref_kind` is what the ref is, whatever
# the event (main, release-branch, release-tag or other): a workflow that
# signs on a manual run too (mas-sandbox.yml) gates on that.
#
# Also prints the version, derived once here from the full history (the
# checkout must have fetch-depth: 0) so every job names its files alike.

set -euo pipefail

: "${GITHUB_EVENT_NAME:?}" "${GITHUB_REF:?}"

# The ref shapes, spelled as scripts/release/refs.py and semver.RELEASE_TAG
# spell them (scripts/tests/test_build_mode.py holds them together): no
# leading zeros, nothing after the last number.
num='(0|[1-9][0-9]*)'
release_tag_re="^refs/tags/v${num}\.${num}\.${num}\$"
release_branch_re="^refs/heads/release/[0-9]+\.[0-9]+\$"

if [[ "$GITHUB_REF" == "refs/heads/main" ]]; then
  ref_kind="main"
elif [[ "$GITHUB_REF" =~ $release_branch_re ]]; then
  ref_kind="release-branch"
elif [[ "$GITHUB_REF" =~ $release_tag_re ]]; then
  ref_kind="release-tag"
else
  ref_kind="other"
fi

if [[ "$GITHUB_EVENT_NAME" == "workflow_dispatch" ]]; then
  build_mode="${BUILD_MODE_INPUT:?workflow_dispatch needs a build_mode}"
  [[ "$build_mode" == "nightly" || "$build_mode" == "release" ]] || {
    echo "::error::build_mode must be nightly or release, not $build_mode" >&2
    exit 1
  }
  build_type="Release"
elif [[ "$GITHUB_EVENT_NAME" == "pull_request" ]]; then
  build_mode="pr"
  build_type="RelWithDebInfo"
elif [[ "$ref_kind" == "release-tag" ]]; then
  build_mode="release"
  build_type="Release"
else
  build_mode="nightly"
  build_type="Release"
fi

publish_kind="none"
if [[ "$GITHUB_EVENT_NAME" == "push" ]]; then
  case "$ref_kind" in
    main) publish_kind="nightly-main" ;;
    release-branch) publish_kind="beta" ;;
    release-tag) publish_kind="final" ;;
  esac
fi
publishes="false"
[[ "$publish_kind" != "none" ]] && publishes="true"

# Google Play takes only a versionCode above every one it has
# (scripts/android/play_upload.py refuses anything else), and main's
# nightlies go there too. So a release is uploaded only when its build number
# is above main's current one: the .0 of the series main is building, never
# a fix of an older series (v2.3.1 while main builds 2.4.0-alpha.N), which
# would only fail there. The release's number follows from its tag (it is
# HEAD's in CI); main's is asked of git, the way the build derives it
# (GF_MAIN_BUILD_NUMBER: given instead, for the tests).
release_number() {
  [[ "$1" =~ $release_tag_re ]]
  # 10#: decimal, whatever the digits (no leading zero passes the pattern).
  echo $((1000000000 + 10#${BASH_REMATCH[1]} * 100000000 +
    10#${BASH_REMATCH[2]} * 1000000 + 10#${BASH_REMATCH[3]} * 10000 + 9000))
}
play_upload="false"
if [[ "$publish_kind" == "nightly-main" ]]; then
  play_upload="true"
elif [[ "$publish_kind" == "final" ]]; then
  main_number="${GF_MAIN_BUILD_NUMBER:-}"
  if [[ -z "$main_number" ]]; then
    main_rev="$(git rev-parse -q --verify refs/remotes/origin/main || true)"
    if [[ -n "$main_rev" ]]; then
      main_number="$(cmake -DGF_FIELD=build_number \
        -DGPGFRONTEND_VERSION_REV="$main_rev" -DGPGFRONTEND_VERSION_REF=main \
        -P cmake/GitVersion.cmake)"
    fi
  fi
  if [[ -z "$main_number" ]]; then
    echo "::warning::origin/main is not in this checkout; not uploading to Play" >&2
  elif (($(release_number "$GITHUB_REF") > main_number)); then
    play_upload="true"
  fi
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
# Each in an assignment of its own: a cmake that fails stops the script
# (set -e), rather than printing an empty value inside an echo.
version="$(version_field version)"
artifact_version="$(version_field filesafe)"
build_number="$(version_field build_number)"
[[ "$artifact_version" =~ ^[0-9A-Za-z.-]+$ ]] || {
  echo "::error::unexpected file-safe version: $artifact_version" >&2
  exit 1
}
[[ "$build_number" =~ ^[1-9][0-9]*$ ]] || {
  echo "::error::unexpected build number: $build_number" >&2
  exit 1
}
echo "version=$version"
echo "artifact_version=$artifact_version"
echo "build_number=$build_number"
echo "build_mode=$build_mode"
echo "ref_kind=$ref_kind"
echo "build_type=$build_type"
echo "gpgfrontend_build_mode=$gpgfrontend_build_mode"
echo "screenshot_cmake_flag=$screenshot_cmake_flag"
echo "publish_kind=$publish_kind"
echo "publishes=$publishes"
echo "play_upload=$play_upload"

{
  echo "Version: $version"
  echo "Build mode: $build_mode"
  echo "Publish: $publish_kind"
  echo "Build type: $build_type"
  echo "Screenshot mode: ${SCREENSHOT_INPUT:-false}"
} >&2
