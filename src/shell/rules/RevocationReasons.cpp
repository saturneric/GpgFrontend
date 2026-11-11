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

#include "shell/rules/RevocationReasons.h"

namespace GpgFrontend::Shell {

auto RevocationReasonsFor(RevocationTarget target) -> QList<RevocationReason> {
  // Spelled out at each call, which is what lupdate reads.
  const auto none = QCoreApplication::translate(
      "GpgFrontend::Shell::RevocationReasons", "No reason given");
  if (target == RevocationTarget::kUserId) {
    return {
        {0, none},
        {4, QCoreApplication::translate("GpgFrontend::Shell::RevocationReasons",
                                        "The user ID is no longer valid")}};
  }
  return {
      {0, none},
      {1, QCoreApplication::translate("GpgFrontend::Shell::RevocationReasons",
                                      "It may have been compromised")},
      {2, target == RevocationTarget::kKey
              ? QCoreApplication::translate(
                    "GpgFrontend::Shell::RevocationReasons",
                    "A newer key replaces it")
              : QCoreApplication::translate(
                    "GpgFrontend::Shell::RevocationReasons",
                    "A newer subkey replaces it")},
      {3, QCoreApplication::translate("GpgFrontend::Shell::RevocationReasons",
                                      "It is no longer used")},
  };
}

auto DefaultRevocationReason(RevocationTarget target) -> int {
  return target == RevocationTarget::kUserId ? 4 : 3;
}

}  // namespace GpgFrontend::Shell
