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
# Build the static GpgME, then GpgFrontend, in the MSYS2 MinGW64 shell.
#
# Environment: BUILD_TYPE, GPGFRONTEND_BUILD_MODE, FLAVOR, CCACHE_EPOCH, and
# SCREENSHOT_CMAKE_FLAG (empty, or the screenshot-mode CMake flags).

set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
: "${BUILD_TYPE:?}" "${GPGFRONTEND_BUILD_MODE:?}" "${FLAVOR:?}" "${CCACHE_EPOCH:?}"
cd "$ROOT"
# shellcheck source=scripts/ci/lib.sh
. "$ROOT/scripts/ci/lib.sh"

(
  export CFLAGS="${CFLAGS:-} -Wno-int-conversion -Wno-incompatible-pointer-types"
  build_gpgme "" --enable-maintainer-mode --enable-w32-glib
)

echo "::group::Build GpgFrontend"
# Unquoted on purpose: empty, or two separate -D flags.
# shellcheck disable=SC2086
cmake -G "Ninja" -S . -B build \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DGPGFRONTEND_BUILD_MODE="$GPGFRONTEND_BUILD_MODE" \
  -DGPGFRONTEND_BUILD_FLAVOR="$FLAVOR" \
  -DGPGFRONTEND_INSTALL_KIND="$([ "$FLAVOR" = portable ] && echo portable-zip || echo msi)" \
  -DGPGFRONTEND_LINK_GPGME_INTO_CORE=ON \
  -DGPGFRONTEND_INTEGRATED_MODULE_NATIVE_BINDING=REQUIRED \
  -DGPGFRONTEND_INTEGRATED_MODULE_BINDING_ALGORITHM=PE_AUTHENTICODE_SHA256 \
  ${SCREENSHOT_CMAKE_FLAG:-}
cmake --build build --config "$BUILD_TYPE" --parallel --verbose
ccache_keep_working_set
echo "::endgroup::"
