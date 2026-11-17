# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Assets\ of the MSIX, drawn from the application's icon: every logo
# AppxManifest.xml names, in each scale and size Windows asks for. They are
# kept nowhere: the offline stage runs this into the folder it packs, each
# time it makes a package (scripts/release/gfrelease/msix.py; Windows
# PowerShell, which brings System.Drawing).
#
# The icon is cut to what is drawn of it and scaled down from its largest
# form with its soft edge kept. A list icon (the taskbar, the Start menu's
# list, a file of the application's type) is the icon alone; a tile is the
# icon in the middle of a transparent one.

param(
  # The application's icon, as large as there is one (a PNG).
  [Parameter(Mandatory = $true)][string]$Icon,
  # The package's Assets folder, made if it is not there.
  [Parameter(Mandatory = $true)][string]$Assets
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing


# The icon without the empty margin around it.
$whole = [System.Drawing.Bitmap]::FromFile((Resolve-Path $Icon).Path)
$left = $whole.Width; $top = $whole.Height; $right = -1; $bottom = -1
for ($y = 0; $y -lt $whole.Height; $y++) {
  for ($x = 0; $x -lt $whole.Width; $x++) {
    if ($whole.GetPixel($x, $y).A -gt 0) {
      if ($x -lt $left) { $left = $x }; if ($x -gt $right) { $right = $x }
      if ($y -lt $top) { $top = $y }; if ($y -gt $bottom) { $bottom = $y }
    }
  }
}
$drawn = New-Object System.Drawing.Rectangle $left, $top, ($right - $left + 1), ($bottom - $top + 1)

# One logo: the icon, @p share of the canvas' smaller side, in its middle.
function Write-Logo([string]$name, [int]$width, [int]$height, [double]$share) {
  $canvas = New-Object System.Drawing.Bitmap $width, $height, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
  $g = [System.Drawing.Graphics]::FromImage($canvas)
  $g.Clear([System.Drawing.Color]::Transparent)
  $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
  $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
  $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
  $side = [Math]::Min($width, $height) * $share
  $scale = $side / [Math]::Max($drawn.Width, $drawn.Height)
  $w = $drawn.Width * $scale; $h = $drawn.Height * $scale
  $target = New-Object System.Drawing.RectangleF (($width - $w) / 2), (($height - $h) / 2), $w, $h
  # Without the wrap mode the edge pixels are blended with nothing, and the
  # icon gets a pale line around it.
  $attributes = New-Object System.Drawing.Imaging.ImageAttributes
  $attributes.SetWrapMode([System.Drawing.Drawing2D.WrapMode]::TileFlipXY)
  $points = [System.Drawing.PointF[]]@(
    (New-Object System.Drawing.PointF $target.Left, $target.Top),
    (New-Object System.Drawing.PointF $target.Right, $target.Top),
    (New-Object System.Drawing.PointF $target.Left, $target.Bottom))
  $g.DrawImage($whole, $points, $drawn, [System.Drawing.GraphicsUnit]::Pixel, $attributes)
  $g.Dispose()
  $canvas.Save("$Assets/$name.png", [System.Drawing.Imaging.ImageFormat]::Png)
  $canvas.Dispose()
}

New-Item -ItemType Directory -Force -Path $Assets | Out-Null

$scales = 100, 125, 150, 200, 400
# name, width and height at scale 100, the icon's share of it
$logos = @(
  @('StoreLogo', 50, 50, 1.0),
  @('GPGFRONTEND-Square44x44Logo', 44, 44, 1.0),
  @('GPGFRONTEND-Square71x71Logo', 71, 71, 0.7),
  @('GPGFRONTEND-Square150x150Logo', 150, 150, 0.6),
  @('GPGFRONTEND-Square310x310Logo', 310, 310, 0.6),
  @('GPGFRONTEND-Wide310x150Logo', 310, 150, 0.6))
foreach ($logo in $logos) {
  foreach ($scale in $scales) {
    Write-Logo "$($logo[0]).scale-$scale" ([Math]::Round($logo[1] * $scale / 100)) ([Math]::Round($logo[2] * $scale / 100)) $logo[3]
  }
}
# The sizes a list shows the application in, with and without the plate
# Windows may put behind an icon: the same picture, which needs none.
foreach ($size in 16, 24, 32, 48, 256) {
  Write-Logo "GPGFRONTEND-Square44x44Logo.targetsize-$size" $size $size 1.0
  Write-Logo "GPGFRONTEND-Square44x44Logo.targetsize-${size}_altform-unplated" $size $size 1.0
}
# A .gfp file's icon.
Write-Logo 'gfp' 256 256 1.0

$whole.Dispose()
"wrote $((Get-ChildItem "$Assets/*.png").Count) logos to $Assets"
