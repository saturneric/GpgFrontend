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
# Turn a finished Linux build tree into the AppImage, in four phases that are
# one operation on one tree:
#
#   1. assemble  the AppDir is an ordinary `cmake --install` tree;
#   2. deploy    linuxdeployqt, plus the repair it makes necessary;
#   3. reseal    re-describe the rewritten natives, then audit them;
#   4. image     appimagetool.
#
# Environment: ARTIFACT_VERSION, FLAVOR_SUFFIX, RUNNER_TEMP.
# Writes build/upload-artifact/GpgFrontend-<version>-linux-<arch>[-portable].AppImage,
# <version> being the file-safe one scripts/ci/build_mode.sh derives.

set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
: "${ARTIFACT_VERSION:?}" "${FLAVOR_SUFFIX?}" "${RUNNER_TEMP:?}"

BUILD="$ROOT/build"
APP_DIR="$BUILD/artifacts/AppDir"
MODULE_ROOT="$APP_DIR/usr/lib/gpgfrontend/modules"
OUT_DIR="$BUILD/upload-artifact"
TOOLS="$RUNNER_TEMP/appimage-tools"
ARCH="$(uname -m)"
mkdir -p "$OUT_DIR" "$TOOLS"

# ---------------------------------------------------------------------------
echo "::group::Assemble the AppDir"
# It used to be produced by redirecting every output directory at configure
# time, which made the CI layout one no developer or packager could reproduce
# and put the host build tools inside the image. Now the job installs exactly
# as a distribution would, and gets the modules, the desktop entry, the
# metainfo, the MIME package and the icons because install() already ships
# all of them.
#
# `cmake --install` verifies the module tree it writes, so this also proves the
# descriptors are good BEFORE deployment starts rewriting the natives they
# bind.
rm -rf "$APP_DIR"
cmake --install "$BUILD" --prefix "$APP_DIR/usr"
# The icon and .DirIcon AppImage wants at the root of the image, and which a
# normal install correctly does not place.
cmake --install "$BUILD" --component appimage --prefix "$APP_DIR"
echo "::endgroup::"

# ---------------------------------------------------------------------------
echo "::group::Deploy Qt dependencies"
# linuxdeployqt REPLACES a file's RUNPATH with its own hop to usr/lib. That is
# right for reaching Qt and wrong for reaching the private helpers beside a
# module entry, which need $ORIGIN itself, so the repair follows directly.

QT_PLUGIN_DIR="$(qmake -query QT_INSTALL_PLUGINS)"
echo "Found Qt plugin dir: $QT_PLUGIN_DIR"
# Remove all non-sqlite drivers to reduce the size of the final AppImage.
find "$QT_PLUGIN_DIR/sqldrivers" -type f ! -name '*sqlite*' -delete
ls -l "$QT_PLUGIN_DIR/sqldrivers"

EXTRA_ARGS=()
case "$ARCH" in
  x86_64) ;;
  aarch64)
    mkdir -p "$APP_DIR/usr/share/doc/libc6/"
    touch "$APP_DIR/usr/share/doc/libc6/copyright"
    EXTRA_ARGS=(-unsupported-allow-new-glibc)
    ;;
  *) echo "unsupported architecture $ARCH" >&2; exit 1 ;;
esac
LINUXDEPLOYQT="$TOOLS/linuxdeployqt.AppImage"
wget -nv -O "$LINUXDEPLOYQT" \
  "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-$ARCH.AppImage"
chmod u+x "$LINUXDEPLOYQT"

# The AppImage bundles libglib/libgobject/libgio and its AppRun puts them ahead
# of the host's copies, so a host libsecret built against a newer glib cannot
# resolve its own symbols and the system keychain simply disappears -- see
# linuxdeployqt issue 544. Carrying our own copy is what makes the dependency
# closure self-consistent. It has to be staged before linuxdeployqt runs: a
# file dropped in afterwards gets neither an rpath nor its own dependencies
# deployed.
LIBSECRET_SRC="/usr/lib/$(dpkg-architecture -qDEB_HOST_MULTIARCH)/libsecret-1.so.0"
test -f "$LIBSECRET_SRC"
cp -L "$LIBSECRET_SRC" "$APP_DIR/usr/lib/libsecret-1.so.0"

# Anything the app needs out of /usr/local, staged the same way and for the
# same reason.
#
# This was googletest, back when every build linked gf_test: CI built
# googletest as a SHARED library into /usr/local/lib, which is a directory
# that exists on a runner and on nobody's desktop. The AppImage started,
# looked for libgtest.so.1.15.2, and died before main(). Resolved rather than
# named: /usr/local is where a hand-built dependency lands, and the next one
# should not need this comment rewritten.
APP_BIN="$(find "$APP_DIR/usr/bin" -maxdepth 1 -type f -perm -u+x | head -1)"
test -n "$APP_BIN" || { echo "no application binary in the AppDir" >&2; exit 1; }

