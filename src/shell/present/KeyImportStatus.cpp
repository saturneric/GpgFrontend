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

#include "shell/present/KeyImportStatus.h"

namespace GpgFrontend::Shell {

namespace {

// GnuPG's import status bits (GPGME_IMPORT_*), which the rPGP engine follows.
constexpr int kNewKey = 1;
constexpr int kNewUid = 2;
constexpr int kNewSignature = 4;
constexpr int kNewSubkey = 8;
constexpr int kSecret = 16;

}  // namespace

auto DescribeKeyImportStatus(int import_status) -> QString {
  // The desktop's import report said these first; its context keeps their
  // translations. Spelled out at each call, which is what lupdate reads.
  QStringList parts;
  parts << ((import_status & kSecret) != 0
                ? QCoreApplication::translate(
                      "GpgFrontend::UI::KeyImportDetailDialog", "Private")
                : QCoreApplication::translate(
                      "GpgFrontend::UI::KeyImportDetailDialog", "Public"));
  if ((import_status & kNewKey) != 0) {
    parts << QCoreApplication::translate(
        "GpgFrontend::UI::KeyImportDetailDialog", "New Key");
  } else {
    if ((import_status & kNewUid) != 0) {
      parts << QCoreApplication::translate(
          "GpgFrontend::UI::KeyImportDetailDialog", "New UID");
    }
    if ((import_status & kNewSignature) != 0) {
      parts << QCoreApplication::translate(
          "GpgFrontend::UI::KeyImportDetailDialog", "New Signature");
    }
    if ((import_status & kNewSubkey) != 0) {
      parts << QCoreApplication::translate(
          "GpgFrontend::UI::KeyImportDetailDialog", "New Subkey");
    }
  }
  if (parts.size() == 1) {
    parts << QCoreApplication::translate(
        "GpgFrontend::UI::KeyImportDetailDialog", "Unchanged");
  }
  return parts.join(u", "_s);
}

auto KeyImportChangeOf(int import_status) -> KeyImportChange {
  if ((import_status & kNewKey) != 0) return KeyImportChange::kNewKey;
  if ((import_status & (kNewUid | kNewSignature | kNewSubkey)) != 0) {
    return KeyImportChange::kUpdated;
  }
  return KeyImportChange::kUnchanged;
}

}  // namespace GpgFrontend::Shell
