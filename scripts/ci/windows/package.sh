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
# Turn build/artifacts into the Windows payload: stage it, deploy Qt and the
# MSYS2 runtime into it, prove the module descriptors still hold, and remove
# what is not a deliverable.
#
# Payload staging is flavour-independent: the portable ZIP and the MSI are
# both built from this same tree, only from a differently configured build.
#
# Relative paths throughout: the workspace is D:\a\..., and unquoted in bash
# its backslashes vanish.

set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
cd "$ROOT"

# ---------------------------------------------------------------------------
echo "::group::Stage the payload"
cp PrivacyPolicy.md README.md SECURITY.md TRANSLATORS COPYING build/artifacts/
cp gpgfrontend.ico build/artifacts/bin/
rm -rf build/artifacts/bin/*.a
# CMake writes each module straight into its own namespace at
# build/artifacts/modules/<key>/{module.gfmodule,native/}, which is exactly
# where the Host looks (<appdir>/../modules); nothing is moved into place
# after the fact.
rm -rf build/artifacts/modules/*/native/*.a
rm -rf build/artifacts/modules/*/native/*.dll.a

cd build
# The libraries CMake registered, read from the list it writes -- not a
# `libgf_*.dll` glob. The glob matched by name, so a test fixture called
# libgf_mod_test_sentinel.dll was handed to windeployqt6, which correctly
# reported that it is not a Qt executable and stopped the build. Read into a
# variable first, because a failure inside `< <(...)` is swallowed and left
# windeployqt never run on the libraries at all.
#
# The key database opens QSQLITE and nothing else. windeployqt would otherwise
# copy every SQL driver Qt has, and the Firebird, MySQL and PostgreSQL ones
# import client libraries (fbclient, libmariadb, libpq) that are not
# installed: dead files that could never load, and that the runtime
# deployment rightly refuses.
UNUSED_QT_PLUGINS="qsqlibase,qsqlmysql,qsqlpsql"
LIBS="$(../scripts/build_info.sh . deployable_libraries)"
test -n "$LIBS" || { echo "no deployable libraries registered" >&2; exit 1; }
while IFS= read -r lib; do
  [ -n "$lib" ] || continue
  test -f "./artifacts/bin/$lib" || {
    echo "registered library $lib is not in artifacts/bin" >&2
    exit 1
  }
  windeployqt6 --no-translations --force --exclude-plugins "$UNUSED_QT_PLUGINS" "./artifacts/bin/$lib"
done <<< "$LIBS"
windeployqt6 --no-translations --force --exclude-plugins "$UNUSED_QT_PLUGINS" ./artifacts/bin/GpgFrontend.exe
# A module may need Qt modules the app itself never links (Qt6Xml,
# Qt6Network, ...), so each one still has to be scanned. But --dir sends what
# it needs into bin/, which is the first directory the loader searches,
# instead of duplicating the whole Qt runtime next to every module.
while IFS= read -r module; do
  windeployqt6 --no-translations --force --exclude-plugins "$UNUSED_QT_PLUGINS" --dir ./artifacts/bin "$module"
done < <(find ./artifacts/modules -type f -name '*.dll' | sort)
# windeployqt brings Qt's plugins (platforms/, tls/, imageformats/ ...) but not
# the MSYS2 DLLs those plugins import. The build's own deployment ran before
# they existed, so scan again now, plugins included. This target always runs;
# it is the packaging rescan, not the incremental one, and a DLL it cannot
# resolve fails the job.
cmake --build . --target gf_deploy_runtime_rescan
mkdir -p upload-artifact
cd ..
echo "::endgroup::"

# ---------------------------------------------------------------------------
echo "::group::Verify module descriptors"
# windeployqt has just run. It reads the module DLLs and writes only into bin/,
# so the descriptors still describe what is there -- and this proves that
# rather than assuming it. The audit first verifies the descriptors strictly,
# then asks the packager which native each signed descriptor binds and audits
# it. --require-binding: Windows must ship bound artifacts, and an unbound tree
# verifies fine without this flag.
PACKAGER="$(./scripts/build_info.sh build module_packager)"
EXPECTED="$(./scripts/build_info.sh build module_count)"
./scripts/audit_module_natives.sh \
  --namespace-root build/artifacts/modules \
  --packager "$PACKAGER" \
  --expect-count "$EXPECTED" \
  --require-binding \
  --assert-no-native-outside build/artifacts
echo "::endgroup::"

# ---------------------------------------------------------------------------
echo "::group::Remove build-only files from the payload"
TOOL_DIR="$(./scripts/build_info.sh build host_tool_dir)"
rm -f "$TOOL_DIR/gf_module_packager.exe" "$TOOL_DIR/gf_module_keygen.exe" \
      "$TOOL_DIR/gf_module_externalize.exe"

# Stated as a check rather than assumed: this is the last chance to notice,
# and a stray build tool in a signed installer is the kind of thing found by a
# user rather than by us.
STRAY="$(find build/artifacts \( -name 'gf_module_packager*' \
           -o -name 'gf_module_keygen*' \
           -o -name 'gf_module_externalize*' \
           -o -name '*test_sentinel*' \) -print)"
if [ -n "$STRAY" ]; then
  echo "these are not deliverables and are still in the payload:" >&2
  printf '%s\n' "$STRAY" >&2
  exit 1
fi
echo "::endgroup::"
