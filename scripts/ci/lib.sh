# shellcheck shell=bash
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
# What the platform build scripts (scripts/ci/<platform>/build.sh) share,
# sourced by them after they set ROOT:
#
#   build_gpgme <sudo or ""> [<configure flag>...]
#       the static GpgME of third_party/gpgme, through ccache, installed
#       (with sudo where the prefix is the system's)
#   ccache_keep_working_set
#       the cache trimmed to what this build used, so the saved cache stays
#       the working set instead of growing toward max-size run after run
#       (CCACHE_EPOCH: when the job started, set by the workflow)

cpus() {
  nproc 2>/dev/null || sysctl -n hw.logicalcpu
}

build_gpgme() {
  local install="$1"
  shift
  echo "::group::Build GpgME"
  (
    cd "${ROOT:?}/third_party/gpgme" || exit 1
    export CC="ccache gcc"
    export CXX="ccache g++"
    ./autogen.sh
    mkdir -p build && cd build || exit 1
    ../configure --enable-static \
      --disable-shared \
      --enable-silent-rules \
      --disable-dependency-tracking \
      --enable-languages=cl \
      --disable-gpgconf-test \
      --disable-gpg-test \
      --disable-gpgsm-test \
      --disable-g13-test \
      "$@"
    make -j"$(cpus)"
    if [ -n "$install" ]; then
      "$install" make install
    else
      make install
    fi
  )
  ccache -s -v 2>/dev/null || ccache -s
  echo "::endgroup::"
}

ccache_keep_working_set() {
  : "${CCACHE_EPOCH:?}"
  ccache -s -v 2>/dev/null || ccache -s
  ccache --evict-older-than "$(($(date +%s) - CCACHE_EPOCH + 60))s"
  ccache -s
}
