#!/bin/sh
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
# The name of a file this build publishes, the one way every job spells it:
#
#   artifact_name.sh <leg>      linux-x86_64.AppImage, windows-x86_64.msi,
#                               macos-15.dmg, android-arm64-v8a.apk, ...
#
# prints <artifact_prefix><version>-<leg>, the version being
# $ARTIFACT_VERSION (the file-safe one `gfrelease ci build-mode` derives).
# release/product.json's artifact rules read the leg back (os, arch, kind,
# flavour); scripts/tests/test_artifact_names.py holds the prefix here to
# product.json's and every leg CI makes to a rule.

set -eu

leg="${1:?usage: $0 <leg>}"
echo "GpgFrontend-${ARTIFACT_VERSION:?}-${leg}"