staged=0
while IFS= read -r lib; do
  [ -n "$lib" ] || continue
  cp -L "$lib" "$APP_DIR/usr/lib/$(basename "$lib")"
  echo "staged $(basename "$lib") from /usr/local"
  staged=$((staged + 1))
done < <(LD_LIBRARY_PATH="$APP_DIR/usr/lib:/usr/local/lib:/usr/local/lib64" \
           ldd "$APP_BIN" \
           | awk '$3 ~ /^\/usr\/local\// { print $3 }' | sort -u)
echo "$staged library/libraries staged from /usr/local"

# Every module native, named individually. `-executable-dir=` took a single
# flat directory, which the namespace layout no longer has: each module owns
# usr/lib/gpgfrontend/modules/<key>/native/.
test -d "$MODULE_ROOT" || {
  echo "no module tree at $MODULE_ROOT; installed module trees:" >&2
  find "$APP_DIR" -type d -path '*/gpgfrontend/modules' >&2 || true
  exit 1
}
EXECUTABLES=()
while read -r native; do
  EXECUTABLES+=("-executable=$native")
done < <(find "$MODULE_ROOT" -type f -name '*.so' | sort)
test "${#EXECUTABLES[@]}" -gt 0 \
  || { echo "no module natives under $MODULE_ROOT" >&2; exit 1; }
echo "deploying ${#EXECUTABLES[@]} module native(s)"

export LD_LIBRARY_PATH="/usr/local/lib:/usr/local/lib64:${LD_LIBRARY_PATH:-}"
# Deliberately no -appimage: the image is built at the end, after the
# descriptors have been regenerated against the deployed natives.
#
# -appimage also switched on bundling of every non-Qt library, as a side
# effect. Dropping it alone shipped an image carrying Qt and nothing else -- no
# libsodium, libarchive, glib, xcb -- that died before main() on any desktop
# without them. -bundle-non-qt-libs is that side effect on its own, and has to
# stay while -appimage is absent.
"$LINUXDEPLOYQT" \
  "$APP_DIR/usr/share/applications"/*.desktop \
  "${EXTRA_ARGS[@]}" \
  -bundle-non-qt-libs \
  -no-translations \
  -extra-plugins=iconengines,platforms,sqldrivers/libqsqlite.so \
  "-executable=$APP_DIR/usr/lib/libsecret-1.so.0" \
  "${EXECUTABLES[@]}"

# Without the rpath patch the staged copy cannot find its own dependencies,
# which is the bug the libsecret staging exists to fix, so it fails the build
# rather than shipping a silent regression.
echo "--- deployed credential-store closure ---"
# shellcheck disable=SC2010 # a diagnostic listing, names are known
ls -l "$APP_DIR/usr/lib" | grep -E 'secret|glib|gobject|gio|gcrypt' || true
readelf -d "$APP_DIR/usr/lib/libsecret-1.so.0" | grep -E 'RUNPATH|RPATH|SONAME|NEEDED' || true
readelf -d "$APP_DIR/usr/lib/libsecret-1.so.0" | grep -q 'ORIGIN' \
  || { echo "libsecret was not rpath-patched by linuxdeployqt" >&2; exit 1; }

# Put $ORIGIN back on every module native. Unconditional and idempotent rather
# than conditional on what linuxdeployqt did: a repair that only runs when
# someone predicted the need is a repair that stops running, and the audit
# below is the gate either way.
sudo apt-get install -y --no-install-recommends patchelf
while read -r native; do
  current="$(patchelf --print-rpath "$native" || true)"
  # shellcheck disable=SC2016 # $ORIGIN is for the loader, not the shell
  case ":$current:" in
    *':$ORIGIN:'*) want="$current" ;;
    '::') want='$ORIGIN:$ORIGIN/../../../..' ;;
    *) want="\$ORIGIN:$current" ;;
  esac
  patchelf --set-rpath "$want" "$native"
  echo "$(basename "$native"): $want"
done < <(find "$MODULE_ROOT" -type f -name '*.so' | sort)

# Every library anything in the AppDir needs must be in the AppDir, or be one
# the AppImage deliberately takes from the host (the excludelist linuxdeployqt
# itself honours). The smoke test cannot see this: the runner has all of them
# installed, so a missing one resolves against the host there and nowhere
# else.
wget -nv -O "$TOOLS/excludelist" \
  https://raw.githubusercontent.com/AppImage/pkg2appimage/master/excludelist
sed 's/#.*//' "$TOOLS/excludelist" | awk 'NF { print $1 }' \
  | sort -u > "$TOOLS/excluded.txt"
test -s "$TOOLS/excluded.txt" \
  || { echo "the AppImage excludelist came back empty" >&2; exit 1; }
