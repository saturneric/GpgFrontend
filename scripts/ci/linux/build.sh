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
# Configure and build the Linux release tree.
#
# Environment: BUILD_TYPE, GPGFRONTEND_BUILD_MODE, FLAVOR, CCACHE_EPOCH.

set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
: "${BUILD_TYPE:?}" "${GPGFRONTEND_BUILD_MODE:?}" "${FLAVOR:?}" "${CCACHE_EPOCH:?}"

export LD_LIBRARY_PATH="/usr/local/lib:/usr/local/lib64:${LD_LIBRARY_PATH:-}"

# Native binding is REQUIRED here and OFF by default, which is the one place
# the two legitimately differ. An AppImage has no map-time code-integrity
# enforcement of its own -- nothing like Apple's signature checked by dyld --
# so the exact-ELF binding is the only thing that would notice a stale,
# corrupt or simply wrong native inside the image. --require-binding in
# package.sh proves this setting was not lost, because an unbound tree
# verifies perfectly well without it.
#
# No GPGFRONTEND_BUILD_APP_IMAGE: release packaging does not redirect where the
# build puts things. The AppDir is produced by `cmake --install`, which is the
# same tree a distribution gets, so CI does not assemble something nobody else
# can reproduce.
#
# LIBDIR is pinned because a /usr prefix on Debian/Ubuntu makes GNUInstallDirs
# pick the multiarch lib/<triplet>, which put the modules under
# usr/lib/aarch64-linux-gnu/ -- where neither the deployment nor the AppImage
# runtime ($APPDIR/usr/lib/gpgfrontend/modules) looks for them.
cmake -B "$ROOT/build" -S "$ROOT" -G Ninja \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DCMAKE_INSTALL_LIBDIR=lib \
  -DGPGFRONTEND_BUILD_MODE="$GPGFRONTEND_BUILD_MODE" \
  -DGPGFRONTEND_BUILD_FLAVOR="$FLAVOR" \
  -DGPGFRONTEND_LINK_GPGME_INTO_CORE=ON \
  -DGPGFRONTEND_INTEGRATED_MODULE_NATIVE_BINDING=REQUIRED \
  -DGPGFRONTEND_INTEGRATED_MODULE_BINDING_ALGORITHM=FILE_SHA256
cmake --build "$ROOT/build" --config "$BUILD_TYPE" --parallel --verbose

ccache -s -v 2>/dev/null || ccache -s
# Keep only what this build used, so the saved cache stays the working set
# instead of growing toward max-size run after run.
ccache --evict-older-than "$(( $(date +%s) - CCACHE_EPOCH + 60 ))s"
ccache -s
