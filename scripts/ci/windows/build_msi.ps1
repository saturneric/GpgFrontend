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
# Build the MSI installer from the staged payload. An MSI installs into
# Program Files and keeps its data in the user profile, so it is only ever
# built from the installed flavour.

param(
  [Parameter(Mandatory = $true)][string]$OutputName
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot/../../..").Path

# The version the build actually used, asked of the build. Scraping it out of
# CMakeLists.txt with a regular expression worked until the day someone
# reformatted that line.
$info = Get-Content "$root/build/artifacts/build-info.json" -Raw | ConvertFrom-Json
$version = $info.project_version
Write-Host "Project version: $version"

# Toolset and extension versions must match: an unpinned extension resolves to
# the latest major (7.x), which a WiX 5 host cannot load (wixext5 vs wixext7).
$wixVersion = '5.0.2'
dotnet tool install --global wix --version $wixVersion
wix extension add -g WixToolset.UI.wixext/$wixVersion
wix extension add -g WixToolset.Util.wixext/$wixVersion

$out = "$root/build/upload-artifact"
New-Item -ItemType Directory -Force -Path $out | Out-Null

# -arch x64 is mandatory: the package is MsiPackageType=x64. PayloadDir /
# ProductVersion / IconSource / BrandingDir override the wxs defaults for this
# runner (its defaults assume a build run from resource/wix).
wix build -arch x64 `
  -ext WixToolset.UI.wixext/$wixVersion `
  -ext WixToolset.Util.wixext/$wixVersion `
  -d PayloadDir="$root/build/artifacts" `
  -d ProductVersion="$version" `
  -d IconSource="$root/gpgfrontend.ico" `
  -d BrandingDir="$root/resource/wix" `
  -o "$out/$OutputName" `
  "$root/resource/wix/Package.wxs"
if ($LASTEXITCODE -ne 0) { throw "wix build failed with exit code $LASTEXITCODE" }

# wix drops a .wixpdb next to the .msi; it is build metadata, not a deliverable.
Remove-Item -Force -ErrorAction SilentlyContinue "$out/*.wixpdb"
