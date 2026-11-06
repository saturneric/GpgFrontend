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
# Put the modules and Qt into the unsigned build/package/GpgFrontend.app and
# prove the result loads. Shared by the Developer ID and App Store legs, which
# assemble the same bundle from the same tree.
#
#   assemble_bundle.sh [--app-store]
#
# --app-store asks macdeployqt to drop the plugins that use API the App Store
# rejects.
#
# Environment: BUILD_TYPE.

set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
: "${BUILD_TYPE:?}"

APP_STORE=0
while [ $# -gt 0 ]; do
  case "$1" in
    --app-store) APP_STORE=1; shift ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done

BUILD="$ROOT/build"
APP="$BUILD/package/GpgFrontend.app"
STAGING="$BUILD/staging"
MODULE_ROOT="$STAGING/lib/gpgfrontend/modules"
DESC_ROOT="$APP/Contents/Resources/modules"
NATIVE_ROOT="$APP/Contents/Frameworks/GpgFrontendModules"

# How many modules this build produced, from the list CMake writes at configure
# time. Never a literal: a hardcoded count means adding a module breaks every
# gate at once, and the obvious repair is to bump the number in each, which is
# how a gate stops checking.
EXPECTED="$("$ROOT/scripts/build_info.sh" "$BUILD" module_count)"
PACKAGER="$("$ROOT/scripts/build_info.sh" "$BUILD" module_packager)"

# ---------------------------------------------------------------------------
echo "::group::Install the module tree"
# The modules come from an install tree, like every other platform.
#
# `xcodebuild archive` leaves each module native as a RELATIVE symlink into
# DerivedData, and install(TARGETS) copies a symlink as a symlink -- two
# directories deeper, where it resolves to nothing, and verify-module-set
# rightly refuses it. Replace each link with the file it names first; the
# install then verifies what it wrote, and a symlinked entry native that slips
# through is refused by name with where it points.
rm -rf "$STAGING"
while IFS= read -r link; do
  cp -L "$link" "$link.real"
  mv -f "$link.real" "$link"
  echo "materialized $link"
done < <(find "$BUILD/artifacts/modules" -type l)

# --config: under Xcode an install without it picks "Release", whatever was
# built. --component runtime: the one Xcode-only rule outside it installs the
# app bundle to /Applications, and the app is taken from the archive instead;
# the module-set verification is in runtime and still runs.
cmake --install "$BUILD" --config "$BUILD_TYPE" --component runtime \
  --prefix "$STAGING"

# The same namespace layout a distribution gets. The bundle's two roots are
# mapped from it below, which is the one macOS-specific step Apple's layout
# genuinely requires.
test -d "$MODULE_ROOT"
echo "::endgroup::"

# ---------------------------------------------------------------------------
echo "::group::Bundle module dependencies"
"$ROOT/scripts/bundle_macos_module_deps.sh" --namespace-root "$MODULE_ROOT"
echo "::endgroup::"

# ---------------------------------------------------------------------------
echo "::group::Place modules in the bundle"
# A namespace is split across two directories here and nowhere else, because
# Apple wants data under Resources and executable code under Frameworks:
#
#   Contents/Resources/modules/<key>/module.gfmodule
#   Contents/Frameworks/GpgFrontendModules/<key>/lib*.dylib
#
# ModuleNativeRootFor() is the single place that knows those two belong
# together.
#
# No install_name_tool: the rpaths a module needs are set at link time (see
# ModuleRegistry.cmake), so this is a copy and nothing more. Nothing re-signs
# either -- the signing job signs every loose Mach-O under Contents, these
# included, with the app's own identity, which is exactly what Library
# Validation wants and what the App Store requires.
test -d "$APP"
rm -rf "$DESC_ROOT" "$NATIVE_ROOT"
mkdir -p "$DESC_ROOT" "$NATIVE_ROOT"

placed=0
for ns in "$MODULE_ROOT"/*/; do
  key="$(basename "$ns")"
  [ -f "$ns/module.gfmodule" ] || continue

  mkdir -p "$DESC_ROOT/$key" "$NATIVE_ROOT/$key"
  cp "$ns/module.gfmodule" "$DESC_ROOT/$key/"
  cp "$ns"native/*.dylib "$NATIVE_ROOT/$key/"

  echo "  placed $key ($(find "$NATIVE_ROOT/$key" -type f | wc -l | tr -d ' ') native file(s))"
  placed=$((placed + 1))
done

# The gate that would have caught a leg shipping nothing.
test "$placed" -eq "$EXPECTED" || {
  echo "placed $placed namespaces, expected $EXPECTED" >&2
  exit 1
}

# Every module-shaped dylib in the bundle must be inside a namespace. Done in
# shell because on macOS the two physical roots differ and
# --assert-no-native-outside takes only one of them.
STRAY="$(find "$APP/Contents" -name 'libgf_mod_*' ! -path "$NATIVE_ROOT/*" -print)"
if [ -n "$STRAY" ]; then
  echo "module libraries outside any namespace in the bundle:" >&2
  printf '%s\n' "$STRAY" >&2
  exit 1
fi

echo "--- what ships ---"
find "$DESC_ROOT" "$NATIVE_ROOT" -type f | sed "s#^$APP/##" | sort
echo "::endgroup::"

# ---------------------------------------------------------------------------
echo "::group::Deploy Qt"
# Every module native, named individually.
#
# macdeployqt deploys the Qt frameworks the APP links, and a module may need
# frameworks the app never does -- QtConcurrent and QtXml, here. Without this,
# those modules are placed in the bundle, pass every descriptor check, and then
# fail to dlopen on a user's Mac with "Library not loaded:
# @rpath/QtConcurrent.framework". Which is exactly what happened.
#
# The same thing the other two platforms do: `-executable=` per module for
# linuxdeployqt, `--dir` per module for windeployqt. The modules have to be
# INSIDE the bundle first -- macdeployqt computes bundle-relative paths --
# which is why placement comes first.
EXECUTABLES=()
while IFS= read -r native; do
  EXECUTABLES+=("-executable=$native")
done < <(find "$NATIVE_ROOT" -type f -name '*.dylib' | sort)
test "${#EXECUTABLES[@]}" -gt 0
echo "deploying for ${#EXECUTABLES[@]} module native(s)"

DEPLOY_ARGS=(-verbose=2 -always-overwrite)
if [ "$APP_STORE" -eq 1 ]; then
  DEPLOY_ARGS+=(-appstore-compliant)
fi
# No -codesign=: macdeployqt only ever signed what it copied itself. The
# signing job signs every code object explicitly, inside out.
macdeployqt "$APP" "${DEPLOY_ARGS[@]}" "${EXECUTABLES[@]}"
echo "::endgroup::"

# ---------------------------------------------------------------------------
echo "::group::Verify the bundle's modules"
# Every module dependency must actually resolve inside the bundle, the
# descriptors must still verify after deployment rewrote load commands, the
# natives must pass the deployment audit, and every entry must still carry its
# binding section -- checked on the copy that ships, not the staging copy.
#
# This is the check whose absence let a signed, notarized dmg ship with three
# of its four modules unable to load. Everything else passed: descriptors
# verified, natives resolved across the Resources/Frameworks split, binding
# sections intact, codesign happy. The modules referenced Qt frameworks the
# app does not link, so macdeployqt had never deployed them, and dyld refused
# them at load.
"$ROOT/scripts/verify_macos_bundle_modules.sh" \
  --app "$APP" \
  --namespace-root "$MODULE_ROOT" \
  --packager "$PACKAGER" \
  --expect-count "$EXPECTED"
echo "::endgroup::"
