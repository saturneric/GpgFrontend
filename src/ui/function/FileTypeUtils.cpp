/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
 *
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GpgFrontend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from
 * the gpg4usb project, which is under GPL-3.0-or-later.
 *
 * All the source code of GpgFrontend was modified and released by
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "ui/function/FileTypeUtils.h"

#include "core/profile/ProfilePackage.h"

namespace GpgFrontend::UI {

namespace {

/// Lower-cased suffix of a file system entry, without the dot. Matching is
/// case insensitive because a file named KEY.ASC is the same thing as key.asc.
auto LowerSuffix(const QFileInfo& info) -> QString {
  return info.suffix().toLower();
}

}  // namespace

auto IsOpenPGPMessageFile(const QFileInfo& info) -> bool {
  const auto suffix = LowerSuffix(info);
  return suffix == "gpg" || suffix == "pgp" || suffix == "asc";
}

auto IsOpenPGPRelatedFile(const QFileInfo& info) -> bool {
  return IsOpenPGPMessageFile(info) || IsOpenPGPSignatureFile(info);
}

auto IsOpenPGPSignatureFile(const QFileInfo& info) -> bool {
  return LowerSuffix(info) == "sig";
}

auto IsProfilePackageFile(const QFileInfo& info) -> bool {
  // Taken from the format's own constant rather than spelled out again here:
  // what the file panel calls a profile file and what the packing code writes
  // must be the same thing, and two literals are how that stops being true.
  static const auto kSuffix =
      QString::fromLatin1(kProfilePackageExtension).mid(1).toLower();
  return LowerSuffix(info) == kSuffix;
}

}  // namespace GpgFrontend::UI
