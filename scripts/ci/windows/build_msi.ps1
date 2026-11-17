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
# Build the MSI installer from a staged payload: the Windows build as the
# portable zip has it, without bin\ENV.ini (see resource/wix/Package.wxs).

param(
  [Parameter(Mandatory = $true)][string]$PayloadDir,
  [Parameter(Mandatory = $true)][string]$BuildInfo,
  [Parameter(Mandatory = $true)][string]$OutputPath
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot/../../..").Path

# The version the build actually used, asked of the build. An MSI
# ProductVersion is numeric (a.b.c), so it takes the version's X.Y.Z; the full
# SemVer, prerelease and all, is in the file name.
$info = Get-Content $BuildInfo -Raw | ConvertFrom-Json
$version = $info.version_core
Write-Host "Project version: $($info.project_version) (MSI ProductVersion $version)"

if (Test-Path (Join-Path $PayloadDir 'bin/ENV.ini')) {
  throw "the MSI payload holds bin/ENV.ini, which would make the install portable"
}

# Toolset and extension versions must match: an unpinned extension resolves to
# the latest major (7.x), which a WiX 5 host cannot load (wixext5 vs wixext7).
$wixVersion = '5.0.2'
dotnet tool install --global wix --version $wixVersion
if ($LASTEXITCODE -ne 0) { throw "installing wix failed with exit code $LASTEXITCODE" }
wix extension add -g WixToolset.UI.wixext/$wixVersion
wix extension add -g WixToolset.Util.wixext/$wixVersion

New-Item -ItemType Directory -Force -Path (Split-Path $OutputPath) | Out-Null

wix build -arch x64 `
  -ext WixToolset.UI.wixext/$wixVersion `
  -ext WixToolset.Util.wixext/$wixVersion `
  -d PayloadDir="$PayloadDir" `
  -d ProductVersion="$version" `
  -d IconSource="$root/gpgfrontend.ico" `
  -d BrandingDir="$root/resource/wix" `
  -o "$OutputPath" `
  "$root/resource/wix/Package.wxs"
if ($LASTEXITCODE -ne 0) { throw "wix build failed with exit code $LASTEXITCODE" }

# wix drops a .wixpdb next to the .msi; it is build metadata, not a deliverable.
Remove-Item -Force -ErrorAction SilentlyContinue ([IO.Path]::ChangeExtension($OutputPath, '.wixpdb'))