find "$APP_DIR" -name '*.so*' -printf '%f\n' | sort -u > "$TOOLS/bundled.txt"
while IFS= read -r elf; do
  readelf -h "$elf" >/dev/null 2>&1 || continue
  readelf -d "$elf" | sed -n 's/.*(NEEDED).*\[\(.*\)\]/\1/p'
done < <(find "$APP_DIR" -type f \( -name '*.so*' -o -perm -u+x \)) \
  | sort -u > "$TOOLS/needed.txt"
# The dynamic loader is always the host's and can never be bundled. The
# excludelist only names the x86 ones, so aarch64's ld-linux-aarch64.so.1 is
# matched by pattern rather than listed.
unbundled="$(comm -23 "$TOOLS/needed.txt" "$TOOLS/bundled.txt" \
               | comm -23 - "$TOOLS/excluded.txt" \
               | grep -v -E '^ld-linux' || true)"
if [ -n "$unbundled" ]; then
  echo "needed by the AppDir but neither bundled nor excluded:" >&2
  echo "$unbundled" >&2
  exit 1
fi
echo "dependency closure complete: $(wc -l < "$TOOLS/needed.txt") NEEDED entries"
echo "::endgroup::"

# ---------------------------------------------------------------------------
echo "::group::Refresh and verify module descriptors"
# The natives were just rewritten, so the descriptors that bind them are stale
# by construction. This re-describes them and then proves the result.
#
# NOT by deleting the descriptors and re-running the build, which is the
# obvious thing and is wrong here: the natives are build outputs, so a
# `cmake --build` at this point sees that linuxdeployqt and patchelf changed
# them underneath it and RELINKS them, discarding the rpath work above.
# `reseal` does not involve the build graph at all.
#
# The audit has to come last: it asks the packager which native each module's
# SIGNED descriptor binds, so it needs descriptors that verify. It proves
# something the smoke test structurally cannot: that no dependency resolves
# through a path that exists on this runner and not on a user's machine.
PACKAGER="$("$ROOT/scripts/build_info.sh" "$BUILD" module_packager)"
EXPECTED="$("$ROOT/scripts/build_info.sh" "$BUILD" module_count)"

"$PACKAGER" reseal \
  --namespace-root "$MODULE_ROOT" \
  --signing-seed "$BUILD/.module-build-key/module-build.seed" \
  --expect-count "$EXPECTED"

# NOTHING may touch a module native after this point. --require-binding,
# because this leg MUST ship bound artifacts and the build default is the
# weaker one. Without it a configure line that lost
# -DGPGFRONTEND_INTEGRATED_MODULE_NATIVE_BINDING=REQUIRED would verify green
# and release an unbound AppImage.
"$ROOT/scripts/audit_module_natives.sh" \
  --namespace-root "$MODULE_ROOT" \
  --packager "$PACKAGER" \
  --qt-relative ../../../.. \
  --expect-count "$EXPECTED" \
  --require-binding \
  --assert-no-native-outside "$APP_DIR"
echo "::endgroup::"

# ---------------------------------------------------------------------------
echo "::group::Build the AppImage"
APPIMAGETOOL="$TOOLS/appimagetool.AppImage"
wget -nv -O "$APPIMAGETOOL" \
  "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-$ARCH.AppImage"
chmod u+x "$APPIMAGETOOL"

# appimagetool only looks for the legacy <desktop-id>.appdata.xml name, so the
# installed metainfo is renamed rather than copied: the image carries one
# component, identical to what distributions get.
METAINFO_DIR="$APP_DIR/usr/share/metainfo"
mv "$METAINFO_DIR/com.bktus.gpgfrontend.metainfo.xml" \
  "$METAINFO_DIR/com.bktus.gpgfrontend.appdata.xml"

# A failed `appstreamcli validate-tree` aborts appimagetool. Ubuntu 22.04 has
# no appstreamcli, and its packaged 0.15 rejects URL types newer than itself
# (vcs-browser) that 1.x accepts, so there the check is skipped; the 24.04
# runners validate the same file.
APPSTREAM_ARGS=()
if ! appstreamcli --version 2>/dev/null \
    | grep -Eq 'version: ([1-9][0-9]*)\.'; then
  echo "appstreamcli >= 1.0 not available, skipping AppStream validation"
  APPSTREAM_ARGS=(--no-appstream)
fi

# Same naming scheme as every other platform. linuxdeployqt's own -appimage
# named the image after the .desktop entry, so both flavours came out as
# Gpg_Frontend-<arch>.AppImage and collided once the release job merged every
# runner's artifacts into one directory.
OUTPUT="$OUT_DIR/GpgFrontend-${ARTIFACT_VERSION}-linux-${ARCH}${FLAVOR_SUFFIX}.AppImage"
ARCH="$ARCH" "$APPIMAGETOOL" "${APPSTREAM_ARGS[@]}" "$APP_DIR" "$OUTPUT"
ls -l "$OUT_DIR"
echo "::endgroup::"
